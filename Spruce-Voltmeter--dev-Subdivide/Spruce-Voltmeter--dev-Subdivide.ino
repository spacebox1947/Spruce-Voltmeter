/*
 * Code based on a project by Shahariar
 * Find the original project and Code @
 * https://create.arduino.cc/projecthub/PSoC_Rocks/arduino-negative-voltmeter-993902
 * 
 * This branch 'subdivides' the measuring of channels, aims to show smaller steps between reads
 * AND shows an overall average. How does it do?
 * 
 * This dev branch focuses on seeing the 'inbetween' samples
 * subdivides our 150 averaged samples into 5 sets of 25 averaged samples
 * can be changed to have a variety of sample sets;
 * change SAMPSPER and INTERVAL to mess with this.
 * voltValsMatrix will need to be set to [3][(SAMPSPER/INTERVAL) + 1]
 * this is currently managed w/ the variables...
 * aPinsLenght = 3, voltMatrixWidth = 6
 * 
 */

// LIBRARIES
// i2c and u8g library for 1306 oled display
#include <Wire.h>
#include <U8glib.h>
#include <SD.h>
#include <SPI.h>

// CONSTANTS
// 25V Reader
#define R_LOW 100           // 100k or 10k resistor, 1%, smd1206, 1/10 watt
#define R_HIGH 5000         // 5M or 500k  resistor, 1%, smd1206, 1/10 watt

// 5V Reader with larger resistors
//#define R_LOW 500
//#define R_HIGH 5000

// 5V Reader with Small Bois resistors
//#define R_LOW 100
//#define R_HIGH 1000

#define AREF 1.1            // Aref pin voltage 
#define ADC_RESOLUTION 1023 // 10 bit ADC
#define SAMPSPER 150        // samples per read of analog pin for value averaging
#define INTERVAL 25         // subdivide those samps

//U8GLIB_SSD1306_128X32 u8g(U8G_I2C_OPT_NONE);     // I2C OLED  Display instance

// File for Filing
File myFile;
// use 5 switches on DIP to name a file in binary
// utilized in FUNction: buildFileNameBinary()
// pins do not neet to be set up for a filename to be made,
// but filename will probably be 00000 or 11111
int fileNamePins[] = {5, 6, 7, 8, 9};
String fileName = "";

// measuring analog ins; for use with measureChannel() function
int analogPins[] = {A1, A2, A3};
const int aPinsLength = 1;
const int SIGFIGS = 4;           // How many figs to print/save

// change this is you changed SAMPSPER or INTERVAL
// e.g 150 / 25 = 5  + 1 = voltMatrixWidth
const int voltMatrixWidth = 6; // subidivded samples + final sample
float voltValsMatrix[aPinsLength][voltMatrixWidth];

// Digitl Pin Control
// Use 3 DIP switches to manage writing of data from unused pins
// print/write "NULL" if current test lead is OFF
// NULL is wonderful
int controlPins[] = {2, 3, 4};

// keep track of number of lines save in SD file
int linesWritten = 0;
const int linesPerSave = 500;

// Variables for volty maths
long half_Aref = 0; // adc 0 for reading half AREF = 550mv, should read 512
//float voltVals[3];  // calculating measured voltVals from ADC ch 1-3

// control if Serial is used for printing/debugging
bool printSerial = true;      

// measure the length of one data cycle
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
  //fileName = "bestfrnd.txt";
  
  delay(500);

  if(!SD.begin(10)) { //10 is the pin our SD card reader is designed to speak to.
    if (printSerial) {
      Serial.println("Failure\tCheck SD card is inserted and wired properly ... unless it is not");
    }
    while(1);
  }
  //myFile = SD.open("NameIt.txt", FILE_WRITE);
  myFile = SD.open(fileName, FILE_WRITE);
  if (printSerial) {
    Serial.print("Opening SD file for writing of Data, Please: ");
    Serial.println(fileName);
  }

  delay(500);                // delay to make sure ADC is stable
  analogReference(INTERNAL); // set 1.1v internal reference
  delay(500);                // delay to stabilize aref voltage

  //u8g.setRot180();           // change display orientation
  
  currentMs = millis();      // start millis() timer
  lastMs = millis();
}

