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
#include "PlayingWithFusion_MAX31856.h"
#include "PlayingWithFusion_MAX31856_STRUCT.h"
#include "SPI.h"

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

// MAX31856 shield pin definitions
uint8_t TC0_CS  =  7;
uint8_t TC1_CS  =  8;
uint8_t TC2_CS  =  9;
uint8_t TC3_CS  = 10;
uint8_t TC4_CS  =  4;
uint8_t TC5_CS  =  3;
uint8_t TC6_CS  =  A4;  // A4
uint8_t TC7_CS  =  A5;  // A5
uint8_t TC8_CS  =  A0;  // A0
uint8_t TC9_CS  =  A1;  // A1
uint8_t TC10_CS  = A2;  // A2
uint8_t TC11_CS  = A3;  // A3

// SD / RTC Logger shield pin definitions
uint8_t CS_SD   =  5;
uint8_t CD_SD   =  6;

PWF_MAX31856  thermocouple0(TC0_CS);
PWF_MAX31856  thermocouple1(TC1_CS);
PWF_MAX31856  thermocouple2(TC2_CS);
PWF_MAX31856  thermocouple3(TC3_CS);
PWF_MAX31856  thermocouple4(TC4_CS);
PWF_MAX31856  thermocouple5(TC5_CS);
PWF_MAX31856  thermocouple6(TC6_CS);
PWF_MAX31856  thermocouple7(TC7_CS);
PWF_MAX31856  thermocouple8(TC8_CS);
PWF_MAX31856  thermocouple9(TC9_CS);
PWF_MAX31856  thermocouple10(TC10_CS);
PWF_MAX31856  thermocouple11(TC11_CS);
struct var_max31856 TC_CH0, TC_CH1, TC_CH2, TC_CH3, TC_CH4, TC_CH5;
struct var_max31856 TC_CH6, TC_CH7, TC_CH8, TC_CH9, TC_CH10, TC_CH11;
// proto for display results function
void print31856Results(struct var_max31856 *tc_ptr);

// RTC interface
RTC_MCP79410 rtc;

