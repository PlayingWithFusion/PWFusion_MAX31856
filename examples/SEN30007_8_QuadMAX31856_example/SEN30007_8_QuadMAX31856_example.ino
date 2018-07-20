/***************************************************************************
* File Name: SEN30007_8_QuadMAX31856_example.ino
* Processor/Platform: Arduino Uno R3 (tested)
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
* J. Steinlage		2015Dec30   Baseline Rev, first production support
* J. Steilnage    2016Aug21   Change functions to support 1shot mode
* J. Steinlage    2017May08   Change display function, always read TC ch even if fault
* J. Steinlage    2018Jul10   Removed DR and FLT pins - nobody uses them
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
*    Arduino Uno   Arduino Mega  -->  SEN-30006
*    DIO pin 10      DIO pin 10  -->  CS0
*    DIO pin  9      DIO pin  9  -->  CS1
*    DIO pin  8      DIO pin  8  -->  CS2
*    DIO pin  7      DIO pin  7  -->  CS3
*    DIO pin  6      DIO pin  6  -->  DR0 (Data Ready... not used in example, but routed)
*    DIO pin  5      DIO pin  5  -->  DR1 (Data Ready... not used in example, but routed)
*    DIO pin  4      DIO pin  4  -->  DR2 (Data Ready... not used in example, but routed)
*    DIO pin  3      DIO pin  3  -->  DR3 (Data Ready... not used in example, but routed)
*    MOSI: pin 11  MOSI: pin 51  -->  SDI (must not be changed for hardware SPI)
*    MISO: pin 12  MISO: pin 50  -->  SDO (must not be changed for hardware SPI)
*    SCK:  pin 13  SCK:  pin 52  -->  SCLK (must not be changed for hardware SPI)
*    D03           ''            -->  FAULT (not used in example, pin broken out for dev)
*    D02           ''            -->  DRDY (not used in example, only used in single-shot mode)
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

uint8_t TC0_CS  =  7;
uint8_t TC1_CS  =  8;
uint8_t TC2_CS  =  9;
uint8_t TC3_CS  = 10;

PWF_MAX31856  thermocouple0(TC0_CS);
PWF_MAX31856  thermocouple1(TC1_CS);
PWF_MAX31856  thermocouple2(TC2_CS);
PWF_MAX31856  thermocouple3(TC3_CS);
struct var_max31856 TC_CH0, TC_CH1, TC_CH2, TC_CH3;

// proto for display results function
void print31856Results(struct var_max31856 *tc_ptr);

void setup()
{
  delay(1000);                            // give chip a chance to stabilize
  Serial.begin(115200);                   // set baudrate of serial port
  Serial.println("Playing With Fusion: MAX31856, SEN-30007/8");
  Serial.println("Continous Mode Example");

  // setup for the the SPI library:
  SPI.begin();                            // begin SPI
  SPI.setClockDivider(SPI_CLOCK_DIV64);   // SPI speed to SPI_CLOCK_DIV16 (1MHz)
  SPI.setDataMode(SPI_MODE1);             // MAX31856 is a MODE3 device
  
  // call config command... options can be seen in the PlayingWithFusion_MAX31856.h file
  thermocouple0.MAX31856_config(K_TYPE, CUTOFF_60HZ, AVG_SEL_1SAMP, CMODE_AUTO);
  thermocouple1.MAX31856_config(K_TYPE, CUTOFF_60HZ, AVG_SEL_1SAMP, CMODE_AUTO);
  thermocouple2.MAX31856_config(K_TYPE, CUTOFF_60HZ, AVG_SEL_1SAMP, CMODE_AUTO);
  thermocouple3.MAX31856_config(K_TYPE, CUTOFF_60HZ, AVG_SEL_1SAMP, CMODE_AUTO);
}

void loop()
{
  
  delay(500);                                   // 500ms delay... can be as fast as ~100ms in continuous mode, 1 samp avg
  
  // Read CH 0
  thermocouple0.MAX31856_update(&TC_CH0);        // Update MAX31856 channel 0
  // Read CH 1
  thermocouple1.MAX31856_update(&TC_CH1);        // Update MAX31856 channel 1
  // Read CH 2
  thermocouple2.MAX31856_update(&TC_CH2);        // Update MAX31856 channel 2
  // Read CH 3
  thermocouple3.MAX31856_update(&TC_CH3);        // Update MAX31856 channel 3
  
  
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
