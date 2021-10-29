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
const char* ssid     = "SamsungTarek";
const char* password = "kevinziadeh";

// Timer variables
unsigned long lastTime = 0;
unsigned long timerDelay = 30000;

// ADC Read and Save variables
const int GPIO_pin= 4;
int ADC_VALUE = 0;
float voltage_value = 0;
String dataMessage;

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

// // Write to the SD card
// void writeFile(fs::FS &fs, const char * path, const char * message) {
//   Serial.printf("Writing file: %s\n", path);

//   File file = fs.open(path, FILE_WRITE);
//   if(!file) {
//     Serial.println("Failed to open file for writing");
//     return;
//   }
//   if(file.print(message)) {
//     Serial.println("File written");
//   } else {
//     Serial.println("Write failed");
//   }
//   file.close();
// }

// // Append data to the SD card
// void appendFile(fs::FS &fs, const char * path, const char * message) {
//   Serial.printf("Appending to file: %s\n", path);

//   File file = fs.open(path, FILE_APPEND);
//   if(!file) {
//     Serial.println("Failed to open file for appending");
//     return;
//   }
//   if(file.print(message)) {
//     Serial.println("Message appended");
//   } else {
//     Serial.println("Append failed");
//   }
//   file.close();
// }

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
        writeFile(SD, "/data.txt", "Epoch Time, ADC Values, Voltage \r\n");
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

    // xTaskCreatePinnedToCore(ADC_Reader, "ADC Reader", 5000, NULL, 1, &ADCTask, 0);
    
    // xTaskCreatePinnedToCore(SD_Writer, "SD Writer", 5000, NULL, 1, &SDTask, 1);
    
}

void ADC_Reader(void * pvParameters){
    
    Serial.print("ADC Reader");
    Serial.println(xPortGetCoreID());

    for(;;){
        
        ADC_VALUE = analogRead(GPIO_pin);
        xTaskNotify(SDTask, 1, eIncrement);
    }
}

void SD_Writer(void * pvParameters){

    // Get epoch time
    // epochTime = getTime();

    Serial.print("SD Writer");
    Serial.println(xPortGetCoreID());

    Serial.print("Voltage = ");
    Serial.print(ADC_VALUE);
    voltage_value = (ADC_VALUE * 3.3)/4096;
    dataMessage = String((epochTime*1000 + millis())) + "," + String(ADC_VALUE) + "," + String(voltage_value, 16) + "\r\n";
    appendFile(SD, "/data.txt", dataMessage.c_str());

    vTaskDelete(NULL);
}

void loop(){
    // Serial.println(epochTime*1000 + millis());
}