void setup()
{
  delay(1000);                            // give chip a chance to stabilize
  Serial.begin(115200);                   // set baudrate of serial port
  Serial.println("Playing With Fusion: Dodecal MAX31856 Logger");
  Serial.println("With RTC. SEN-30007 + IFB-11001");

  // setup for the the SPI library:
  SPI.begin();                            // begin SPI
  SPI.setClockDivider(SPI_CLOCK_DIV32);   // SPI speed to SPI_CLOCK_DIV16 (1MHz)
  SPI.setDataMode(SPI_MODE1);             // MAX31856 is a MODE1 or 3 device
  
  /**********    THERMOCOUPLE CHANNEL SETUP    **********/
  // call config command... options can be seen in the PlayingWithFusion_MAX31856.h file
  thermocouple0.MAX31856_config(K_TYPE, CUTOFF_60HZ, AVG_SEL_1SAMP, CMODE_AUTO);
  thermocouple1.MAX31856_config(K_TYPE, CUTOFF_60HZ, AVG_SEL_1SAMP, CMODE_AUTO);
  thermocouple2.MAX31856_config(K_TYPE, CUTOFF_60HZ, AVG_SEL_1SAMP, CMODE_AUTO);
  thermocouple3.MAX31856_config(K_TYPE, CUTOFF_60HZ, AVG_SEL_1SAMP, CMODE_AUTO);
  thermocouple4.MAX31856_config(K_TYPE, CUTOFF_60HZ, AVG_SEL_1SAMP, CMODE_AUTO);
  thermocouple5.MAX31856_config(K_TYPE, CUTOFF_60HZ, AVG_SEL_1SAMP, CMODE_AUTO);
  thermocouple6.MAX31856_config(K_TYPE, CUTOFF_60HZ, AVG_SEL_1SAMP, CMODE_AUTO);
  thermocouple7.MAX31856_config(K_TYPE, CUTOFF_60HZ, AVG_SEL_1SAMP, CMODE_AUTO);
  thermocouple8.MAX31856_config(K_TYPE, CUTOFF_60HZ, AVG_SEL_1SAMP, CMODE_AUTO);
  thermocouple9.MAX31856_config(K_TYPE, CUTOFF_60HZ, AVG_SEL_1SAMP, CMODE_AUTO);
  thermocouple10.MAX31856_config(K_TYPE, CUTOFF_60HZ, AVG_SEL_1SAMP, CMODE_AUTO);
  thermocouple11.MAX31856_config(K_TYPE, CUTOFF_60HZ, AVG_SEL_1SAMP, CMODE_AUTO);

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

    // Get current time
    DateTime now = rtc.now();
  
    // swtich back to Mode 1 for MAX31856
    SPI.setDataMode(SPI_MODE1);             // MAX31856 is a MODE1 or 3 device
   
    // Read CH 0
    thermocouple0.MAX31856_update(&TC_CH0);        // Update MAX31856 channel 0
    // Read CH 1
    thermocouple1.MAX31856_update(&TC_CH1);        // Update MAX31856 channel 1
    // Read CH 2
    thermocouple2.MAX31856_update(&TC_CH2);        // Update MAX31856 channel 2
    // Read CH 3
    thermocouple3.MAX31856_update(&TC_CH3);        // Update MAX31856 channel 3
    // Read CH 4
    thermocouple4.MAX31856_update(&TC_CH4);        // Update MAX31856 channel 4
    // Read CH 5
    thermocouple5.MAX31856_update(&TC_CH5);        // Update MAX31856 channel 5
    // Read CH 6
    thermocouple6.MAX31856_update(&TC_CH6);        // Update MAX31856 channel 6
    // Read CH 7
    thermocouple7.MAX31856_update(&TC_CH7);        // Update MAX31856 channel 7
    // Read CH 8
    thermocouple8.MAX31856_update(&TC_CH8);        // Update MAX31856 channel 8
    // Read CH 9
    thermocouple9.MAX31856_update(&TC_CH9);        // Update MAX31856 channel 9
    // Read CH 10
    thermocouple10.MAX31856_update(&TC_CH10);        // Update MAX31856 channel 10
    // Read CH 11
    thermocouple11.MAX31856_update(&TC_CH11);        // Update MAX31856 channel 11
 
    // switch back to Mode 0 for SD card
    SPI.setDataMode(SPI_MODE0);             // SD card is a Mode 0 device
  
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
    dataString += String(TC_CH0.lin_tc_temp);   // must scale by 0.0078125 in spreadsheet to get value
    dataString += String(TC_CH1.lin_tc_temp);   // must scale by 0.0078125 in spreadsheet to get value
    dataString += String(TC_CH2.lin_tc_temp);   // must scale by 0.0078125 in spreadsheet to get value
    dataString += String(TC_CH3.lin_tc_temp);   // must scale by 0.0078125 in spreadsheet to get value
    dataString += String(TC_CH4.lin_tc_temp);   // must scale by 0.0078125 in spreadsheet to get value
    dataString += String(TC_CH5.lin_tc_temp);   // must scale by 0.0078125 in spreadsheet to get value
    dataString += String(TC_CH6.lin_tc_temp);   // must scale by 0.0078125 in spreadsheet to get value
    dataString += String(TC_CH7.lin_tc_temp);   // must scale by 0.0078125 in spreadsheet to get value
    dataString += String(TC_CH8.lin_tc_temp);   // must scale by 0.0078125 in spreadsheet to get value
    dataString += String(TC_CH9.lin_tc_temp);   // must scale by 0.0078125 in spreadsheet to get value
    dataString += String(TC_CH10.lin_tc_temp);   // must scale by 0.0078125 in spreadsheet to get value
    dataString += String(TC_CH11.lin_tc_temp);   // must scale by 0.0078125 in spreadsheet to get value
    
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
  /*
  // ##### Print information to serial port ####
  Serial.print("TC_0: ");            // Print TC0 header
  print31856Results(&TC_CH0);
  Serial.println(" ");

  Serial.print("TC_1: ");            // Print TC1 header
  print31856Results(&TC_CH1);
  Serial.println(" ");

  Serial.print("TC_2: ");            // Print TC2 header
  print31856Results(&TC_CH2);
  Serial.println(" ");

  Serial.print("TC_3: ");            // Print TC3 header
  print31856Results(&TC_CH3);
  Serial.println(" ");
  
  Serial.print("TC_4: ");            // Print TC4 header
  print31856Results(&TC_CH4);
  Serial.println(" ");

  Serial.print("TC_5: ");            // Print TC5 header
  print31856Results(&TC_CH5);
  Serial.println(" ");

  Serial.print("TC_6: ");            // Print TC6 header
  print31856Results(&TC_CH6);
  Serial.println(" ");

  Serial.print("TC_7: ");            // Print TC7 header
  print31856Results(&TC_CH7);
  Serial.println(" ");
  
  Serial.print("TC_8: ");            // Print TC8 header
  print31856Results(&TC_CH8);
  Serial.println(" ");

  Serial.print("TC_9: ");            // Print TC9 header
  print31856Results(&TC_CH9);
  Serial.println(" ");

  Serial.print("TC_10: ");            // Print TC10 header
  print31856Results(&TC_CH10);
  Serial.println(" ");

  Serial.print("TC_11: ");            // Print TC11 header
  print31856Results(&TC_CH11);
  Serial.println(" ");
*/ 
  
    // Re attempt config if any channels appear stuck or faulted
    if(TC_CH0.status == 0xFF)
    {
      thermocouple0.MAX31856_config(K_TYPE, CUTOFF_60HZ, AVG_SEL_1SAMP, CMODE_AUTO);
      Serial.println("re-attempt config on TC0");
    }
    if(TC_CH1.status == 0xFF)
    {
      thermocouple1.MAX31856_config(K_TYPE, CUTOFF_60HZ, AVG_SEL_1SAMP, CMODE_AUTO);
      Serial.println("re-attempt config on TC1");
    }
    if(TC_CH2.status == 0xFF)
    {
      thermocouple2.MAX31856_config(K_TYPE, CUTOFF_60HZ, AVG_SEL_1SAMP, CMODE_AUTO);
      Serial.println("re-attempt config on TC2");
    }
    if(TC_CH3.status == 0xFF)
    {
      thermocouple3.MAX31856_config(K_TYPE, CUTOFF_60HZ, AVG_SEL_1SAMP, CMODE_AUTO);
      Serial.println("re-attempt config on TC3");
    }
    if(TC_CH4.status == 0xFF)
    {
      thermocouple4.MAX31856_config(K_TYPE, CUTOFF_60HZ, AVG_SEL_1SAMP, CMODE_AUTO);
      Serial.println("re-attempt config on TC4");
    }
    if(TC_CH5.status == 0xFF)
    {
      thermocouple5.MAX31856_config(K_TYPE, CUTOFF_60HZ, AVG_SEL_1SAMP, CMODE_AUTO);
      Serial.println("re-attempt config on TC5");
    }
    if(TC_CH6.status == 0xFF)
    {
      thermocouple6.MAX31856_config(K_TYPE, CUTOFF_60HZ, AVG_SEL_1SAMP, CMODE_AUTO);
      Serial.println("re-attempt config on TC6");
    }
    if(TC_CH7.status == 0xFF)
    {
      thermocouple7.MAX31856_config(K_TYPE, CUTOFF_60HZ, AVG_SEL_1SAMP, CMODE_AUTO);
      Serial.println("re-attempt config on TC7");
    }
    if(TC_CH8.status == 0xFF)
    {
      thermocouple8.MAX31856_config(K_TYPE, CUTOFF_60HZ, AVG_SEL_1SAMP, CMODE_AUTO);
      Serial.println("re-attempt config on TC8");
    }
    if(TC_CH9.status == 0xFF)
    {
      thermocouple9.MAX31856_config(K_TYPE, CUTOFF_60HZ, AVG_SEL_1SAMP, CMODE_AUTO);
      Serial.println("re-attempt config on TC9");
    }
    if(TC_CH10.status == 0xFF)
    {
      thermocouple10.MAX31856_config(K_TYPE, CUTOFF_60HZ, AVG_SEL_1SAMP, CMODE_AUTO);
      Serial.println("re-attempt config on TC10");
    }
    if(TC_CH11.status == 0xFF)
    {
      thermocouple11.MAX31856_config(K_TYPE, CUTOFF_60HZ, AVG_SEL_1SAMP, CMODE_AUTO);
      Serial.println("re-attempt config on TC11");
    }
  }
}

