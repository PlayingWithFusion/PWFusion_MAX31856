/***************************************************************************
* File Name: SEN30007_8_DodecaMAX31856_Logger_example.ino
* Processor/Platform: Arduino Mega 2560 R3 (tested)
* Development Environment: Arduino 1.8.3
*
* Designed for use with with Playing With Fusion MAX31856 thermocouple
* breakout boards: SEN-30007 (any TC type) or SEN-30008 (any TC type)
*
* Copyright © 2015-18 Playing With Fusion, Inc.
* SOFTWARE LICENSE AGREEMENT: This code is released under the MIT License.
*
* Permission is hereby granted, free of charge, to any person obtaining a
* copy of this software and associated documentation files (the "Software"),
* to deal in the Software without restriction, including without limitation
* the rights to use, copy, modify, merge, publish, distribute, sublicense,
* and/or sell copies of the Software, and to permit persons to whom the
* Software is furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included in
* all copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
* FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
* DEALINGS IN THE SOFTWARE.
* **************************************************************************
* REVISION HISTORY:
* Author		Date	    Comments
* J. Steinlage		2018Jul01   Copied base from Quad example, expanded
*
* Playing With Fusion, Inc. invests time and resources developing open-source
* code. Please support Playing With Fusion and continued open-source
* development by buying products from Playing With Fusion!
* **************************************************************************
* ADDITIONAL NOTES:
* This file contains functions to initialize and run an Arduino Uno R3 in
* order to communicate with a MAX31856 single channel thermocouple breakout
* board. Funcionality is as described below:
*	- Configure Arduino to broadcast results via UART
*       - call PWF library to configure and read MAX31856 IC (SEN-30005, any type)
*	- Broadcast results to COM port
*  Circuit:
*    Arduino Uno   Arduino Mega  -->  SEN-30007/8
*    DIO pin 10      DIO pin 10  -->  CS0
*    DIO pin  9      DIO pin  9  -->  CS1
*    DIO pin  8      DIO pin  8  -->  CS2
*    DIO pin  7      DIO pin  7  -->  CS3
*    DIO pin  6      DIO pin  6  -->  IFB-11001 / SD Card Detect
*    DIO pin  5      DIO pin  5  -->  IFB-11001 / SD Chip Select
*    DIO pin  4      DIO pin  4  -->  CS4
*    DIO pin  3      DIO pin  3  -->  CS5
*    AN  pin  5      AN  pin  5  -->  CS6
*    AN  pin  4      AN  pin  4  -->  CS7
*    AN  pin  3      AN  pin  3  -->  CS8
*    AN  pin  2      AN  pin  2  -->  CS9
*    AN  pin  1      AN  pin  1  -->  CS10
*    AN  pin  0      AN  pin  0  -->  CS11
*    MOSI: pin 11  MOSI: pin 51  -->  SDI (must not be changed for hardware SPI)
*    MISO: pin 12  MISO: pin 50  -->  SDO (must not be changed for hardware SPI)
*    SCK:  pin 13  SCK:  pin 52  -->  SCLK (must not be changed for hardware SPI)
*    GND           GND           -->  GND
*    5V            5V            -->  Vin (supply with same voltage as Arduino I/O, 5V)
*     NOT CONNECTED              --> 3.3V (this is 3.3V output from on-board LDO. DO NOT POWER THIS PIN!
* It is worth noting that the 0-ohm resistors on the PCB can be removed to 
* free-up DIO pins for use with other shields if the 'Data Ready' funcionality
* isn't being used.
***************************************************************************/
#include <PWFusion_MAX31856.h>

// Stuff needed for SD + RTC Shield
#include "Wire.h"
#include "SD.h"
#include "RTClib.h"     // must be PWFusion version - includes MCP7941X chip on IFB-11001
#include "string.h"

// First things first... declare timing interval (time between logged samples)
// Settings is a number of 20ms loops... logger_interval = time(seconds) / 0.020
// minimum config should be 20 (0.2 seconds) for the MAX31856. Maximum timer setting 
// available is 655.65 seconds (65535 = 655.35 seconds)
uint16_t logger_interval = 500;    // setting of 500 logs one sample every 5 seconds

// File name
String file_name = "dlx5.csv";


// SD / RTC Logger shield pin definitions
uint8_t CS_SD   =  5;
uint8_t CD_SD   =  6;

// RTC interface
RTC_MCP79410 rtc;

#define NUM_THERMOCOUPLES   (sizeof(tcChipSelects) / sizeof(uint8_t))

