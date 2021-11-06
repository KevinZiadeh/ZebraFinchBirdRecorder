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

#define BUFFER_SIZE 4096

void ADC_Reader(void * pvParameters);
void SD_Writer(void * pvParameters);


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

    while(1){
        // 34 is pin A2
        // int localRead = analogRead(34); 
        // Serial.println(localRead);
        // if(localRead == 0){
        //     Serial.println("Exiting ADC");
        //     SD.end();
        //     return;
        // }
        ADC_VALUE = analogRead(GPIO_pin);
        // Get epoch time
        // epochTime = getTime();
        xTaskNotifyGive(SDTask);
        vTaskDelay(pdMS_TO_TICKS(50));
        // vTaskResume(SDTask);
    }
}

void SD_Writer(void * pvParameters){ 

    const TickType_t xMaxBlockTime = pdMS_TO_TICKS(100);
    uint32_t ulNotificationValue;

    Serial.print("SD Writer ");
    Serial.println(xPortGetCoreID());

    while(1){

        ulNotificationValue = ulTaskNotifyTake(pdTRUE, xMaxBlockTime);
        if (ulNotificationValue > 0){

            // Serial.print("SD Writer ");
            // Serial.println(xPortGetCoreID());

            Serial.print("Voltage = ");
            Serial.println(ADC_VALUE);
            voltage_value = (ADC_VALUE * 3.3)/4096;
            // dataMessage = String((epochTime*1000 + millis())) + "," + String(ADC_VALUE) + "," + String(voltage_value, 16) + "\r\n";
            dataMessage = "Time, " + String(ADC_VALUE) + "," + String(voltage_value, 16) + "\r\n";
            Serial.println(dataMessage);
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