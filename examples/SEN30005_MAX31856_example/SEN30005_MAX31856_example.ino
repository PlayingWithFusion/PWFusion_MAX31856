/***************************************************************************
* File Name: SEN30006_MAX31856_example.ino
* Processor/Platform: Arduino Uno R3 (tested)
* Development Environment: Arduino 1.8.3
*
* Designed for use with with Playing With Fusion MAX31856 thermocouple
* breakout boards: SEN-30005 (any TC type) or SEN-30006 (any TC type)
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
* J. Steinlage		2015Aug10   Baseline Rev, first production support
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
#include <PWFusion_MAX31856.h>

uint8_t TC0_CS  = 10;

MAX31856  thermocouple0;

void setup()
{
  // Give the MAX31856 a chance to stabilize
  delay(1000);  

  Serial.begin(115200);  // set baudrate of serial port
  Serial.println(F("Playing With Fusion: MAX31856, SEN-30005"));

  // call config command... options can be seen in the PWFusion_MAX31856.h file
  thermocouple0.begin(TC0_CS);
  thermocouple0.config(K_TYPE, CUTOFF_60HZ, AVG_SEL_1SAMP, CMODE_AUTO);
}


void loop()
{
  delay(150);  // 150ms delay... can be as fast as ~100ms in continuous mode, 1 samp avg

  // Get latest measurement from MAX31856 channel 0
  thermocouple0.sample();
  
  // Print information to serial port
  print31856Results(0, thermocouple0);
  Serial.println();
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
