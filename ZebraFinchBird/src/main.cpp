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

// // Libraries to get time from NTP Server
// #include <WiFi.h>
// #include "time.h"

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
int i_vocalisationDetected = 0; // result of the analysis to decide if we want to save or not 
double d_notchedSignalPrev1 = 0;
double d_notchedSignalPrev2 = 0;
double d_notchedReferenceSignalPrev1 = 0;
double d_notchedReferenceSignalPrev2 = 0;

// ADC Read and Save variables
const int ADC_GPIO_pin = A5; // Pin to read accelerometer values from

// // Wifi
// // Replace with your network credentials
// const char *ssid = "";
// const char *password = "";
// const char *ntpServer = "pool.ntp.org"; // NTP server to request epoch time
// long epochTime; // Variable to save current epoch time

// // Initialize WiFi
// void initWiFi(){
//     WiFi.mode(WIFI_STA);
//     WiFi.begin(ssid, password);
//     Serial.print("Connecting to WiFi ..");
//     while (WiFi.status() != WL_CONNECTED)
//     {
//         Serial.print('.');
//         delay(1000);
//     }
//     Serial.println(WiFi.localIP());
// }

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

long getTime(){
    time_t now;
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo))
    {
        // Serial.println("Failed to obtain time");
        return (0);
    }
    time(&now);
    return now;
}

void uint64_to_string(uint64_t value, String& result ) {
    result.clear();
    result.reserve( 20 ); // max. 20 digits possible
    uint64_t q = value;
    do {
        result += "0123456789"[ q % 10 ];
        q /= 10;
    } while ( q );
    std::reverse( result.begin(), result.end() );
}

