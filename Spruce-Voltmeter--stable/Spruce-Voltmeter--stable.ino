/*
 * Code based on a project by Shahariar
 * Find the original project and Code @
 * https://create.arduino.cc/projecthub/PSoC_Rocks/arduino-negative-voltmeter-993902
 * 
 * ---- 30 May 2022 ----
 * - Working OLED! Prints read values + tuner pot + lines written to SD card
 *    - switches between Vin values and tuner pot values via __ drawMs __
 *    - change drawMs to slow/speed up display
 * - re-organized measuring Vin logic + NULL prints.
 *    - Disabling a lead via digitalRead now skips measuring its paired Vin.
 * - ATM stable is set to only read A1 and A2.\
 *    - Must change __ aPinsLength __ to 3 for all the Vins
 * - changed file extension to defaul of ".csv"
 *    - this can be changed with __ fileExt __
 * - several variables pushed to #define to save minimal program space. woo
 * 
 */

// LIBRARIES
// i2c and u8g library for 1306 oled display
#include <Wire.h>
#include <U8glib.h>
#include <SD.h>
#include <SPI.h>

// CONSTANTS
// 5V Reader with larger resistors
#define R_LOW 490.0         // measure BOTH resistors w/ voltmeter and adjust this value for better maths
#define R_HIGH 5000.0       // stretch: static measurements for all 6 resistors!
//float resist[] = {5000, 490, 5000, 500, 1000, 100};

#define AREF 1.1            // Aref pin voltage 
#define ADC_RESOLUTION 1023 // 10 bit ADC
#define SAMPSPER 150        // samples per read of analog pin for value averaging

#define nully "NULL"        // NULL string for unread Vins
#define SIGFIGS 4           // How many figs to print/save

U8GLIB_SSD1306_128X32 u8g(U8G_I2C_OPT_NONE);     // I2C OLED  Display instance

// File for Filing
File myFile;
// use 5 switches on DIP to name a file in binary
// utilized in FUNction: buildFileNameBinary()
// pins do not neet to be set up for a filename to be made,
// but filename will probably be 00000 or 11111
int fileNamePins[] = {5, 6, 7, 8, 9};
String fileName = "";
String fileExt = ".csv";

// measuring analog ins; for use with measureChannel() function
int analogPins[] = {A1, A2, A3};
#define aPinsLength 3

// Digitl Pin Control
// Use 3 DIP switches to manage writing of data from unused pins
// print/write "NULL" if current test lead is OFF
// NULL is wonderful
int controlPins[] = {2, 3, 4};

// keep track of number of lines save in SD file
int linesWritten = 0;
#define linesPerSave 250
unsigned int linesTotal = 0;

// Variables for volty maths
long half_Aref = 0;         // adc 0 for reading half AREF = 550mv, should read 512
float voltVals[3];          // calculating measured voltVals from ADC ch 1-3

// control if Serial is used for printing/debugging
bool printSerial = true;      

// measure the length of one data cycle
// not implemented for SD card flush loop. >:)
int currentMs;
int lastMs;
int thisCycleMs;

//////////////////////////////////////////////////////////
void setup() {
  if (printSerial) {
    Serial.begin(9600);
    while(!Serial) {;}
  }
  // generate 5 letter string from DIP switch on digiPins
  // assigned @ fileNamePins[] = {5, 6, 7, 8, 9};
  // these pins should be set BEFORE powering ard if you want a unique name
  fileName = buildFileNameBinary();
  //fileName = "bestfrnd.csv";    // custom file name here
  
  delay(500);

  if(!SD.begin(10)) { //10 is the pin our SD card reader is designed to speak to.
    if (printSerial) {
      Serial.println("Failure\tCheck SD card is inserted and wired");
    }
    while(1);
  }
  
  myFile = SD.open(fileName, FILE_WRITE);
  if (printSerial) {
    Serial.print("Opening SD file for writing of Data, Please: ");
    Serial.println(fileName);
  }

  delay(500);                // delay to make sure ADC is stable
  analogReference(INTERNAL); // set 1.1v internal reference
  delay(500);                // delay to stabilize aref voltage

  
  u8g.setRot180();           // change display orientation
  
  currentMs = millis();      // start millis() timer
  lastMs = millis();
}

