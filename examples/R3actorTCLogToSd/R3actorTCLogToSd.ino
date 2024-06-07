/***************************************************************************
* File Name: R3actorLogToSd.ino
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
* It's recommended to reference the datasheet for the MAX31856 as well as the
* MAX31856 Playing With Fusion library. 
*
* This example demonstrates how to: 
* - Read from thermocouples
* - Read from internal MAX31856 temperature
* - Read battery voltage from charge circuit
* - Log as CSV to SD card
***************************************************************************/

#include <SD.h>
#include <PWFusion_MAX31856.h>

// =========================================================================
//                                     DEFS
// =========================================================================

#define NUM_CHANNELS (4)

// Structure holding thermocouple conversion data
typedef struct {
  float temp;
  float intTemp;
  bool fault;
} tc_vals_t;

// =========================================================================
//                             EXAMPLE VARIABLES
// =========================================================================

static File csvFile;   // CSV file object
static String path;    // Name of CSV file

static MAX31856 thermocouples[NUM_CHANNELS] = {};    // Thermocouples on FDQ-30001 R3actor TC Board
static int csPins[NUM_CHANNELS] = { 2, 3, 4, 5 };    // Chip-select pins for each MAX31856
static tc_vals_t vals[NUM_CHANNELS] = {};            // Measured temperatures, internal temp, fault

// =========================================================================
//                               USER VARIABLES
// =========================================================================
// Change these to change the behavior of the example

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

void
setup()
{
  // Setup tasks (see below)
  setupSerial();
  setupSDCard();
  setupThermocouples();
}

void
loop()
{
  // Periodic tasks (see below)
  updateThermocouples();
  writeToSDCard();

  // Run every 2 seconds
  delay(2000);
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

static void setupSDCard() {
  // Setup SD Card
  Serial.print("Initializing SD card....");

  if (!SD.begin())
  {
    Serial.println("Failed");
    while (1);
  }
  else
  {
    Serial.println("Done");
  }

  Serial.println("Initializing log file...");

  path = getNextFileName();
  csvFile = SD.open(path, FILE_WRITE);

  if (!csvFile) {
    Serial.println("Failed to initialize CSV file, aborting");
    while (1);
  }
  
  Serial.print("Logging to: ");
  Serial.println(path);
  Serial.println("Done");

  csvFile.println(getCSVHeader());
  csvFile.close();
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
     * - CMODE_AUTO: Put chip into continuous conversion mode. The chip will
     *   perform conversions until told to stop, automatically updating it's 
     *   internal registers. 
     */ 
    thermocouples[i].config(thermTypes[i], CUTOFF_60HZ, AVG_SEL_1SAMP, CONV_AUTO);
  }
}

static void updateThermocouples() {
  // Update thermocouples
  for (size_t i = 0; i < NUM_CHANNELS; ++i){

    float temp = 0;
    float intTemp = 0;
    bool fault = true;

    thermocouples[i].sample();

    // Check for faults (report zero temps if faulting)
    if (thermocouples[i].getStatus() == 0) {
      fault = false;
    }

    intTemp = thermocouples[i].getTemperature();
    temp = thermocouples[i].getTemperature();

    vals[i] = tc_vals_t {
      .temp = temp,
      .intTemp = intTemp,
      .fault = fault,
    };

  }
}

static void writeToSDCard() {
  // Write data to SD card
  // TODO: remove sim
  csvFile = SD.open(path, FILE_WRITE);
  String data = getCSVData(vals[0], vals[1], vals[2], vals[3], getBatteryVoltage(), millis());
  csvFile.println(data);
  Serial.println(data);
  csvFile.close();
}

// =========================================================================
//                             UTILITY FUNCTIONS
// =========================================================================

/*
 * Count the number of files present on the SD card
 * to create a new file name for the R3actor Logger.
 * NOTE: SD library support 8.3 file names, meaning
 * with the naming scheme "log<number>.csv", this function
 * can support up to 99,999 different files.
 */
String
getNextFileName()
{
  int numFiles = 0;
 File root = SD.open("/");
  if (!root)
  {
    Serial.println("Failed to open root file...");
    return "";
  }

  File nextFile = root.openNextFile();

  while (nextFile)
  {
    numFiles++;
    nextFile = root.openNextFile();
  }

  return "tc" + String(numFiles) + ".csv";
}

/*
 * Create the string representation of data for CSV file
 * 
 */
String
getCSVData(tc_vals_t tc0, tc_vals_t tc1, tc_vals_t tc2, tc_vals_t tc3, float battV, long runTime)
{
  return String(tc0.temp) + "," + String(tc0.intTemp) + "," + String(tc0.fault) 
  + "," + String(tc1.temp) + "," + String(tc1.intTemp) + "," + String(tc1.fault)
  + "," + String(tc2.temp) + "," + String(tc2.intTemp) + "," + String(tc2.fault) 
  + "," + String(tc3.temp) + "," + String(tc3.intTemp) + "," + String(tc3.fault) 
  + "," + battV + "," + runTime;
}

/*
 * Construct a CSV file header. The CSV file will be 
 * prepended with the model number of board currently being used
 */
String
getCSVHeader()
{
  return "MODEL NUMBER: FDQ-30001\ntc0-t,tc0-int,tc0-fault,tc1-t,tc1-int,tc1-fault,tc2-t,tc2-int,tc2-fault,tc3-t,tc3-int,tc3-fault,battV,ms-after-start";
}

/*
 * Get the battery voltage from the charge circuit onboard
 */
float
getBatteryVoltage()
{
  // two 499k ohm resistors create a 
  // voltage divier with a ratio of 1:2
  // 
  // This function assumes:
  // 10-bit wide, 1.65V Vref, which should be 
  // the case if running on a R3actor TC Logger
  return ((float)analogRead(ADC_BATTERY) * 3.3) / 1023.0; // 3.3 = 1.65 * 2
}