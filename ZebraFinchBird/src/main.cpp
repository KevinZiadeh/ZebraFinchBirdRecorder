#include "Arduino.h"
#include "FS.h"
#include "SD.h"
#include "SPI.h"
#include <stdint.h>
#include <SD_handler.h>
#include <noise_filter.h>

// Libraries to get time from NTP Server
#include <WiFi.h>
#include "time.h"

#define SINGLE_BUFFER_SIZE 1500
#define COMPLETE_BUFFER_SIZE int(1.25*SINGLE_BUFFER_SIZE)
#define THRESHOLD 7

void ADC_Reader(void * pvParameters);
void SD_Writer(void * pvParameters);

// Global Variables
double d_filteredPrev1 = 0; // before last element of previously filtered buffer
double d_filteredPrev2 = 0; // last element of previously filtered buffer
double u16_filteredPrev1 = 0; // before last element of previously filtered buffer
double u16_filteredPrev2 = 0; // last element of previously filtered buffer
double d_rawPrev1 = 0; // before last element of previous buffer before filtering
double d_rawPrev2 = 0; // last element of previous buffer before filtering
int i_vocalisationDetected = 0; // result of the analysis to decide if we want to save or not 
int i_mergeState = 0; // selects if we need to merge current buffer with previous one: THREE STATES -> YES, YES/NO, NO. This is done in order to save instances together when there is only one buffer of difference of no
int i_startCopyIndex = (int)((0.6*COMPLETE_BUFFER_SIZE+0.5)); // selects the beginning of the last 25% of the SINGLE BUFFER (which is 80% of the BUFFER)
double* dp_filteredSignalAfterBuffer = (double *)malloc(sizeof(double)*262144); 
int i_filteredSignalAfterBufferIndex = 0; 
int b_startingIndex = false; 
int d_notchedSignalPrev1 = 0;
int d_notchedSignalPrev2 = 0;
int d_notchedReferenceSignalPrev1 = 0;
int d_notchedReferenceSignalPrev2 = 0;
double* dp_audioBuffer = (double *)malloc(sizeof(double)*COMPLETE_BUFFER_SIZE); // Complete buffer allocated for double values 
int i_buffer1Head = 0; // integer indicating the head of the first buffer
int i_buffer2Head = 0.2*COMPLETE_BUFFER_SIZE; // integer indicating the head of the second buffer
int i_buffer1Tail = 0.8*COMPLETE_BUFFER_SIZE; // integer indicating the tail of the first buffer
int i_buffer2Tail = COMPLETE_BUFFER_SIZE; // integer indicating the tail of the second buffer
int i_head = -1; // helper integer to specify head of buffer for filtering


// Replace with your network credentials
const char* ssid     = "SamsungKevin";
const char* password = "tareksoubra";

// Timer variables
unsigned long lastTime = 0;
unsigned long timerDelay = 30000;

// ADC Read and Save variables
const int GPIO_pin= 36; // This is pin A4
int ADC_VALUE = 0;
float voltage_value = 0;
String dataMessage = "";

// Testing/Debugging parameters


// NTP server to request epoch time
const char* ntpServer = "pool.ntp.org";

// Variable to save current epoch time
unsigned long epochTime; 

// Function that gets current epoch time
unsigned long getTime() {
  time_t now;
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    //Serial.println("Failed to obtain time");
    return(0);
  }
  time(&now);
  return now;
}

// Initialize WiFi
void initWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi ..");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print('.');
    delay(1000);
  }
  Serial.println(WiFi.localIP());
}

// Initialize SD Card
bool initSDCard(){
    uint8_t retries = 50;
    while (!SD.begin()){
        if (!retries){
            Serial.println("Card Mount Failed");
            return false;
         }
         retries--;

    }
    uint8_t cardType = SD.cardType();

    if(cardType == CARD_NONE){
        Serial.println("No SD card attached");
        return false;
    }

    Serial.print("SD Card Type: ");
    if(cardType == CARD_MMC){
        Serial.println("MMC");
    } else if(cardType == CARD_SD){
        Serial.println("SDSC");
    } else if(cardType == CARD_SDHC){
        Serial.println("SDHC");
    } else {
        Serial.println("UNKNOWN");
    }

    uint64_t cardSize = SD.cardSize() / (1024 * 1024);
    Serial.printf("SD Card Size: %lluMB\n", cardSize);
    Serial.printf("Total space: %lluMB\n", SD.totalBytes() / (1024 * 1024));
    Serial.printf("Used space: %lluMB\n", SD.usedBytes() / (1024 * 1024));

    listDir(SD, "/", 0);
    return true;
}


TaskHandle_t ADCTask;
TaskHandle_t SDTask;


void setup(){

    Serial.begin(115200);
    
    // initWiFi();
    if (!initSDCard()) return;
    configTime(0, 0, ntpServer);

    // If the data.txt file doesn't exist
    // Create a file on the SD card and write the data labels
    File file = SD.open("/data.txt");
    if(!file) {
        Serial.println("File doesn't exist");
        Serial.println("Creating file...");
        writeFile(SD, "/data.txt", "Time, ADC Values, Voltage \r\n");
    }
    else {
        Serial.println("File already exists");  
    }
    file.close();
    // epochTime = getTime();



    // uint16_t* audioBuffer1 = (uint16_t *)malloc(2*BUFFER_SIZE);
    // uint16_t* audioBuffer2 = (uint16_t *)malloc(2*BUFFER_SIZE);

    // int buffer1_head = 0;
    // int buffer2_head = 0;
    // int buffer1_tail = BUFFER_SIZE;
    // int buffer2_tail = BUFFER_SIZE;

    xTaskCreatePinnedToCore(ADC_Reader, "ADC Reader", 5000, NULL, 1, &ADCTask, 0);
    
    xTaskCreatePinnedToCore(SD_Writer, "SD Writer", 15000, NULL, 1, &SDTask, 1);
    // vTaskSuspend(SDTask);
    
}

