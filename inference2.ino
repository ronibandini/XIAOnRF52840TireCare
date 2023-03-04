/* 
 * Xiao nrf52840 TinyML Tire care Inference script
 * Roni Bandini March 2023 @RoniBandini
 * Machine Learning via Edge Impulse
 * Connect button to GND and D1. Connect Led to GND and D2. Connect microSD to GND, VCC, MISO pin to D9, MOSI to D10, SCK to D8, CS to D3.
 */

/* Includes ---------------------------------------------------------------- */

#include <XiaoTireCare_inferencing.h>
#include <LSM6DS3.h>
#include <Wire.h>
#include <Arduino.h>
#include <SPI.h>
#include <SD.h>


/* Constant defines -------------------------------------------------------- */
#define CONVERT_G_TO_MS2    9.80665f
#define MAX_ACCEPTED_RANGE  2.0f        // starting 03/2022, models are generated setting range to +-2, but this example use Arudino library which set range to +-4g. If you are using an older model, ignore this value and use 4.0f instead

/* Private variables ------------------------------------------------------- */
static bool debug_nn = false; // Set this to true to see e.g. features generated from the raw signal
LSM6DS3 myIMU(I2C_MODE, 0x6A);
/**
* @brief      Arduino setup function
*/

File myFile;
String dataString="";
const int buttonPin = 1;
const int ledPin=2;
const float confidence=0.7;

void setup()
{

    Serial.begin(115200);
    Serial.println("Xiao nrf52840 ML Tire Care");
    Serial.println("Roni Bandini");

    pinMode(ledPin, OUTPUT);  
    digitalWrite(ledPin, HIGH);   
    delay(1000);                       
    digitalWrite(ledPin, LOW);    
    delay(3000);  

      if (!myIMU.begin()) {
        ei_printf("Failed to initialize IMU!\r\n");
    }
    else {
        ei_printf("IMU initialized\r\n");
    }

    if (EI_CLASSIFIER_RAW_SAMPLES_PER_FRAME != 3) {
        ei_printf("ERR: EI_CLASSIFIER_RAW_SAMPLES_PER_FRAME should be equal to 3 (the 3 sensor axes)\n");
        return;
    }

    Serial.println("Initializing SD card...");

    if (!SD.begin(3)) {
    Serial.println("initialization failed. Things to check:");
    Serial.println("1. is a card inserted?");
    Serial.println("2. is your wiring correct?");
    Serial.println("3. did you change the chipSelect pin to match your shield or module?");
    Serial.println("Note: press reset button on the board and reopen this Serial Monitor after fixing your issue!");
    while (true);
    }

    Serial.println("SD initialization done.");
  
}

float ei_get_sign(float number) {
    return (number >= 0.0) ? 1.0 : -1.0;
}


void loop()
{
    uint8_t buf1[64]="break";
    uint8_t buf2[64]="driving";
    uint8_t buf3[64]="hardbreak";      
  
    ei_printf("\nStarting inferencing in 2 seconds...\n");

    delay(2000);

    ei_printf("Sampling...\n");

    // Allocate a buffer here for the values we'll read from the IMU
    float buffer[EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE] = { 0 };

    for (size_t ix = 0; ix < EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE; ix += 3) {
        // Determine the next tick (and then sleep later)
        uint64_t next_tick = micros() + (EI_CLASSIFIER_INTERVAL_MS * 1000);

        buffer[ix] = myIMU.readFloatAccelX();
        buffer[ix+1] = myIMU.readFloatAccelY();
        buffer[ix+2] = myIMU.readFloatAccelZ();

        for (int i = 0; i < 3; i++) {
            if (fabs(buffer[ix + i]) > MAX_ACCEPTED_RANGE) {
                buffer[ix + i] = ei_get_sign(buffer[ix + i]) * MAX_ACCEPTED_RANGE;
            }
        }

        buffer[ix + 0] *= CONVERT_G_TO_MS2;
        buffer[ix + 1] *= CONVERT_G_TO_MS2;
        buffer[ix + 2] *= CONVERT_G_TO_MS2;

        delayMicroseconds(next_tick - micros());
    }

    // Turn the raw buffer in a signal which we can the classify
    signal_t signal;
    int err = numpy::signal_from_buffer(buffer, EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE, &signal);
    if (err != 0) {
        ei_printf("Failed to create signal from buffer (%d)\n", err);
        return;
    }

    // Run the classifier
    ei_impulse_result_t result = { 0 };

    err = run_classifier(&signal, &result, debug_nn);
    if (err != EI_IMPULSE_OK) {
        ei_printf("ERR: Failed to run classifier (%d)\n", err);
        return;
    }

    // print the predictions
    ei_printf("Predictions ");
    ei_printf("(DSP: %d ms., Classification: %d ms., Anomaly: %d ms.)",
        result.timing.dsp, result.timing.classification, result.timing.anomaly);
    ei_printf(": \n");
    for (size_t ix = 0; ix < EI_CLASSIFIER_LABEL_COUNT; ix++) {
        ei_printf("    %s: %.5f\n", result.classification[ix].label, result.classification[ix].value);
    }
#if EI_CLASSIFIER_HAS_ANOMALY == 1
    ei_printf("    anomaly score: %.3f\n", result.anomaly);
#endif

  if (result.classification[1].value > confidence) {
      ei_printf("Driving");

      delay(2000);
   }
   
  if (result.classification[0].value > confidence) {
     
     ei_printf("Break");
     
     digitalWrite(ledPin, HIGH);
     delay(1000);
     digitalWrite(ledPin, LOW);
     delay(1000);
     digitalWrite(ledPin, HIGH);
     delay(1000);
     digitalWrite(ledPin, LOW);

     File dataFile = SD.open("report.csv", FILE_WRITE);
              
     dataString="Regular break detected with score " + String(result.classification[0].value)+" %";
      
      
      if (dataFile) {
                dataFile.println(dataString);
                dataFile.close();
              }
              // if the file isn't open, pop up an error:
              else {
                Serial.println("error opening file");
              }  
     
     delay(2000);
   }
 
   if (result.classification[2].value > confidence) {
       
       ei_printf("Hard break");
       
       digitalWrite(ledPin, HIGH);
       delay(10000);
       digitalWrite(ledPin, LOW);

       File dataFile = SD.open("report.csv", FILE_WRITE);
              
       dataString="Hard break detected with score " + String(result.classification[2].value)+" %";
      
      
      if (dataFile) {
                dataFile.println(dataString);
                dataFile.close();
              }
              // if the file isn't open, pop up an error:
              else {
                Serial.println("error opening file");
              }  
              
   }
 
}
