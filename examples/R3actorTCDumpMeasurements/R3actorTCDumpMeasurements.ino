/***************************************************************************
* File Name: R3actorDumpMeasurements.ino
* Processor/Platform: R3actor Core Thermo TC
* Development Environment: Arduino 2.3.2
*
* Designed for use with with Playing With Fusion MAX31856 R3actor 
* thermocouple breakout boards: FDQ-30001
*
* Copyright © 2024 Playing With Fusion, Inc.
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
* Author		  Date	      Comments
* J.Simeone   2024-05-30  Initial release
*
* Playing With Fusion, Inc. invests time and resources developing open-source
* code. Please support Playing With Fusion and continued open-source
* development by buying products from Playing With Fusion!
* **************************************************************************
* ADDITIONAL NOTES:
* 
* 
***************************************************************************/

#include <PWFusion_MAX31856.h>

// =========================================================================
//                               TYPES N' DEFS
// =========================================================================

#define NUM_CHANNELS (4)

// =========================================================================
//                               USER VARIABLES
// =========================================================================

static MAX31856 thermocouples[NUM_CHANNELS] = {};    // Thermocouples on FDQ-30001 R3actor TC Board
static int csPins[NUM_CHANNELS] = { 2, 3, 4, 5 };    // Chip-select pins for each MAX31856

// The types of thermocouples, in channel order. Change these if using
// something other than k-type
Tc_Type thermTypes[NUM_CHANNELS] = {
 TYPE_K,
 TYPE_K,
 TYPE_K,
 TYPE_K,
};

// =========================================================================
//                                MAIN PROGRAM
// =========================================================================

void setup() {
  setupSerial();
  setupThermocouples();
}

void loop() {
  for (size_t i = 0; i < NUM_CHANNELS; ++i){
    thermocouples[i].sample();
    print31856Results(i, thermocouples[i]);
    if (i != NUM_CHANNELS - 1){
      Serial.println("----------------------------------------------------------------------|");
    }
  }
  Serial.println("=======================================================================");

  // Run every 1 seconds
  delay(1000);
}

// =========================================================================
//                              EXAMPLE TASKS
// =========================================================================

static void setupSerial() {
  // Setup Serial
  Serial.begin(115200);
  while (!Serial)
  {
    ; // Wait for serial to be set up
  }
}

static void setupThermocouples() {
  // Initialize thermocouples
  for (size_t i = 0; i < NUM_CHANNELS; ++i){
    thermocouples[i].begin(csPins[i]);
    /*
     * This configures each MAX31856 with the following parameters:
     * - thermTypes[i]: Type of thermocouple being measured (configured above)
     *
     * - CUTOFF_60HZ: Internal notch filter frequency, this should match your countries
     *   AC power frequency. If you're in America, that's 60hz. Reference MAX31856 
     *   datasheet for more information.
     *
     * - AVG_SEL_1SAMP: Each conversion should only average accross 1 sample 
     *   (no averaging). This is useful to increase if the thermocouple is in
     *   a high-noise environment.
     * 
     * - CONV_AUTO: Put chip into continuous conversion mode. The chip will
     *   perform conversions until told to stop, automatically updating it's 
     *   internal registers. 
     */ 
    thermocouples[i].config(thermTypes[i], CUTOFF_60HZ, AVG_SEL_1SAMP, CONV_AUTO);
  }
}

// =========================================================================
//                             UTILITY FUNCTIONS
// =========================================================================

void print31856Results(uint8_t channel, MAX31856 &tc)
{
  uint8_t status = tc.getStatus();

  Serial.print("Thermocouple ");
  Serial.print(channel);

  if(status)
  {
    // lots of faults possible at once, technically... handle all 8 of them
    Serial.print(F(": FAULTED - "));
    if(TC_FAULT_OPEN & status)        { Serial.print(F("OPEN, ")); }
    if(TC_FAULT_VOLTAGE_OOR & status) { Serial.print(F("Overvolt/Undervolt, ")); }
    if(TC_FAULT_TC_TEMP_LOW & status) { Serial.print(F("TC Low, ")); }
    if(TC_FAULT_TC_TEMP_HIGH & status){ Serial.print(F("TC High, ")); }
    if(TC_FAULT_CJ_TEMP_LOW & status) { Serial.print(F("CJ Low, ")); }
    if(TC_FAULT_CJ_TEMP_HIGH & status){ Serial.print(F("CJ High, ")); }
    if(TC_FAULT_TC_OOR & status)      { Serial.print(F("TC Range, ")); }
    if(TC_FAULT_CJ_OOR & status)      { Serial.print(F("CJ Range, ")); }
  }
  else  // no fault, print temperature data
  {
    Serial.print(F(": Good - "));
  }

  // MAX31856 Internal Temp
  Serial.print(F("Tint = "));
  float cjTemp = tc.getColdJunctionTemperature();
  if ((cjTemp > -100) && (cjTemp < 150))
  {
    Serial.print(cjTemp);
    Serial.print(F(", "));
  }
  else
  {
    Serial.print(F("Unknown fault with cold junction measurement"));
  }

  // MAX31856 External (thermocouple) Temp
  // Prints regardless of faults
  Serial.print(F("TC Temp = "));
  Serial.print(tc.getTemperature());
  Serial.print(F(", "));

  Serial.println();
}