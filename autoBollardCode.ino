// Auto-Bollard 2018
// GPL v3
// SUBC: the UBC Submarine Design Team

#include "HX711.h"
#include <SD.h>
#include <SPI.h>

// Load cell calibration
#define calibration_factor -10860.0 //calibrated with 750g test load

// Define which parts are on
#define LOADCELLON false
#define TACHOON true
#define SDON true

// Change when plugged in to Arduino
#define tachoPin 2
#define BUTTONPIN 3
#define INDICATORPIN 9
#define DOUT  5
#define CLK  6

//#define BEFORECALC 4
#define MEASUREDELAY 500
#define CHANGEPERREV 2
#define DELAYTIME 5
#define DEBOUNCETIME 250

// Load cell object
HX711 scale;

volatile int changes=0;
unsigned long startTime;
unsigned long endTime;
unsigned long recordTime;
int duration;
int oldFall=0;
boolean running=false;
//boolean oldRunning=false;
boolean initialized=false;

/*// Debounce stuff
int buttonState;             // the current reading from the input pin
int lastButtonState = LOW;   // the previous reading from the input pin
// the following variables are unsigned longs because the time, measured in
// milliseconds, will quickly become a bigger number than can be stored in an int.
unsigned long lastDebounceTime = 0;  // the last time the output pin was toggled
unsigned long debounceDelay = 50;    // the debounce time; increase if the output flickers*/

File outputFile;

void buttonPress(void);
void tachoChange(void);

void setup() {
  Serial.begin(9600);
  while (!Serial){
    // Wait for Serial to connect
  }
  if (SDON==true){
    Serial.print("Initializing SD card...");
  
    if (!SD.begin(4)) {
      Serial.println("initialization failed!");
      while (1);
    }
    Serial.println("initialization done.");
  }
  // Button stuff
  pinMode(BUTTONPIN, INPUT);
  attachInterrupt(digitalPinToInterrupt(BUTTONPIN), buttonPress, FALLING);
  if (TACHOON==true){
    attachInterrupt(digitalPinToInterrupt(tachoPin), tachoChange, CHANGE);
  }

  pinMode(INDICATORPIN, OUTPUT);
  digitalWrite(INDICATORPIN, LOW);

  // Load cell initialization
  if (LOADCELLON==true){
    scale.begin(DOUT, CLK);
    scale.set_scale(calibration_factor); 
    scale.tare();  //Assuming there is no weight on the scale at start up, reset the scale to 0
  }
}

void loop() {
  if(running==true&&initialized==true&&millis()-startTime>MEASUREDELAY){
    endTime=millis();

    Serial.print("Time: ");
    duration=(endTime-recordTime);
    if (SDON==true){
      outputFile.print(duration);
    }
    Serial.print(duration);
    
    if (TACHOON==true){
      double RPM = (60000.0*changes/CHANGEPERREV)/(duration);

      if (SDON==true){
        outputFile.print(",");
        outputFile.print(RPM);
      }
      
      Serial.print("    RPM: ");
      Serial.print(RPM);
    }    

    if (LOADCELLON==true){
      Serial.print(scale.get_units(), 2); //"scale.get_units(), n" returns a float with n sigfigs
      Serial.print(" N"); //Units can change but you'll need to refactor the calibration_factor

      if (SDON==true){
        outputFile.print(",");
        outputFile.print(scale.get_units());
      }
    }

    Serial.println("");
    if (SDON==true){
      outputFile.println("");
    }
    
    changes = 0;
    startTime=millis();
  } else {
    delay(DELAYTIME);
  }
}
  

void buttonPress(void){
  Serial.println("Button falling!");
  if (millis()-oldFall>DEBOUNCETIME){
    oldFall=millis();
    if (running==true&&initialized==true){
      if (SDON==true){
        // Stuff to close file, etc.
        outputFile.close();
        Serial.println("Closing file...");
      }
      initialized=false;
      running=false;
      digitalWrite(INDICATORPIN, LOW);
      delay(100);
    } else if (running==false&&initialized==false){
      if (SDON==true){
        // Stuff to open file, etc.
        // create a new file
        char filename[] = "DATA00.CSV";
        for (uint8_t i = 0; i < 100; i++) {
          filename[4] = i/10 + '0';
          filename[5] = i%10 + '0';
          if (! SD.exists(filename)) {
            // only open a new file if it doesn't exist
            outputFile = SD.open(filename, FILE_WRITE);
            break;  // leave the loop!
          }
        }
        outputFile.print("Time(ms)");
        if (TACHOON==true){
          outputFile.print(",RPM");
        }
        if (LOADCELLON==true){
          outputFile.print(",Force(N)");
        }
        outputFile.println("");
        Serial.print(filename);
        Serial.println(" successfully initialized!");
      }
      digitalWrite(INDICATORPIN, HIGH);
      initialized=true;
      running=true;
  
      changes=0;
      startTime=millis();
      recordTime=startTime;
    } else {
      if (SDON==true){
        Serial.println("Something went wrong. Trying to close file.");
        outputFile.close();
      }
    }
  } else {
    Serial.println("Not enough time has passed since the last event. Debouncing.");
  }
}

void tachoChange(void) {
  changes++;
}