uint8_t tcChipSelects[] = {10, 9, 8, 7, 4, 3, A4, A5, A0, A1, A2, A3};  // define chip select pins for each thermocouple
MAX31856  thermocouples[NUM_THERMOCOUPLES];


void setup()
{
  // Give the MAX31856 a chance to stabilize
  delay(1000);  

  Serial.begin(115200);  // set baudrate of serial port
  Serial.println(F("Playing With Fusion: Dodecal MAX31856 Logger"));
  Serial.println(F("With RTC. SEN-30007 + IFB-11001"));

  // Initialize each MAX31856... options can be seen in the PWFusion_MAX31856.h file
  for (int i=0; i<NUM_THERMOCOUPLES; i++)
  {
    thermocouples[i].begin(tcChipSelects[i]);
    thermocouples[i].config(K_TYPE, CUTOFF_60HZ, AVG_SEL_1SAMP, CMODE_AUTO);
  }

  /**********    REAL-TIME CLOCK SETUP    **********/
  if (! rtc.begin()) {
    Serial.println("Couldn't find RTC, halting");
    while (1);  // No RTC, stop. Could light LED here to indicate failure
  }

  // Check to see if RTC is running. If not, set the date to settings below. If it is, assume
  // that it is set correctly, and doesn't need to be adjusted. REMOVE THIS CHECK IF YOU WANT 
  // TO OVERRIDE THE CURRENT RTC SETTINGS!
  if (! rtc.isrunning()) {
    Serial.println("Config RTC");
    // following line sets the RTC to the date & time this sketch was compiled
    //                    Y   M  D   H   M   S    enable battery backup
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)), MCP7941X_BATT_BKUP_EN);
    // To set the RTC with an explicit date & time, see below
    // For May 31, 2016 at 1:23:16 : 
    //                      Y   M  D   H   M   S    enable battery backup
    //rtc.adjust(DateTime(2016, 5, 31, 1, 23, 16), MCP7941X_BATT_BKUP_EN);
  }
  else{Serial.println("RTC running, no time adjust");}
  
  /**********    SD CARD SETUP    **********/
  SPI.setDataMode(SPI_MODE0);             // SD card is a Mode 0 device
  delay(100);
  Serial.print("SD card Init...");
  // see if the card is present and can be initialized. Check both SD init and Card Detect
  if (!SD.begin(CS_SD)) {
    Serial.println("Card init failed");
    // don't do anything more:
    while (1);  // initialization failed, stop. Could light LED here to indicate failure
  }
  Serial.println("Begin log file");
  
  // Build data string to write to log file
  String dataString = "Time,TC0,TC1,TC2,TC3,TC4,TC5,TC6,TC7,TC8,TC9,TC10,TC11";
  String dataString2 = "TC Temp Scaling, 0.0078125C/bit";
  // Log new data to SD card
  // open the file. note that only one file can be open at a time,
  // so you have to close this one before opening another.
  
  File dataFile = SD.open(file_name, FILE_WRITE);
  
  // if the file is available, write to it:
  if (dataFile) {
    dataFile.println(dataString);
    dataFile.println(dataString2);
    // print to the serial port too:
    Serial.println(dataString);
    Serial.println(dataString2);
  }
  // if the file isn't open, pop up an error:
  else {
    Serial.println("err opening file");
    dataFile.close();
    while(1);       // file couldn't be opened, stop. Could light LED here to indicate failure
  }
  pinMode(5, OUTPUT);
  digitalWrite(5,HIGH);
  // need to clock out 8 bits so the SD card will release the MISO line
  SPI.transfer(0xAA);
  // timer interrupt setup
  timer_interval_config();
  
}

// timer variables (_tick is cleared by by ISR, count_main keeps track of timing interval)
volatile uint8_t _tick = 0;
volatile uint16_t count_main = 0;