void print31856Results(struct var_max31856 *tc_ptr)
{
  double tmp;
  if(tc_ptr->status)
  {
    // lots of faults possible at once, technically... handle all 8 of them
    // Faults detected can be masked, please refer to library file to enable faults you want represented
    Serial.println("fault(s) detected");
    Serial.print("Fault List: ");
    if(0x01 & tc_ptr->status){Serial.print("OPEN  ");}
    if(0x02 & tc_ptr->status){Serial.print("Overvolt/Undervolt  ");}
    if(0x04 & tc_ptr->status){Serial.print("TC Low  ");}
    if(0x08 & tc_ptr->status){Serial.print("TC High  ");}
    if(0x10 & tc_ptr->status){Serial.print("CJ Low  ");}
    if(0x20 & tc_ptr->status){Serial.print("CJ High  ");}
    if(0x40 & tc_ptr->status){Serial.print("TC Range  ");}
    if(0x80 & tc_ptr->status){Serial.print("CJ Range  ");}
    Serial.println(" ");
    
    // print internal temp anyway
    tmp = (double)tc_ptr->ref_jcn_temp * 0.015625;  // convert fixed pt # to double
    Serial.print("Tint = ");                      // print internal temp heading
    if((-100 > tmp) || (150 < tmp))
    {
      Serial.println("unknown fault");
    }
    else
    {
      Serial.println(tmp);
    }
  }
  else  // no fault, print temperature data
  {
    //Serial.println("no faults detected");
    // MAX31856 Internal Temp
    tmp = (double)tc_ptr->ref_jcn_temp * 0.015625;  // convert fixed pt # to double
    Serial.print("Tint = ");                      // print internal temp heading
    if((-100 > tmp) || (150 < tmp)){Serial.println("unknown fault");}
    else{Serial.print(tmp);}
    
    // MAX31856 External (thermocouple) Temp
    tmp = (double)tc_ptr->lin_tc_temp * 0.0078125;           // convert fixed pt # to double
    Serial.print(" TC Temp = ");                   // print TC temp heading
    Serial.print(tmp);
  }
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
