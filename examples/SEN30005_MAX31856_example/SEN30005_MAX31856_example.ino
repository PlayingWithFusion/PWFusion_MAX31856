/***************************************************************************
* File Name: SEN30006_MAX31856_example.ino
* Processor/Platform: Arduino Uno R3 (tested)
* Development Environment: Arduino 1.6.1
*
* Designed for use with with Playing With Fusion MAX31856 thermocouple
* breakout boards: SEN-30005 (any TC type) or SEN-30006 (any TC type)
*
* Copyright © 2015 Playing With Fusion, Inc.
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
* J. Steinlage		2015Aug10   Baseline Rev, first production support
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
*    Arduino Uno   Arduino Mega  -->  SEN-30005
*    DIO pin 10      DIO pin 10  -->  CS0
*    MOSI: pin 11  MOSI: pin 51  -->  SDI (must not be changed for hardware SPI)
*    MISO: pin 12  MISO: pin 50  -->  SDO (must not be changed for hardware SPI)
*    SCK:  pin 13  SCK:  pin 52  -->  SCLK (must not be changed for hardware SPI)
*    D03           ''            -->  FAULT (not used in example, pin broken out for dev)
*    D02           ''            -->  DRDY (not used in example, only used in single-shot mode)
*    GND           GND           -->  GND
*    5V            5V            -->  Vin (supply with same voltage as Arduino I/O, 5V)
*     NOT CONNECTED              --> 3.3V (this is 3.3V output from on-board LDO. DO NOT POWER THIS PIN!
***************************************************************************/
#include "PlayingWithFusion_MAX31856.h"
#include "PlayingWithFusion_MAX31856_STRUCT.h"
#include "SPI.h"

uint8_t TC0_CS  = 10;
uint8_t TC1_CS  =  9;
uint8_t TC0_FAULT = 3;                     // not used in this example, but needed for config setup
uint8_t TC0_DRDY  = 2;                     // not used in this example, but needed for config setup

PWF_MAX31856  thermocouple0(TC0_CS, TC0_FAULT, TC0_DRDY);

void setup()
{
  delay(1000);                            // give chip a chance to stabilize
  Serial.begin(115200);                   // set baudrate of serial port
  Serial.println("Playing With Fusion: MAX31856, SEN-30005");

  // setup for the the SPI library:
  SPI.begin();                            // begin SPI
  SPI.setClockDivider(SPI_CLOCK_DIV16);   // SPI speed to SPI_CLOCK_DIV16 (1MHz)
  SPI.setDataMode(SPI_MODE3);             // MAX31856 is a MODE3 device
  
  // call config command... options can be seen in the PlayingWithFusion_MAX31856.h file
  thermocouple0.MAX31856_config(K_TYPE, CUTOFF_60HZ, AVG_SEL_4SAMP);
}

void loop()
{
  
  delay(150);                                   // 500ms delay... can be as fast as ~100ms in continuous mode, 1 samp avg
  
  static struct var_max31856 TC_CH0;
  double tmp;
  
  struct var_max31856 *tc_ptr;
  
  // Read CH 0
  tc_ptr = &TC_CH0;                             // set pointer
  thermocouple0.MAX31856_update(tc_ptr);        // Update MAX31856 channel 0
  
  
  // ##### Print information to serial port ####
  
  // Thermocouple channel 0
  Serial.print("Thermocouple 0: ");            // Print TC0 header
  if(TC_CH0.status)
  {
    // lots of faults possible at once, technically... handle all 8 of them
    // Faults detected can be masked, please refer to library file to enable faults you want represented
    Serial.println("fault(s) detected");
    Serial.print("Fault List: ");
    if(0x01 & TC_CH0.status){Serial.print("OPEN  ");}
    if(0x02 & TC_CH0.status){Serial.print("Overvolt/Undervolt  ");}
    if(0x04 & TC_CH0.status){Serial.print("TC Low  ");}
    if(0x08 & TC_CH0.status){Serial.print("TC High  ");}
    if(0x10 & TC_CH0.status){Serial.print("CJ Low  ");}
    if(0x20 & TC_CH0.status){Serial.print("CJ High  ");}
    if(0x40 & TC_CH0.status){Serial.print("TC Range  ");}
    if(0x80 & TC_CH0.status){Serial.print("CJ Range  ");}
    Serial.println(" ");
  }
  else  // no fault, print temperature data
  {
    Serial.println("no faults detected");
    // MAX31856 Internal Temp
    tmp = (double)TC_CH0.ref_jcn_temp * 0.015625;  // convert fixed pt # to double
    Serial.print("Tint = ");                      // print internal temp heading
    if((-100 > tmp) || (150 < tmp)){Serial.println("unknown fault");}
    else{Serial.println(tmp);}
    
    // MAX31856 External (thermocouple) Temp
    tmp = (double)TC_CH0.lin_tc_temp * 0.0078125;           // convert fixed pt # to double
    Serial.print("TC Temp = ");                   // print TC temp heading
    Serial.println(tmp);
  }
  
  // can use this template to add extra TC channels
  // Thermocouple channel 1
//  Serial.print("Thermocouple 1: ");            // Print TC0 header
//  if(TC_CH1.status)
//  {
//    // lots of faults possible at once, technically... handle all 8 of them
//    // Faults detected can be masked, please refer to library file to enable faults you want represented
//    Serial.println("fault(s) detected");
//    Serial.print("Fault List: ");
//    if(0x01 & TC_CH1.status){Serial.print("OPEN  ");}
//    if(0x02 & TC_CH1.status){Serial.print("Overvolt/Undervolt  ");}
//    if(0x04 & TC_CH1.status){Serial.print("TC Low  ");}
//    if(0x08 & TC_CH1.status){Serial.print("TC High  ");}
//    if(0x10 & TC_CH1.status){Serial.print("CJ Low  ");}
//    if(0x20 & TC_CH1.status){Serial.print("CJ High  ");}
//    if(0x40 & TC_CH1.status){Serial.print("TC Range  ");}
//    if(0x80 & TC_CH1.status){Serial.print("CJ Range  ");}
//    Serial.println(" ");
//  }
//  else  // no fault, print temperature data
//  {
//    Serial.println("no faults detected");
//    // MAX31856 Internal Temp
//    tmp = (double)TC_CH1.ref_jcn_temp * 0.015625;  // convert fixed pt # to double
//    Serial.print("Tint = ");                      // print internal temp heading
//    if((-100 > tmp) || (150 < tmp)){Serial.println("unknown fault");}
//    else{Serial.println(tmp);}
//    
//    // MAX31856 External (thermocouple) Temp
//    tmp = (double)TC_CH1.lin_tc_temp * 0.0078125;           // convert fixed pt # to double
//    Serial.print("TC Temp = ");                   // print TC temp heading
//    Serial.println(tmp);
//  }
}