void ADC_Reader(void * pvParameters){
    
    // Serial.print("ADC Reader ");
    // Serial.println(xPortGetCoreID());
    int i_iter = 0; // iterator for the index of the buffer
    
    while(1){
        // Reads data sequentially from array containing all the data -> simulates reading from ADC
        // We lose a maximum of 0.2*COMPLETE_BUFFER_SIZE of data at the end
        ADC_VALUE = analogRead(GPIO_pin);
        // Serial.print("Analog Value Read with i_iter = ");
        // Serial.println(i_iter);
        dp_audioBuffer[i_iter] = ADC_VALUE;
        if (i_iter == i_buffer1Tail){ // when buffer 1 is full
            i_head = i_buffer1Head;
            i_buffer1Head = ((int)(i_buffer1Head+0.4*COMPLETE_BUFFER_SIZE)%(COMPLETE_BUFFER_SIZE+1)); // changes head position on the first buffer to start saving 50% later 
            i_buffer1Tail = ((int)(i_buffer1Tail+0.4*COMPLETE_BUFFER_SIZE)%COMPLETE_BUFFER_SIZE); // changes tail position on the first buffer to end 50% later
            // Serial.print("Filled B1: ");
            // Serial.println(millis());
            xTaskNotifyGive(SDTask);
        }
            
        if (i_iter == i_buffer2Tail){ // when buffer 2 is full
            i_head = i_buffer2Head;
            i_buffer2Head = ((int)(i_buffer2Head+0.4*COMPLETE_BUFFER_SIZE)%(COMPLETE_BUFFER_SIZE+1)); // changes head position on the second buffer to start saving 50% later
            i_buffer2Tail = ((int)(i_buffer2Tail+0.4*COMPLETE_BUFFER_SIZE)%COMPLETE_BUFFER_SIZE); // changes tail position on the second buffer to end 50% later
            // Serial.print("Filled B2: ");
            // Serial.println(millis());
            xTaskNotifyGive(SDTask);
        }
        i_iter++;
        i_iter = i_iter%(COMPLETE_BUFFER_SIZE+1);
    
        vTaskDelay(pdMS_TO_TICKS(1));
        
    }
}

void SD_Writer(void * pvParameters){ 

    const TickType_t xMaxBlockTime = pdMS_TO_TICKS(5);
    uint32_t ulNotificationValue;

    Serial.println(i_head);
    Serial.print("SD Writer ");
    Serial.println(xPortGetCoreID());

    while(1){

        ulNotificationValue = ulTaskNotifyTake(pdTRUE, xMaxBlockTime);
        if (ulNotificationValue > 0){
            int i_startingIndex = (int)((b_startingIndex*0.6*COMPLETE_BUFFER_SIZE+0.5));
            for(int i=i_startingIndex;i<SINGLE_BUFFER_SIZE;i++) {
                voltage_value = (dp_audioBuffer[i+i_startingIndex] * 3.3)/4096;
                dataMessage += String(millis()) + "," + String(dp_audioBuffer[i+i_startingIndex], 16) + "," +
                String(voltage_value) + "\r\n";
            }
            b_startingIndex = true;
            // Serial.println(dataMessage);
            appendFile(SD, "/data.txt", dataMessage.c_str());
        }
    }
}

void loop(){
}

////////////////////////////////////////////////////

// int i = 0;
// float data[6000];
// int timeStamp[6000];
// int initialMillis;


// void setup(){

//     Serial.begin(115200);
//     Serial.println("Testing code beginning: \n");
//     initSDCard();
//     // initWifi();

//     File file = SD.open("/data.txt");
//     if(!file) {
//         Serial.println("File doesn't exist");
//         Serial.println("Creating file...");
//         writeFile(SD, "/data.txt", "Epoch Time, ADC Values, Voltage \r\n");
//     }
//     else {
//         Serial.println("File already exists");  
//     }
//     file.close();

//     Serial.println(millis());
//     initialMillis = millis();


// }

// void loop(){

//     // pinValue = analogRead(26);
//     // Serial.print("The Value on the GPIO Pin is: ");
//     // Serial.println(pinValue);
//     // delay(500);
//     ADC_VALUE = analogRead(GPIO_pin);
//     // delayMicroseconds(35);
//     voltage_value = (ADC_VALUE * 3.3)/4096;
//     data[i] = voltage_value;
//     timeStamp[i] = millis();
//     i++;
//     if(i == 6000){
//         Serial.println(i);
//         for (int j = 0; j < 6000; j++){
//             dataMessage += String(timeStamp[j] - initialMillis, 4) + "," + String(data[j], 6) + "|";
//             Serial.println(dataMessage);
//         }
//         dataMessage += "Wrote to SD Card \n";
//         // Serial.println(dataMessage);
//         appendFile(SD, "/data.txt", dataMessage.c_str());
//         dataMessage = "";
//         // Serial.println(millis());
//         i = 0;

//     }
//     // epochTime = getTime();

//     // dataMessage = String(millis()) + "," + String(ADC_VALUE) + "," + String(voltage_value, 16) + "\r\n";
//     // dataMessage = String((epochTime*1000 + millis())) + "," + String(ADC_VALUE) + "," + String(voltage_value, 16) + "\r\n";
//     // appendFile(SD, "/data.txt", dataMessage.c_str());

//     // if(pinValue < 5){
//     // dataMessage =  "end of saving\r\n";
//     // appendFile(SD, "/data.txt", dataMessage.c_str());
//     //     Serial.println("Pin is pulled down, stop saving");
//     // }

// }