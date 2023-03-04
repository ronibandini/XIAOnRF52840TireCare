/* 
 * Xiao nrf52840 TinyML Tire care Data acquisition script
 * Roni Bandini March 2023 @RoniBandini
 * Machine Learning via Edge Impulse
 * Connect button to GND and D1. Connect Led to GND and D2. Connect microSD to GND, VCC, MISO pin to D9, MOSI to D10, SCK to D8, CS to D3.
 */

#include <Arduino.h>
#include <U8x8lib.h>
#include <Wire.h>
#include "LSM6DS3.h"
#include <SPI.h>
#include <SD.h>

#define MAX_UID 8 

const char * generateFileName(){
  const char possible[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
  static char uid[MAX_UID + 1];
  for(int p = 0, i = 0; i < MAX_UID; i++){
    int r = random(0, strlen(possible));
    uid[p++] = possible[r];
  }
  uid[MAX_UID] = '\0';
  return uid;
}

File myFile;
const int buttonPin = 1;
const int ledPin=2;

int timestamp=0;

int isRecording=0;
String myFileName="";

String dataString = "";
String compiledLines="";

int myCounter=0;

// Oled
//U8X8_SSD1306_128X64_NONAME_HW_I2C u8x8(/* clock=*/ PIN_WIRE_SCL, /* data=*/ PIN_WIRE_SDA, /* reset=*/ U8X8_PIN_NONE);  

//Create a instance of class LSM6DS3
LSM6DS3 myIMU(I2C_MODE, 0x6A);    //I2C device address 0x6A
 
 
#define CONVERT_G_TO_MS2    9.80665f
#define FREQUENCY_HZ        50
#define INTERVAL_MS         (1000 / (FREQUENCY_HZ + 1))

int timestampIncrement=20;
 
static unsigned long last_interval_ms = 0;
 
 
void setup(void) {

  Serial.begin(115200);
  delay(3000);
  
  Serial.print("Tire care - Roni Bandini Febr 2023");

  pinMode(buttonPin, INPUT_PULLUP);

  // on board led 
  pinMode(ledPin, OUTPUT);  
  digitalWrite(ledPin, HIGH);   
  delay(1000);                       
  digitalWrite(ledPin, LOW);    
  delay(3000);  

  Serial.println("Initializing SD card...");

  if (!SD.begin(3)) {
    Serial.println("initialization failed. Things to check:");
    Serial.println("1. is a card inserted?");
    Serial.println("2. is your wiring correct?");
    Serial.println("3. did you change the chipSelect pin to match your shield or module?");
    Serial.println("Note: press reset button on the board and reopen this Serial Monitor after fixing your issue!");
    while (true);
  }

  Serial.println("initialization done.");

  /*
  u8x8.begin();
  u8x8.setFlipMode(1);   // set number from 1 to 3, the screen word will rotary 180

  u8x8.setFont(u8x8_font_chroma48medium8_r);
  u8x8.setCursor(0, 0);   
  u8x8.println("Starting...");
  */
  
  if (myIMU.begin() != 0) {        
        Serial.println("IMU error");
        delay(5000);
    } else {
        Serial.println("IMU Ok");
  }  
    
}
 
void loop(void) {
    

  int buttonState = digitalRead(buttonPin);

  if (buttonState==LOW){
    // button pressed
    if (isRecording==0){            

      myFileName=generateFileName();
      
      // save first line

      Serial.println("File name "+myFileName);
      File dataFile = SD.open(myFileName+".csv", FILE_WRITE);
              
      dataString="timestamp,x,y,z";
      
      
      if (dataFile) {
                dataFile.println(dataString);
                dataFile.close();
              }
              // if the file isn't open, pop up an error:
              else {
                Serial.println("error opening file");
              }  
       
       isRecording=1;
        
      }
     else{
      isRecording=0;

      delay(2000);
              
      }


  }
  
  if (isRecording == 1) {

    Serial.println("Storing...");
    digitalWrite(ledPin, HIGH);
        

     if (millis() > last_interval_ms + INTERVAL_MS) {
        last_interval_ms = millis();

        /*
        u8x8.setCursor(0, 0);
        u8x8.println("Accelerometer:");
        u8x8.println(String(myIMU.readFloatGyroX()* CONVERT_G_TO_MS2,4));
        u8x8.println(String(myIMU.readFloatGyroY()* CONVERT_G_TO_MS2,4));
        u8x8.println(String(myIMU.readFloatGyroZ()* CONVERT_G_TO_MS2,4));
        */        

        
        timestamp=timestamp+timestampIncrement;

        myCounter++;

        if (myCounter==49){

        // save compilation to csv
        File dataFile = SD.open(myFileName+".csv", FILE_WRITE);               
        
        if (dataFile) {
          dataFile.println(compiledLines);
          dataFile.close();
        }
        // if the file isn't open, pop up an error:
        else {
          Serial.println("error opening file");
        }
        compiledLines="";
        myCounter=0
                    
        }// counter limit
        else{
          // add to compiled
          dataString=String(timestamp)+","+String(myIMU.readFloatGyroX() * CONVERT_G_TO_MS2,4)+","+String(myIMU.readFloatGyroY() * CONVERT_G_TO_MS2,4)+","+String(myIMU.readFloatGyroZ() * CONVERT_G_TO_MS2,4);
          Serial.println(dataString);
          compiledLines=compiledLines+"/n"+dataString;          
        }
   }

   digitalWrite(ledPin, LOW);
   
  } else {

    Serial.println("...");
    delay(INTERVAL_MS);
}
  
 
    
  
}


 

                     
