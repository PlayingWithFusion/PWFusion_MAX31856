/***************************************************************************
* File Name: PlayingWithFusion_MAX31856.cpp
* Processor/Platform: Arduino Uno R3 (tested)
* Development Environment: Arduino 1.8.3
*
* Designed for use with with Playing With Fusion MAX31856 thermocouple
* breakout boards: SEN-30005, SEN-30006 (any TC type)
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
* Author         Date      Comments
* J. Steinlage      2015Aug10   First rev
* J. Steinlage      2017May08   Read TC ch, even if fault. Fix reg update fcn
* J. Steinlage      2018Jul20   Removed DR and FLT pins since nobody uses them
* J. Leonard        2024Apr25   Initialized TC and CJ fault threshold registers
*
* Playing With Fusion, Inc. invests time and resources developing open-source
* code. Please support Playing With Fusion and continued open-source
* development by buying products from Playing With Fusion!
* **************************************************************************
* ADDITIONAL NOTES:
* This file contains functions to initialize and run an Arduino Uno R3 in
* order to communicate with a MAX31856 single channel thermocouple breakout
* board. Funcionality is as described below:
*   - Initialize TC channel
*   - Read MAX31856 registers from Playing With Fusion SEN-30005 (any type)
*   - Properly unpack data into internal temp, TC temp and status variables
***************************************************************************/
#include "PWFusion_MAX31856.h"

// MAX31856 CLASS FUNCITONS

MAX31856::MAX31856() :
   _spiSettings(5000000, MSBFIRST, SPI_MODE1),
   _spiSettingsJunk(1000000, MSBFIRST, SPI_MODE1)
{
}


void MAX31856::begin(int8_t chipSelectPin, SPIClass &spiPort)
{
  // Function to initialize thermocouple channel, load private variables
  _cs = chipSelectPin;
  _spiPort = &spiPort;
  
  pinMode(_cs, OUTPUT);
  
  // immediately pull CS pin high to avoid conflicts on SPI bus
  digitalWrite(_cs, HIGH);

   _spiPort->begin();

   // Workaround for Uno R4.  Set SPI configuration to 'something' different so
   // that the next time beginTransaction(_spiSettings) is called for real the R4's
   // SPI driver is forced to change SPI settings
   _spiPort->beginTransaction(_spiSettingsJunk);
   _spiPort->endTransaction(); 
}


uint8_t MAX31856::readByte(Max31856_Reg reg)
{
  uint8_t result;

  _spiPort->beginTransaction(_spiSettings);  
  digitalWrite(_cs, LOW);           // set pin low to start talking to IC

  _spiPort->transfer(reg);            // write address
  result = _spiPort->transfer(0);   // read register data from IC

  digitalWrite(_cs, HIGH);          // set pin high to end SPI session
  _spiPort->endTransaction(); 

  return result;
}


void MAX31856::writeByte(Max31856_Reg reg, uint8_t value)
{
  _spiPort->beginTransaction(_spiSettings);  
  digitalWrite(_cs, LOW);           // set pin low to start talking to IC

  _spiPort->transfer(reg | 0x80);
  _spiPort->transfer(value);

 digitalWrite(_cs, HIGH);          // set pin high to end SPI session
  _spiPort->endTransaction(); 
}


void MAX31856::config(Tc_Type TC_TYPE, uint8_t FILT_FREQ, uint8_t AVG_MODE, Max31856_Conversion_Mode MEAS_MODE)
{
   // TC_TYPE: B_TYPE, E_TYPE, J_TYPE, K_TYPE, N_TYPE, R_TYPE, S_TYPE, T_TYPE
   // FILT_FREQ: CUTOFF_60HZ, CUTOFF_50HZ
   // AVG_MODE: AVG_SEL_1SAMP, AVG_SEL_2SAMP, AVG_SEL_4SAMP, AVG_SEL_8SAMP, AVG_SEL_16SAMP
   // MEAS_MODE: CONV_AUTO, CONV_SINGL
   
   uint8_t regdat = 0;      // set up paramater to compile register configs
   
   // set CR0 (REG_CR0)
   if(MEAS_MODE == CONV_AUTO) // auto conversion mode selected (~100ms interval sampling)
   {
      regdat = (CMODE_AUTO | ONESHOT_OFF | OCFAULT_10MS | CJ_ENABLED | FAULT_AUTO | FAULT_CLR_DEF | FILT_FREQ);
   }
   else   // else it's single-shot mode
   {
      regdat = (CMODE_OFF | ONESHOT_OFF | OCFAULT_10MS | CJ_ENABLED | FAULT_AUTO | FAULT_CLR_DEF | FILT_FREQ);
   }
   writeByte(REG_CR0, regdat);   // write data to register
/*   CRO, 00h/80h:[7] cmode (0=off (default), 1=auto conv mode)
      [6] 1shot (0=off, default)
      [5:4] OCFAULT (table 4 in datasheet)
      [3] CJ disable (0=cold junction enabled by default, 1=CJ disabled, used to write CJ temp)
      [2] FAULT mode (0=sets, clears automatically, 1=manually cleared, sets automatically)
      [1] FAULTCLR   (0 - default, 1=see datasheet)
      [0] 50/60Hz (0=60hz (default), 1=50Hz filtering) + harmonics */
   
   // set CR1 (REG_CR1)
   regdat = (AVG_MODE | TC_TYPE);
   writeByte(REG_CR1, regdat);
/*   CR1, 01h/81h:[7] reserved
      [6:4] AVGSEL (0=1samp(default),1=2samp,2=4samp,3=8samp,0b1xx=16samp])
      [3:0] TC type (0=B, 1=E, 2=J, 3=K(default), 4=N, 5=R, 6=S, 7=T, others, see datasheet)*/
   
   // set MASK (REG_MASK) - PWF default masks all but OV/UV and OPEN from lighting LED
   regdat = (CJ_HIGH_MASK | CJ_LOW_MASK | TC_HIGH_MASK | TC_LOW_MASK); 
   writeByte(REG_MASK, regdat);
/*   MASK, 02h/82h: This register masks faults from causing the FAULT output from asserting,
               but fault bits will still be set in the FSR (0x0F)
                 All faults are masked by default... must turn them on if desired
      [7:6] reserved
      [5] CJ high fault mask
      [4] CJ low fault mask
      [3] TC high fault mask
      [2] TC low fault mask
      [1] OV/UV fault mask
      [0] Open fault mask
      PWF example: 0x03 (OV/UV + open) */
   
   // LEAVE CJHFT/CJLFT AT DEFAULT VALUES FOR PWF EXAMPLE
   // note: these values would potentially be used to indicate material or component  
   //       limits have been exceeded for your specific measurement configuration
/*   CJHFT, 03h/83h: cold-jcn high fault threshold, default 0x7F (bit 7 is sign)
   CJLFT, 04h/84h: cold-jcn low fault threshold, default 0x00) */
   writeByte(REG_CJHF, 0x7F);
   writeByte(REG_CJLF, 0x00);

   // LEAVE LTXFTX AT DEFAULT VALUES FOR PWF EXAMPLE
   // note: these values would potentially be used to indicate material limits 
   //       have been exceeded for your specific thermocouple
/*   LTHFTH, 05h/85h: Linearize temperature high fault thresh MSB (bit 7 is sign)
   LTHFTL, 06h/86h: Linearize temperature high fault thresh LSB
   LTLFTH, 07h/87h: Linearize temperature low fault thresh MSB (bit 7 is sign)
   LTLFTL, 08h/88h: Linearize temperature low fault thresh LSB */
   writeByte(REG_LTHFTH, 0x7F);
   writeByte(REG_LTHFTL, 0xFF);
   writeByte(REG_LTLFTH, 0xFF);
   writeByte(REG_LTLFTL, 0x80);
}


