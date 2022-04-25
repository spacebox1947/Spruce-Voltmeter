/*
 * Code based on a project by Shahariar
 * Find the original project and Code @
 * https://create.arduino.cc/projecthub/PSoC_Rocks/arduino-negative-voltmeter-993902
 * 
 * 
 * 
 * 
 * 
 * 
 * 
 * 
 */

// LIBRARIES
// i2c and u8g library for 1306 oled display
#include <Wire.h>
#include "U8glib.h"
#include <SD.h>
#include <SPI.h>

// CONSTANTS
// 25V Reader
#define R_LOW 100           // 100k or 10k resistor, 1%, smd1206, 1/10 watt
#define R_HIGH 5000         // 5M or 500k  resistor, 1%, smd1206, 1/10 watt

// 5V Reader
//#define R_LOW 500
//#define R_HIGH 5000

// 5V Reader with Small Bois
//#define R_LOW 100
//#define R_HIGH 1000

#define AREF 1.1            // Aref pin voltage 
#define ADC_RESOLUTION 1023 // 10 bit ADC
#define SAMPSPER 150        // samples per read of analog pin for value averaging

U8GLIB_SSD1306_128X32 u8g(U8G_I2C_OPT_NONE);     // I2C OLED  Display instance

// measuring analog ins; for use with measureChannel() function
int analogPins[] = {A1, A2, A3};
int aPinsLength = 3;
int SIGFIGS = 6;           // How many figs to print/save

// File for Filing
File myFile;

// keep track of number of lines save in SD file
int linesWritten = 0;
int linesPerSave = 100;

// Variables for volty maths
long half_Aref = 0; // adc 0 for reading half AREF = 550mv, should read 512
//int dump = 0;       // discarding first adc conversion (only used for half_Aref
float voltVals[3];  // calculating measured voltVals from ADC ch 1-3

// control if Serial is used for printing/debugging
bool printSerial = true;                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                          

//////////////////////////////////////////////////////////
void setup(void) {
  if (printSerial) {
    Serial.begin(9600);
    while(!Serial) {;}
  }

  if(!SD.begin(10)) { //10 is the pin our SD card reader is designed to speak to.
    if (printSerial) {
      Serial.println("Failure"); //sometimes fail message prints anyway
    }
    while(1);
  }
  if (printSerial) {
    Serial.println("Success! Moving on...");
  }

  
  /*
   * // lots of work for a file name
   * // stretch use: generate sequential filenames mid run in void loop()
   * if (String(fileNumber).length() < 4) {
   *   do cool stuff that replaces fileNameNumber code below
   * }
   */
  /*
  fileNameNumber = "000" + String(fileNumber);
  fileName = fileNamePrefix + fileNameNumber + fileNameSuffix;
  myFile = SD.open(fileName, FILE_WRITE);
  if (printSerial) {
    Serial.println("Opening SD file for writing of Data, Please: " + fileName);
  }
  */
  myFile = SD.open("Sample.txt", FILE_WRITE);
  if (printSerial) {
    Serial.print("Opening SD file for writing of Data, Please: ");
    Serial.println("Sample.txt");
  }
  delay(1000); // unnecessary debugging delay
  if (printSerial) {
    Serial.println("Moving On...");
  }

  // awesome code
  delay(500);                // delay for idk
  analogReference(INTERNAL); // set 1.1v internal reference
  delay(500);                // delay to stabilize aref voltage
  u8g.setRot180();           // change display orientation
}

