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
* Author    Date      Comments
* J. Steinlage    2015Dec30   Baseline Rev, first production support
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
* - Configure Arduino to broadcast results via UART
*       - call PWF library to configure and read MAX31856 IC (SEN-30005, any type)
* - Broadcast results to COM port
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
#include <PWFusion_MAX31856.h>

#define NUM_THERMOCOUPLES   (sizeof(tcChipSelects) / sizeof(uint8_t))

uint8_t tcChipSelects[] = {10, 9, 8, 7};  // define chip select pins for each thermocouple
MAX31856  thermocouples[NUM_THERMOCOUPLES];


void setup()
{
  // Give the MAX31856 a chance to stabilize
  delay(1000);  

  Serial.begin(115200);  // set baudrate of serial port
  Serial.println(F("Playing With Fusion: MAX31856, SEN-30007/8"));
  Serial.println(F("Continous Mode Example"));

  // Initialize each MAX31856... options can be seen in the PWFusion_MAX31856.h file
  for (int i=0; i<NUM_THERMOCOUPLES; i++)
  {
    thermocouples[i].begin(tcChipSelects[i]);
    thermocouples[i].config(K_TYPE, CUTOFF_60HZ, AVG_SEL_1SAMP, CMODE_AUTO);
  }
}


void loop()
{
  delay(500);                                   // 500ms delay... can be as fast as ~100ms in continuous mode, 1 samp avg
  
  for (int i=0; i<NUM_THERMOCOUPLES; i++)
  {
    // Get latest measurement from MAX31856 channels
    thermocouples[i].sample();
  
    // Print information to serial port
    print31856Results(i, thermocouples[i]);

    // Attempt to recove misconfigured channels
    if(thermocouples[i].getStatus() == 0xFF)
    {
      thermocouples[i].config(K_TYPE, CUTOFF_60HZ, AVG_SEL_1SAMP, CMODE_AUTO);
      Serial.print(F("re-attempt config on TC"));
      Serial.println(i);
    }
  }
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