//////////////////////////////////////////////////////////
void loop() {
  if (linesWritten < linesPerSave) {
    doDraw();                   // screen time

    ////////////////////////////////////////////////////////////////////// 
    /// Half AREF measurement ///  
    // clear variable's old value
    half_Aref = 0;
    int dump = analogRead(A0); // discard first adc convertion
    delay(5); // delay for channel switching stabilizaion
  
    // sum a number of samples
    for (int i=0; i < SAMPSPER; i++)
    {
      half_Aref += analogRead(A0);
    }
  
    // average the samples: we enjoy improved accuracy by doing this
    half_Aref /= float(SAMPSPER);

    ////////////////////////////////////////////////////////////////////// 
    
    for (int i = 0; i < aPinsLength; i++) {             // measure each pin
      if (digitalRead(controlPins[i])) {                // skip read if DIP LOW
        voltVals[i] = measureChannel(analogPins[i]);    
      }
    }
    
    ////////////////////////////////////////////////////////////////////// 
    // Format string for Serial
    if (printSerial) {
      for (int i = 0; i < aPinsLength; i++) {
        if (i == aPinsLength - 1) {
          if (digitalRead(controlPins[i])) {
          //if (true) {
            Serial.print(voltVals[i], SIGFIGS);
          }
          else {
            Serial.print(nully);
          }
          Serial.print("\t");
        // end w/ beatiful half_Aref value
        Serial.println(half_Aref);
        }
        else {
          if (digitalRead(controlPins[i])) {
          //if (true) {
            Serial.print(voltVals[i], SIGFIGS);
          }
          else {
            Serial.print(nully);
          }
          Serial.print("\t\t");
        }
      }
    }
  
    // Format string for saving on SD card
    if (myFile) {
      // print to myFile
      for (int i = 0; i < aPinsLength; i++) {
        if (i == aPinsLength - 1) {
          if (digitalRead(controlPins[i])) {
          //if (true) {
            myFile.print(voltVals[i], SIGFIGS);
          }
          else {
            myFile.print(nully);
          }
          //myFile.print(";");
          myFile.print(", ");
        }
        else {
          if (digitalRead(controlPins[i])) {
          //if (true) {
            myFile.print(voltVals[i], SIGFIGS);
          }
          else {
            myFile.print(nully);
          }
          myFile.print(", ");
        }
      }
    }
    else {
      if (printSerial) {
        Serial.println("Error sending data to SD card. Call for help.");
      }
    }
    printMs();
    linesWritten += 1;
  }
  
  // save current sd file, re-open same file + wait a bit
  else if (linesWritten >= linesPerSave) {
    //doDraw();                 // ** Does this call need to be here? TEST IT **
    myFile.flush();             //ensure bytes are written to SD, preserve data with powerloss
    linesTotal += linesPerSave;
    linesWritten = 0;
  }
}

//////////////////////////////////////////////////////////
/// Oled Loop Draw Function ///

void draw() {
  if (digitalRead(9)) {       // digitalPin(9), last pin of DIP switch, controls display mode
    // set font for text
    u8g.setFont(u8g_font_5x8);
    u8g.drawStr(0, 6, "Chan-1   Chan-2   Chan-3");
    
    // set font for number
    u8g.setFont(u8g_font_7x13B);
    
    // print voltages measured on OLED 
    for (int i = 0; i < aPinsLength; i++) {
      if (digitalRead(controlPins[i])) {
        u8g.setPrintPos(i*45, 20);u8g.print(voltVals[i], 2);
      }
      else {
        u8g.setPrintPos(i*45, 20);u8g.print(nully);
      }
    }
    // set font and print units
    u8g.setFont(u8g_font_5x8);
    u8g.drawStr(0, 32, "volts    volts    volts");
  }
  else {
    // use same design language to print half_Aref and samps written
    u8g.setFont(u8g_font_5x8);
    u8g.drawStr(0, 6, "1.1v Pot   Lines Written");

    u8g.setFont(u8g_font_7x13B);
    u8g.setPrintPos(0, 20);u8g.print(half_Aref, 1);
    u8g.setPrintPos(45, 20);u8g.print(linesTotal, 1);

    u8g.setFont(u8g_font_5x8);
    u8g.drawStr(0, 32, "ADC      flushed! :D");
  }
}

///////////////////////////////////////////////////////////////////////
// FUNctions!
///////////////////////////////////////////////////////////////////////

void printMs() {
  // check how long this void loop took
  // print it
  // ** THIS FUNCTION has the PRINTLN for SD DATA**
  currentMs = millis();
  thisCycleMs = currentMs - lastMs;
  myFile.print(thisCycleMs);
  myFile.println(";");
  lastMs = currentMs;
}

String buildFileNameBinary() {
  // collect digiPins and make a string!
  // change filename extension and impress your friends
  String s;
  for (int i = 0; i < 5; i++) {
    s += String(digitalRead(fileNamePins[i]));
  }
  s += fileExt;
  return s;
}

float measureChannel(int pin) {
 /* measure an analog pin (as defined in analogPins[])
  * return adjusted voltage value v, a float
  */
  double ch = 0;
  int dump = analogRead(pin); // forget about that first read bay bee
  delay(5);
  
  for (int i = 0; i < SAMPSPER; i++) {
    ch += analogRead(pin);
  }
  ch /= SAMPSPER;
  ch -= half_Aref;
  ch *= ((R_LOW + R_HIGH) / R_LOW) * ((AREF) / float(ADC_RESOLUTION));
  return ch;
}

void doDraw() {
  delay(5); // delay before using I2C OLED
  // refresh i2c oled with updated value
  u8g.firstPage();  
  do {
    draw();
  }
  while( u8g.nextPage() );
}