//////////////////////////////////////////////////////////
void loop(void) {

  if (linesWritten < linesPerSave) {

    delay(5); // delay before using I2C OLED
    // refresh i2c oled with updated value
    u8g.firstPage();  
    do {
      draw();
    }
    while( u8g.nextPage() );
    delay(5);
  
    // clear variables old value
    half_Aref = 0;

    ////////////////////////////////////////////////////////////////////// 
    /// Half AREF measurement ///    
    int dump = analogRead(A0); // discard first adc convertion
    delay(10); // delay for channel switching stabilizaion
  
    // sum a number of samples
    for (int i=0; i<SAMPSPER; i++)
    {
      half_Aref = half_Aref+analogRead(A0);
    }
  
    // average the samples: we enjoy improved accuracy by doing this
    half_Aref = half_Aref/SAMPSPER;


    ////////////////////////////////////////////////////////////////////// 
    // measure each pin
    for (int i = 0; i < aPinsLength; i++) {
      voltVals[i] = measureChannel(analogPins[i], half_Aref);
    }

    // old volt measuring code died here, see FUNctions
    
    ////////////////////////////////////////////////////////////////////// 
    // Format string for Serial
    if (printSerial) {
      //Serial.println(String(voltVals[0]) + "\t" + String(voltVals[1]) + "\t" + String(voltVals[2]) + "\t" + String(half_Aref));
      Serial.print(voltVals[0], SIGFIGS);
      Serial.print("\t");
      Serial.print(voltVals[1], SIGFIGS);
      Serial.print("\t");
      Serial.print(voltVals[1], SIGFIGS);
      Serial.print("\t");
      Serial.println(half_Aref);
    }
  
    // Format string for saving on SD card
    if (myFile) {
      if (printSerial) {
        Serial.println("Attempting to Print to File");
      }
      //myFile.println(String(voltVals[0]) + ", " + String(voltVals[1]) + ", " + String(voltVals[2]) + ";");
      for (int i = 0; i < aPinsLength; i++) {
        if (i == aPinsLength - 1) {
          myFile.print(voltVals[i], SIGFIGS);
          myFile.println(";");
        }
        else {
          myFile.print(voltVals[i], SIGFIGS);
          myFile.print(", ");
        }
      }
    }
    else {
      if (printSerial) {
        Serial.println("Error sending data to SD card. Call for help. Get out the megaphones! Get out the alpenhorns! Requesting air raid sirens x5!");
      }
    }
    linesWritten += 1;
  }
  
  // save current sd file, re-open same file + wait a bit
  else if (linesWritten >= linesPerSave) {
    
    if (printSerial) {
      Serial.println("Flushing current set of samples ...");
    }
    
    //myFile.close(); //why not close?
    myFile.flush();
    linesWritten = 0;
    delay(250); // delay for SD card to be ready for more writin', typin', and measurin'
    if (printSerial) {
      Serial.println("Moving On ...");
    }
  }
  // hopefully this never executes
  else {}
}

//////////////////////////////////////////////////////////
/// Oled Loop Draw Function ///
void draw(void) {
  
  // set font for text
  u8g.setFont(u8g_font_5x8);
  u8g.drawStr( 0, 6, "Chan-1   Chan-2   Chan-3");
  
  // set font for number
  u8g.setFont(u8g_font_7x13B);
  
  // print voltages measured on OLED
  u8g.setPrintPos(0, 20);u8g.print(voltVals[0],1);
  u8g.setPrintPos(45, 20);u8g.print(voltVals[1],1);
  u8g.setPrintPos(90, 20);u8g.print(voltVals[1],1);
  
  // set font and print units
  u8g.setFont(u8g_font_5x8);
  u8g.drawStr( 0, 32, "volts    volts    volts");
}             

///////////////////////////////////////////////////////////////////////
// FUNctions!
///////////////////////////////////////////////////////////////////////

float measureChannel(int pin, long h_aref) {
 /* measure an analog pin (as defined in analogPins[])
  * return adjusted voltage value v, a float
  */

  long ch = 0;
  float v;
  
  int dump = analogRead(pin); // forget about that first read bay bee
  delay(10);
  for (int i = 0; i < SAMPSPER; i++) {
    ch = ch + analogRead(pin);
  }
  ch /= SAMPSPER;
  ch -= h_aref;
  v = ch *((R_LOW+R_HIGH)/R_LOW)*(AREF)/ADC_RESOLUTION;
  return v;
}
