#include "Arduino.h"
#include "FS.h"
#include "SD.h"
#include "SPI.h"
#include <stdint.h>
#include <SD_handler.h>
#include <noise_filter.h>

#define BUFFER_SIZE 4096

const int GPIO_pin= 4;
int ADC_VALUE = 0;
float voltage_value = 0; 

void setup(){

    Serial.begin(115200);
    uint8_t retries = 10;
    while (!SD.begin()){
        if (!retries){
            Serial.println("Card Mount Failed");
            return;
         }
         retries--;
         delay(500);
    }
    uint8_t cardType = SD.cardType();

    if(cardType == CARD_NONE){
        Serial.println("No SD card attached");
        return;
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

    uint16_t* audioBuffer1 = (uint16_t *)malloc(2*BUFFER_SIZE);
    uint16_t* audioBuffer2 = (uint16_t *)malloc(2*BUFFER_SIZE);

    int buffer1_head = 0;
    int buffer2_head = 0;
    int buffer1_tail = BUFFER_SIZE;
    int buffer2_tail = BUFFER_SIZE;
    
}

void loop(){
  ADC_VALUE = analogRead(GPIO_pin);
  Serial.print("ADC VALUE = ");
  Serial.println(ADC_VALUE);
  voltage_value = (ADC_VALUE * 3.3 ) / (4096);
  Serial.print("Voltage = ");
  Serial.print(voltage_value);
  Serial.println(" volts");
}