void MAX31856::sample()
{
   uint16_t rawCJReg;
   uint32_t rawTCReg;
   // Start by reading SR for any faults, exit if faults present, though some
   // faults could potentially be dealt with
   status = readByte(REG_SR);
   
   // Read Cold Jcn temperature (2 registers)
   rawCJReg  = (uint16_t)readByte(REG_CJTH) << 8; // MSB, left shift 8
   rawCJReg |= (uint16_t)readByte(REG_CJTL);              // LSB read

   // convert sign-magnitude value to 2's complement.   If bit 0x2000 is set this is a negative number, 
   // convert all the bits to the right of the sign bit to 2s complement
   // Shift to align with memory
   rawCJReg >>= 2;
   if (rawCJReg & 0x2000)
   {
      rawCJTemp = (int16_t)(-(rawCJReg & 0x1FFF));
   }
   else
   {
      rawCJTemp = (int16_t)(rawCJReg & 0x1FFF);
   }
   
   // Read Linearized TC temperature (3 registers)
   rawTCReg  = (uint32_t)readByte(REG_LTCBH) << 24;   // MSB, left shift by 3 bytes
   rawTCReg |= (uint32_t)readByte(REG_LTCBM) << 16;
   rawTCReg |= (uint32_t)readByte(REG_LTCBL) << 8;    // LSB, still shifted left by one byte

   // convert sign-magnitude value to 2's complement.   If bit 0x40000 is set this is a negative number, 
   // convert all the bits to the right of the sign bit to 2s complement
   // Shift to align with memory
   rawTCReg >>= 13;
   if (rawTCReg & 0x40000)
   {
      // Negative
      rawTCTemp = (int32_t)(-(rawTCReg & 0x3FFFF));
   }
   else
   {
      // Positive
      rawTCTemp = (int32_t)(rawTCReg & 0x3FFFF);
   }
}


void MAX31856::startOneShotMeasurement()
{
	uint8_t value = readByte(REG_CR0);

	value |= ONESHOT_ON; // Set the oneshot bit
   
   writeByte(REG_CR0,value);   // set the 1shot bit
   // note, it takes approximately 143ms to perform the conversion in 60-Hz filter mode
   // and 169ms in 50-Hz filter mode. Keep this in mind if not using DRDY output for interrupt.
   // Call the MAX31856_update command after the conversion is complete to read the new value.
   // Also, don't run simultaneous conversions on multiple MAX chips if they are electrically connected
   // as the readings will likely be skewed.
}


void MAX31856::setColdJunctionOffset(float offsetDegC)
{
	// offset is 2^-4 degC/bit
	int8_t rawValue = (int8_t)(offsetDegC * 16.0);

   /*   CJTO, 09h/89h: Cold Junction Temperature Offset (int8_t, default 0x00) */
   // This function could be used to add a temperature offset based on a known difference
   //      between the chip temp and the location of the TC metal to copper transition
   writeByte(REG_CJTO, (uint8_t)rawValue);
}


float MAX31856::getTemperature()
{
   // linearized TC temperature, 0.0078125 decC/bit (2^-7)
   return (float)rawTCTemp * 0.0078125;
}


float MAX31856::getColdJunctionTemperature()
{
	// Temperature of chip cold junction, 0.015625 deg C/bit (2^-6)
	return (float)rawCJTemp * 0.015625;
}


uint8_t MAX31856::getStatus()
{
	return status;
}