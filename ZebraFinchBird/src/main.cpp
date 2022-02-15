#include "Arduino.h"
#include "FS.h"
#include "SD.h"
#include "SPI.h"
#include "String.h"
#include <stdint.h>
#include <SD_handler.h>
#include <noise_filter.h>
#include "soc/rtc_wdt.h"
#include "esp_int_wdt.h"
#include "esp_task_wdt.h"

// Configuration Variables
#define SINGLE_BUFFER_SIZE 6000
#define COMPLETE_BUFFER_SIZE int(1.25*SINGLE_BUFFER_SIZE)
#define THRESHOLD 7

// Tasks definition
void ADC_Reader(void * pvParameters);
void Signal_Processing(void * pvParameters);

// Global variables
static TaskHandle_t ADCTask = NULL;
static TaskHandle_t SDTask = NULL;
static hw_timer_t *timer = NULL;
static const uint16_t timer_divider = 80;      // Divide 80 MHz by this
static const uint64_t timer_max_count = 167;  // Timer counts to this value
double* dp_audioBuffer = (double *)malloc(sizeof(double)*COMPLETE_BUFFER_SIZE); // Complete buffer allocated for double values 
int i_buffer1Head = 0; // integer indicating the head of the first buffer
int i_buffer2Head = 0.2*COMPLETE_BUFFER_SIZE; // integer indicating the head of the second buffer
int i_buffer1Tail = 0.8*COMPLETE_BUFFER_SIZE; // integer indicating the tail of the first buffer
int i_buffer2Tail = COMPLETE_BUFFER_SIZE; // integer indicating the tail of the second buffer
int i_head = -1; // helper integer to specify head of buffer for filtering
int i_iter = 0; // iterator for the index of the buffer
// int i_startingIndexDecision = 0;
double d_filteredPrev1 = 0; // before last element of previously filtered buffer
double d_filteredPrev2 = 0; // last element of previously filtered buffer

// ADC Read and Save variables
const int GPIO_pin = A4; // Pin to read accelerometer values from
String dataMessage = ""; // String to save buffer values in before writing to SD card

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

static const BaseType_t pro_cpu = 0;
static const BaseType_t app_cpu = 1;

// Pins
static const int adc_pin = A0;

void IRAM_ATTR onTimer() {

    BaseType_t task_woken = pdFALSE;

    dp_audioBuffer[i_iter] = analogRead(adc_pin);
    if (i_iter == i_buffer1Tail){ // when buffer 1 is full
        i_head = i_buffer1Head;
        i_buffer1Head = ((int)(i_buffer1Head+0.4*COMPLETE_BUFFER_SIZE)%(COMPLETE_BUFFER_SIZE+1)); // changes head position on the first buffer to start saving 50% later 
        i_buffer1Tail = ((int)(i_buffer1Tail+0.4*COMPLETE_BUFFER_SIZE)%COMPLETE_BUFFER_SIZE); // changes tail position on the first buffer to end 50% later
        vTaskNotifyGiveFromISR(SDTask, &task_woken);
    }
        
    if (i_iter == i_buffer2Tail){ // when buffer 2 is full
        i_head = i_buffer2Head;
        i_buffer2Head = ((int)(i_buffer2Head+0.4*COMPLETE_BUFFER_SIZE)%(COMPLETE_BUFFER_SIZE+1)); // changes head position on the second buffer to start saving 50% later
        i_buffer2Tail = ((int)(i_buffer2Tail+0.4*COMPLETE_BUFFER_SIZE)%COMPLETE_BUFFER_SIZE); // changes tail position on the second buffer to end 50% later
        vTaskNotifyGiveFromISR(SDTask, &task_woken);
    }
    i_iter++;
    i_iter = i_iter%(COMPLETE_BUFFER_SIZE+1);

    if (task_woken) {
    portYIELD_FROM_ISR();
    }
}

void ADC_Reader(void *parameters) {

    Serial.print("ADC Reader on core ");
    Serial.println(xPortGetCoreID());

    timer = timerBegin(0, timer_divider, true);
    timerAttachInterrupt(timer, &onTimer, true);
    timerAlarmWrite(timer, timer_max_count, true);
    timerAlarmEnable(timer);
    while (1) {
    vTaskDelay(1 / portTICK_PERIOD_MS);
  }
}

// Wait for semaphore and calculate average of ADC values
void Signal_Processing(void *parameters){

    Serial.print("SD Writer on core ");
    Serial.println(xPortGetCoreID());
    double* dp_bufferSignal = (double *)malloc(sizeof(double)*SINGLE_BUFFER_SIZE); // buffer to be analyzed

    while (1) {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        Serial.println(esp_timer_get_time());

        // populate correct values starting from specified and looping back as if circular queue 
        for (int i=0; i<SINGLE_BUFFER_SIZE;i++){
            dp_bufferSignal[i] = dp_audioBuffer[(i_head+i)%(COMPLETE_BUFFER_SIZE+1)]; // implements circular queue
        }
        
        // filter signal
        double* dp_filteredSignal = filter_signal(dp_bufferSignal, SINGLE_BUFFER_SIZE, d_filteredPrev1, d_filteredPrev2); // filtered signal
        d_filteredPrev1 = dp_filteredSignal[int(SINGLE_BUFFER_SIZE*0.25)+0];
        d_filteredPrev2 = dp_filteredSignal[int(SINGLE_BUFFER_SIZE*0.25)+1];

        appendFile(SD, "/data.txt", dataMessage.c_str());
        dataMessage = "";
    }
}

void setup() {

    Serial.begin(115200);

    vTaskDelay(1000 / portTICK_PERIOD_MS);

    if (!initSDCard()) return;
    File file = SD.open("/data.txt");
    if(!file) {
        Serial.println("File doesn't exist");
        Serial.println("Creating file...");
        writeFile(SD, "/data.txt", "Voltage \r\n");
    }
    else {
        Serial.println("File already exists");  
    }
    file.close();

  xTaskCreatePinnedToCore(ADC_Reader,
                          "ADC Reader",
                          1024,
                          NULL,
                          1,
                          &ADCTask,
                          0);

  xTaskCreatePinnedToCore(Signal_Processing,
                          "SD Writer",
                          4096,
                          NULL,
                          2,
                          &SDTask,
                          1);
  
  vTaskDelete(NULL);
}

void loop() {
  vTaskDelete(NULL);
}