void loop()
{
  // wait for next timer event to trigger (20mS timer interval expected)
  while(_tick);
  // start by re-setting tick
  _tick = 1;
  
  if(count_main < logger_interval) // next main step not met, increment count and wait
  {
    count_main++;
  }
  else // next step met, run code
  {
    // start by clearing the count
    count_main = 0;

    // switch back to Mode 0 for SD card
    SPI.setDataMode(SPI_MODE0);             // SD card is a Mode 0 device

    // Get current time
    DateTime now = rtc.now();
 
    // Build data string to write to log file
    // format of string:
    // Timestamp,TC0_Temp,TC1,TC2,TC3,TC4,TC5,TC6,TC7,TC8,TC9,TC10,TC11
    String dataString = "";

    // add current time to datastring
    dataString += String(now.year());
    dataString += "/";
    dataString += String(now.month());
    dataString += "/";
    dataString += String(now.day());
    dataString += " ";
    dataString += String(now.hour());
    dataString += ":";
    dataString += String(now.minute());
    dataString += ":";
    dataString += String(now.second());
    dataString += ",";
    
    // add Thermocouple data to dataString
    for (int i=0; i<NUM_THERMOCOUPLES; i++)
    {
      // Get latest measurement from MAX31856 channels
      thermocouples[i].sample();
      dataString += String(thermocouples[i].getTemperature());
      dataString += ",";

      // Print information to serial port
      //print31856Results(i, thermocouples[i]);

      // Attempt to recove misconfigured channels
      if(thermocouples[i].getStatus() == 0xFF)
      {
        thermocouples[i].config(K_TYPE, CUTOFF_60HZ, AVG_SEL_1SAMP, CMODE_AUTO);
        Serial.print(F("re-attempt config on TC"));
        Serial.println(i);
      }
    }

    // Log new data to SD card
    // open the file. note that only one file can be open at a time,
    // so you have to close this one before opening another.
    File dataFile = SD.open(file_name, FILE_WRITE);
  
    // if the file is available, write to it:
    if (dataFile) {
      dataFile.println(dataString);
      dataFile.close();
      // print to the serial port too:
      Serial.println(dataString);
    }
    // if the file isn't open, pop up an error:
    else {
      Serial.println("error opening file");
      //while(1);       // uncomment this line to stop execution if file couldn't be opened
    }
    // Get SD card to release data line.... write 0xAA to SPI
    digitalWrite(CS_SD,HIGH);
    SPI.transfer(0xAA);  
  }
}

void print31856Results(uint8_t channel, MAX31856 &tc)
{
  uint8_t status = tc.getStatus();

  Serial.print("Thermocouple ");
  Serial.print(channel);

  if(status)
  {
    // lots of faults possible at once, technically... handle all 8 of them
    // Faults detected can be masked, please refer to library file to enable faults you want represented
    Serial.print(F(": FAULTED - "));
    if(TC_FAULT_OPEN & status)        { Serial.print(F("OPEN, ")); }
    if(TC_FAULT_VOLTAGE_OOR & status) { Serial.print(F("Overvolt/Undervolt, ")); }
    if(TC_FAULT_TC_TEMP_LOW & status) { Serial.print(F("TC Low, ")); }
    if(TC_FAULT_TC_TEMP_HIGH & status){ Serial.print(F("TC High, ")); }
    if(TC_FAULT_CJ_TEMP_LOW & status) { Serial.print(F("CJ Low, ")); }
    if(TC_FAULT_CJ_TEMP_HIGH & status){ Serial.print(F("CJ High, ")); }
    if(TC_FAULT_TC_OOR & status)      { Serial.print(F("TC Range, ")); }
    if(TC_FAULT_CJ_OOR & status)      { Serial.print(F("CJ Range, ")); }
    Serial.println();
  }
  else  // no fault, print temperature data
  {
    Serial.println(F(": Good"));
    
    // MAX31856 External (thermocouple) Temp
    Serial.print(F("TC Temp = "));                   // print TC temp heading
    Serial.println(tc.getTemperature());
  }

  // MAX31856 Internal Temp
  Serial.print(F("Tint = "));
  float cjTemp = tc.getColdJunctionTemperature();
  if ((cjTemp > -100) && (cjTemp < 150))
  {
    Serial.println(cjTemp);
  }
  else
  {
    Serial.println(F("Unknown fault with cold junction measurement"));
  }
  Serial.println();
}

void timer_interval_config(void)
{
  cli();
  //set timer2 interrupt at 100Hz (0.01 sec)
  TCCR2A = 0;// set entire TCCR2A register to 0
  TCCR2B = 0;// same for TCCR2B
  TCNT2  = 0;//initialize counter value to 0
  // set compare match register for 8khz increments
  OCR2A = 155;// = (16*10^6) / (100*1024) - 1 (must be <256)
  // turn on CTC mode
  TCCR2A |= (1 << WGM01);
  // Set CS20 abd CS22 bits for 1024 prescaler
  TCCR2B |= (1 << CS22) | (1 << CS21) | (1 << CS20);
  // enable timer compare interrupt
  TIMSK2 |= (1 << OCIE2A);
  sei();//allow interrupts for timer to work
}

// Interrupt handler for TIMER2 (base 'tick' handler)
ISR(TIMER2_COMPA_vect){
   //the only thing we do here is clear the 'tick' variable
  _tick = 0;
}
