<p align="center">
    <img src="https://www.playingwithfusion.com/images/logo.svg" width="50%" />
</p>

<h1 align="center">Playing With Fusion MAX31856 Arduino Library</h1>

![SEN-30005-K ISO](https://www.playingwithfusion.com/include/getimg.php?imgid=1276)
![SEN-30006-J ISO](https://www.playingwithfusion.com/include/getimg.php?imgid=1292)
![SEN-30007-T ISO](https://www.playingwithfusion.com/include/getimg.php?imgid=1324)
![SEN-30008-ST ISO](https://www.playingwithfusion.com/include/getimg.php?imgid=1334)
![SEN-30012-K ISO](https://www.playingwithfusion.com/include/getimg.php?imgid=1811)

Arduino library designed to interface Arduino-compatible hardware with the Maximum MAX31856 Thermocouple to digital converter IC over SPI, used for digitizing thermocouple measurements. Examples provided to interface with SEN-30005, SEN-30006, SEN-30007 and SEN-30008

# Library Documentation

Brief documentation for this library. Make sure to see "examples" folder for ready-to-upload programs.

## Configuration

Configuration of a thermocouple requires the **Thermocouple Type**, **Filter Frequency**, **Average Mode**, and **Measure Mode**.

A basic configuration for a k-type thermocouple may look like this:

```cpp
MAX31856 tcConverter;

void setup(){
  tcConverter.config(TYPE_K, CUTOFF_60HZ, AVG_SEL_1SAMP, CONV_AUTO);
}

/* ... */
```

The above code configures a K-type thermocouple with a selected notch filter frequency of 60hz, no averaging accross samples, and auto conversion. 

### Notch Frequency

You can configure the notch frequency with the `MAX31856::config` function. The notch frequency is designed to tell the MAX31856 what frequency of noise the internal filter should cut out. For instance, in the U.S., our power is 120V 60hz AC. The noise from these power lines can, in some cases, interfere with long thermocouple wires, so we select 60hz for the filter frequency. Those that have 50hz power, should select 50hz.

### Averaging

The MAX31856 allows a user to configure averaging to be done on the IC itself accross multiple measurements. This increases accuracy, but makes measuring each conversion take longer. For the quickest sampling rate, keep this at `AVG_SEL_1SAMP`.

### Conversion Mode

The MAX31856 can perform measurements over and over again, without input from the microcontroller, or, can be configured to only measure when the MCU requests it. For basic use, auto conversion mode is fine (`CONV_AUTO`).

## Sampling

When in **automatic** conversion mode, simply call the `MAX31856::sample()` function to update the library's internal data with the MAX31856's data. 

```cpp
tcConverter.sample();
```

When in **single-shot** mode, the user must explicitly start the conversion, and then read it later, after it's had time to complete the conversion. For the time it takes for single-shot to perform conversions, see the MAX31856 datasheet. 

```cpp
tcConverter.startOneShotMeasurement();   // Start measurement
delay(180);                              // waiting for new measurement (143ms for 60Hz, 169ms for 50Hz)
tcConverter.sample();                    // Get latest measurement from MAX31856 channels
```

# Compatible Playing With Fusion Products:

- SEN-30005 (1-ch): <a href="http://www.playingwithfusion.com/productview.php?pdid=58">J-Type Connector</a>, <a href="http://www.playingwithfusion.com/productview.php?pdid=59">K-Type Connector</a>, <a href="http://www.playingwithfusion.com/productview.php?pdid=60">T-Type Connector</a>, <a href="http://www.playingwithfusion.com/productview.php?pdid=57">Screw Terminal Connector</a>
  
- SEN-30006 (2-ch): <a href="http://www.playingwithfusion.com/productview.php?pdid=62">J-Type Connector</a>, <a href="http://www.playingwithfusion.com/productview.php?pdid=63">K-Type Connector</a>, <a href="http://www.playingwithfusion.com/productview.php?pdid=64">T-Type Connector</a>, <a href="http://www.playingwithfusion.com/productview.php?pdid=61">Screw Terminal Connector</a>

- SEN-30007 (4-ch Arduino Shield): <a href="http://www.playingwithfusion.com/productview.php?pdid=69">J-Type Connector</a>, <a href="http://www.playingwithfusion.com/productview.php?pdid=70">K-Type Connector</a>, <a href="http://www.playingwithfusion.com/productview.php?pdid=71">T-Type Connector</a>, <a href="http://www.playingwithfusion.com/productview.php?pdid=72">Screw Terminal Connector</a>

- SEN-30008 (4-ch universal): <a href="http://www.playingwithfusion.com/productview.php?pdid=73">J-Type Connector</a>, <a href="http://www.playingwithfusion.com/productview.php?pdid=74">K-Type Connector</a>, <a href="http://www.playingwithfusion.com/productview.php?pdid=75">T-Type Connector</a>, <a href="http://www.playingwithfusion.com/productview.php?pdid=76">Screw Terminal Connector</a>

- SEN-30012 (4-ch Arduino Shield): <a href="http://www.playingwithfusion.com/productview.php?pdid=224">J-Type Connector</a>, <a href="http://www.playingwithfusion.com/productview.php?pdid=225">K-Type Connector</a>, <a href="http://www.playingwithfusion.com/productview.php?pdid=226">T-Type Connector</a>, <a href="http://www.playingwithfusion.com/productview.php?pdid=227">Screw Terminal Connector</a>

Note: all thermocouple interface types are available for each board variant

Questions? Feel free to <a href="http://www.playingwithfusion.com/contactus.php">contact us!</a>