void IRAM_ATTR onTimer() {

    BaseType_t task_woken = pdFALSE;
    dp_audioBuffer[i_iter] = analogRead(ADC_GPIO_pin);
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

    // Serial.println("ADC Reader");

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
    // Serial.println("SD Writer");
    // filter signal        
    double a0 = 0;
    double a1 = 0;
    double a2 = 0;
    double b1 = 0;
    double b2 = 0;
    // analyze signal
    double d_totalPower = 0;
    double d_notchedPower = 0;
    double d_notchedReferencePower = 0;
    bool b_vocalisation_detected = false;
    // saving to sd card
    int i_mergeState = 0;
    float voltage_value = 0;
    String dataMessage = "";
    String s_prevFileName = "";
    String s_FileName = "";

    int* dp_bufferSignal = (int *)malloc(sizeof(int)*SINGLE_BUFFER_SIZE); // buffer to be analyzed
    double* dp_filteredSignal = (double *)malloc(sizeof(double)*SINGLE_BUFFER_SIZE);
    double* dp_notchedSignal = (double *)malloc(sizeof(double)*SINGLE_BUFFER_SIZE);

    while (1) {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        uint64_to_string(esp_timer_get_time(), s_FileName);        
        // uint64_to_string((epochTime*1000000 + esp_timer_get_time()), s_FileName);   
        Serial.println(s_FileName);
        // populate correct values starting from specified and looping back as if circular queue 
        for (int i=0; i<SINGLE_BUFFER_SIZE;i++){
            dp_bufferSignal[i] = dp_audioBuffer[(i_head+i)%(COMPLETE_BUFFER_SIZE+1)]; // implements circular queue
        }
        
        // Filter Signal
        // a0 = 0.8948577513857248;
        // a1 = -1.7897155027714495;
        // a2 = 0.8948577513857248;
        // b1 = -1.7786300789392977;
        // b2 = 0.8008009266036016;
        // dp_filteredSignal[0] = d_filteredPrev1;
        // dp_filteredSignal[1] = d_filteredPrev2;
        // for (int i=2; i<SINGLE_BUFFER_SIZE; i++){
        //     dp_filteredSignal[i] = a0*dp_bufferSignal[i] + a1*dp_bufferSignal[i-1] + a2*dp_bufferSignal[i-2] - b1*dp_filteredSignal[i-1] - b2*dp_filteredSignal[i-2];
        // }
        // d_filteredPrev1 = dp_filteredSignal[int(SINGLE_BUFFER_SIZE*0.25)+0];
        // d_filteredPrev2 = dp_filteredSignal[int(SINGLE_BUFFER_SIZE*0.25)+1];

        // // Analyze Signal
        // d_totalPower = 0;
        // for (int i=0; i<SINGLE_BUFFER_SIZE; i++){
        //     d_totalPower += dp_filteredSignal[i]*dp_filteredSignal[i];
        // }    
        // a0 = 0.3444719716111889;
        // a1 = 0;
        // a2 = -0.3444719716111889;
        // b1 = 0.8772677342420642;
        // b2 = 0.3110560567776222;
        // dp_notchedSignal[0] = d_notchedSignalPrev1;
        // dp_notchedSignal[1] = d_notchedSignalPrev2;
        // for (int i=2; i<SINGLE_BUFFER_SIZE; i++){
        //     dp_notchedSignal[i] = a0*dp_filteredSignal[i] + a1*dp_filteredSignal[i-1] + a2*dp_filteredSignal[i-2] - b1*dp_notchedSignal[i-1] - b2*dp_notchedSignal[i-2];
        // }
        // d_notchedSignalPrev1 = dp_notchedSignal[int(SINGLE_BUFFER_SIZE*0.25)+0];
        // d_notchedSignalPrev2 = dp_notchedSignal[int(SINGLE_BUFFER_SIZE*0.25)+1];
        // d_notchedPower = 0;
        // for (int i=0; i<SINGLE_BUFFER_SIZE; i++){
        //     d_notchedPower += dp_notchedSignal[i]*dp_notchedSignal[i];
        // }

        // a0 = 0.22336671878312517;
        // a1 = 0;
        // a2 = -0.22336671878312517;
        // b1 = -1.4189796126194893;
        // b2 = 0.5532665624337496;
        // dp_notchedSignal[0] = d_notchedReferenceSignalPrev1;
        // dp_notchedSignal[1] = d_notchedReferenceSignalPrev2;
        // for (int i=2; i<SINGLE_BUFFER_SIZE; i++){
        //     dp_notchedSignal[i] = a0*dp_filteredSignal[i] + a1*dp_filteredSignal[i-1] + a2*dp_filteredSignal[i-2] - b1*dp_notchedSignal[i-1] - b2*dp_notchedSignal[i-2];
        // }
        // d_notchedReferenceSignalPrev1 = dp_notchedSignal[int(SINGLE_BUFFER_SIZE*0.25)+0];
        // d_notchedReferenceSignalPrev2 = dp_notchedSignal[int(SINGLE_BUFFER_SIZE*0.25)+1];
        // d_notchedReferencePower = 0;
        // for (int i=0; i<SINGLE_BUFFER_SIZE; i++){
        //     d_notchedReferencePower += dp_notchedSignal[i]*dp_notchedSignal[i];
        // }
        // b_vocalisation_detected = (d_notchedPower/d_notchedReferencePower >= THRESHOLD);

        // Serial.println(d_notchedPower/d_notchedReferencePower);
    
        // Saving to SD card
        // if (b_vocalisation_detected == 1){
        if (b_vocalisation_detected = 1){
            if (i_mergeState == 0){
                for(int i=0;i<(0.25*SINGLE_BUFFER_SIZE);i++) {
                    voltage_value = (dp_bufferSignal[i] * 3.3)/4096;
                    dataMessage += String(voltage_value, 16) + "\r\n";
                }
                writeFile(SD, ("/"+s_FileName+".txt").c_str(), dataMessage.c_str());
                dataMessage = "";
                i_mergeState = 1;
                s_prevFileName = s_FileName;
            }
            else{
                for(int i=0;i<(0.25*SINGLE_BUFFER_SIZE);i++) {
                    voltage_value = (dp_bufferSignal[i] * 3.3)/4096;
                    dataMessage += String(voltage_value, 16) + "\r\n";
                }
                appendFile(SD, ("/"+s_prevFileName+".txt").c_str(), dataMessage.c_str());
                dataMessage = "";
            }
        } else {
            i_mergeState = 0;
        }
    }

}

void setup() {

    Serial.begin(115200);

    vTaskDelay(1000 / portTICK_PERIOD_MS);
    
    // https://esp32.com/viewtopic.php?t=5492
    // initWiFi();
    // configTime(0, 0, ntpServer);
    // epochTime = getTime();
    // WiFi.disconnect();

    if (!initSDCard()) return;

  xTaskCreatePinnedToCore(ADC_Reader,
                          "ADC Reader",
                          1024,
                          NULL,
                          1,
                          &ADCTask,
                          0);

  xTaskCreatePinnedToCore(Signal_Processing,
                          "SD Writer",
                          10000,
                          NULL,
                          2,
                          &SDTask,
                          1);
  
  vTaskDelete(NULL);
}

void loop() {
  vTaskDelete(NULL);
}