/* 
 * Xiao nrf52840 TinyML Tire care acquisition script
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
String compiledLines = "";

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

  randomSeed(analogRead(0));
  
  Serial.begin(115200);
  delay(3000);
  
  Serial.print("Tire care acquisition - Roni Bandini February 2023");

  pinMode(buttonPin, INPUT_PULLUP);

  // led
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
      compiledLines="";
      
      
      if (dataFile) {
                dataFile.println(dataString);
                dataFile.close();
              }
              // if the file isn't open, pop up an error:
              else {
                Serial.println("error opening file");
              }  
       
       isRecording=1;
       delay(500);
        
      }
     else{
      
      // dump to file
      Serial.println("Dump to file");
      File dataFile = SD.open(myFileName+".csv", FILE_WRITE);
        
      if (dataFile) {
          dataFile.println(compiledLines);
          dataFile.close();
        }
        // if the file isn't open, pop up an error:
        else {
          Serial.println("error opening datalog.txt");
        }          
        
      isRecording=0;
      compiledLines="";
      digitalWrite(ledPin, LOW);
      delay(5000);
              
      }


  }
  
  if (isRecording == 1) {

    digitalWrite(ledPin, HIGH);
        

    if (millis() > last_interval_ms + INTERVAL_MS) {
        last_interval_ms = millis();                         
        dataString=String(timestamp)+","+String(myIMU.readFloatGyroX() * CONVERT_G_TO_MS2,4)+","+String(myIMU.readFloatGyroY() * CONVERT_G_TO_MS2,4)+","+String(myIMU.readFloatGyroZ() * CONVERT_G_TO_MS2,4);
        Serial.println("Adding: "+dataString);        
        compiledLines=compiledLines+"\n"+dataString;     
        timestamp=timestamp+timestampIncrement;                           
   }

   
  } else {

    Serial.println("...");
}

delay(INTERVAL_MS);
 
    
  
}


 

                     