//////////////////////////////////////////////////////////
void loop() {
  if (linesWritten < linesPerSave) {

    delay(5); // delay before using I2C OLED
    // refresh i2c oled with updated value
    /*
    u8g.firstPage();  
    do {
      draw();
    }
    while( u8g.nextPage() );
    delay(5);
    */
    // clear variables old value
    half_Aref = 0;

    ////////////////////////////////////////////////////////////////////// 
    /// Half AREF measurement ///    
    int dump = analogRead(A0); // discard first adc convertion
    delay(10); // delay for channel switching stabilizaion
  
    // sum a number of samples
    for (int i=0; i<SAMPSPER; i++)
    {
      half_Aref += analogRead(A0);
    }
  
    // average the samples: we enjoy improved accuracy by doing this
    half_Aref = half_Aref/SAMPSPER;

    ////////////////////////////////////////////////////////////////////// 
    // measure pins at intervals!
    // this block assumes measureChannelWithIntervals() 
    //   read first aPin in list 25 times.
    //   append this average to voltValsMatrix
    //   each set of 25 samples is also added to v_acc
    //   when 125 samples have been averaged and appended;
    //   read 25 more samples, add to v_acc
    //   average v_acc by # of intervals
    //   append to voltValsMatrix
    for (int j = 0; j < aPinsLength; j++) {
      float v_acc = 0.0; // accumulate volatges for 150 samples
      int dump = analogRead(analogPins[j]); // forget about that first read bay bee
      delay(10);
      for (int i = 0; i < voltMatrixWidth; i++) {
        if (i == voltMatrixWidth-1) {
          v_acc += measureChannelWithIntervals(analogPins[j], INTERVAL);
          v_acc /= (i+1);
          voltValsMatrix[j][i] = v_acc; // 150 averaged samples to array!
        }
        else if (i < voltMatrixWidth-1) {
          voltValsMatrix[j][i] = measureChannelWithIntervals(analogPins[j], INTERVAL); // 25 averaged samples to array!
          v_acc += voltValsMatrix[j][i];
        }
      }
    }
    ////////////////////////////////////////////////////////////////////// 
    // print to Serial monitor
    if (printSerial) {
      formatMatrixSerialPrint(aPinsLength, voltMatrixWidth);
    }
    // save to SD card
    if (myFile) {
      formatMatrixSDPrint(aPinsLength, voltMatrixWidth);
    }
    else {
      if (printSerial) {
        Serial.println("Error sending data to SD card. Call for help.");
      }
    }
    
    checkMs();
    /*
    if (printSerial) {
      printMsSerial();
    }
    */
    printMsSD();
    linesWritten += 1;
  }
  
  // save current sd file, re-open same file + wait a bit
  else if (linesWritten >= linesPerSave) {
    //printMs();
    myFile.flush(); //ensure bytes are written to SD, preserve data with powerloss
    linesWritten = 0;
  }
}

//////////////////////////////////////////////////////////
/// Oled Loop Draw Function ///
/*
void draw() {
  
  // set font for text
  u8g.setFont(u8g_font_5x8);
  u8g.drawStr( 0, 6, "Chan-1   Chan-2   Chan-3");
  
  // set font for number
  u8g.setFont(u8g_font_7x13B);
  
  // print voltages measured on OLED
  u8g.setPrintPos(0, 20);u8g.print(voltVals[0],1);
  u8g.setPrintPos(45, 20);u8g.print(voltVals[1],1);
  u8g.setPrintPos(90, 20);u8g.print(voltVals[2],1);
  
  // set font and print units
  u8g.setFont(u8g_font_5x8);
  u8g.drawStr( 0, 32, "volts    volts    volts");
}
*/
///////////////////////////////////////////////////////////////////////
// FUNctions!
///////////////////////////////////////////////////////////////////////

void checkMs() {
  // check how long this void loop took
  // print it
  currentMs = millis();
  thisCycleMs = currentMs-lastMs;
  lastMs = currentMs;
}

void printMsSD() {
  myFile.print(thisCycleMs);
  myFile.println(";");
}

void printMsSerial() {
  Serial.print(thisCycleMs);
  Serial.println(";");
}

String buildFileNameBinary() {
  // collect digiPins and make a string!
  // change filename extension and impress your friends
  String s;
  for (int i = 0; i < 5; i++) {
    s += String(digitalRead(fileNamePins[i]));
  }
  s = s + ".txt";
  return s;
}

float measureChannel(int pin) {
  /* measure an analog pin (as defined in analogPins[])
  * return adjusted voltage value v, a float
  */
  float ch = 0;
  float v;
  
  int dump = analogRead(pin); // forget about that first read bay bee
  delay(10);
  for (int i = 0; i < SAMPSPER; i++) {
    ch += analogRead(pin);
  }
  ch /= float(SAMPSPER);
  ch -= half_Aref;
  v = ch * ((R_LOW+R_HIGH)/float(R_LOW))*((AREF)/float(ADC_RESOLUTION));
  return v;
}


float measureChannelWithIntervals(int pin, int interval) {
  float ch = 0;
  float v;
  for (int i = 0; i < interval; i++) {
    ch += analogRead(pin);
  }
  ch /= float(interval);
  ch -= half_Aref;
  v = ch * ((R_LOW+R_HIGH)/float(R_LOW)) * ((AREF)/float(ADC_RESOLUTION));
  return v;
}

void formatMatrixSerialPrint(int a, int b) {
  // serial print a matrix of a x b values
  for (int j = 0; j < a; j++) {
    // start with list @ index a
    Serial.print("Ch_");
    Serial.print(j+1);
    Serial.print(":\t");
    for (int i = 0; i < b; i++) {
      //print all values in that list
      Serial.print(voltValsMatrix[j][i], SIGFIGS);
      Serial.print(", ");
    }
    Serial.println();
  }
}

void formatMatrixSDPrint(int a, int b) {
  // serial print a matrix of a x b values
  for (int j = 0; j < a; j++) {
    // start with list @ index a
    myFile.print(j+1);
    myFile.print(",");
    for (int i = 0; i < b; i++) {
      //print all values in that list
      myFile.print(voltValsMatrix[j][i], SIGFIGS);
      if (i == b-1) {
        myFile.println(";");
      }
      else myFile.print(", ");
    }
    
  }
}


  
