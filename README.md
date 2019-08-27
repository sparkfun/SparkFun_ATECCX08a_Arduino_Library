SparkFun ADS1015 Arduino Library
===========================================================
 Note, this library is written for use with the Texas Instruments ADS1015. It can be used with most hardware designed around this chip. SparkFun has two products that use this chip, and so we have tailored the example sketchs (and some of the function names) to make most sense with each of these designs. 
 
 Please see the product pages and custom hookup guides for more information on the specific parts of this library that work best with each design.
 
 ![SparkFun Qwiic 12 Bit ADC - 4 Channel ADS1015](https://cdn.sparkfun.com//assets/parts/1/3/8/5/7/15334-SparkFun_Qwiic_12_Bit_ADC_-_4_Channel__ADS1015_-02.jpg)
 
 [*SparkFun Qwiic 12 Bit ADC - 4 Channel (DEV-15334)*](https://www.sparkfun.com/products/15334)
 [Hookup Guide](https://learn.sparkfun.com/tutorials/qwiic-12-bit-adc)

The SparkFun Qwiic 12 Bit ADC can provide four channels of I2C controlled ADC input to your Qwiic enabled project. These channels can be used as single-ended inputs, or in pairs for differential inputs. What makes this even more powerful is that it has a programmable gain amplifier that lets you "zoom in" on a very small change in analog voltage (but will still effect your input range and resolution). Utilizing our handy Qwiic system, no soldering is required to connect it to the rest of your system. However, we still have broken out 0.1"-spaced pins in case you prefer to use a breadboard.

The ADS1015 uses its own internal voltage reference for measurements, but a ground and 3.3V reference are also available on the pin outs for users. This ADC board includes screw pin terminals on the four channels of input, allowing for solderless connection to voltage sources in your setup. It also has an address jumper that lets you choose one of four unique addresses (0x48, 0x49, 0x4A, 0x4B). With this, you can connect up to four of these on the same I2C bus and have sixteen channels of ADC. The maximum resolution of the converter is 12-bits in differential mode and 11-bits for single-ended inputs. Step sizes range from 125μV per count to 3mV per count depending on the full-scale range (FSR) setting.

We have included an onboard 10K trimpot connected to channel A3. This is handy for initial setup testing and can be used as a simple variable input to your project. But don't worry, we added an isolation jumper so you can use channel A3 however you'd like.

![SparkFun Qwiic Flex Glove Controller](https://cdn.sparkfun.com//assets/parts/1/2/8/6/2/14666-SparkFun_Qwiic_Flex_Glove_Controller-01.jpg)

 [*SparkFun Qwiic Flex Glove Controller (SEN-14666)*](https://www.sparkfun.com/products/14666)
 [Hookup Guide](https://learn.sparkfun.com/tutorials/qwiic-flex-glove-controller-hookup-guide/all)

The Qwiic Flex Glove Controller allows you to figure out just how bent all of your fingers are. This board uses an ADC to I2C to create an interface with SparkFun's Qwiic system, which means you won't have to do any soldering to figure out how bent your fingers are.

Library written by Andy England, then modified by Pete Lewis ([SparkFun](http://www.sparkfun.com)). Also, we'd like to thank Adafruit for their work on their Arduino library, as it proved to be a great reference with lots of information. If you'd like to try out their library (which also supports the 16 bit version of this chip:ADS1115), please check that out here: [Adafruit's ADS1X15 library](https://github.com/adafruit/Adafruit_ADS1X15).

Repository Contents
-------------------

* **/examples** - Example sketches for the library (.ino). Run these from the Arduino IDE. 
* **/src** - Source files for the library (.cpp, .h).
* **keywords.txt** - Keywords from this library that will be highlighted in the Arduino IDE. 
* **library.properties** - General library properties for the Arduino package manager. 

Documentation
--------------

* **[Installing an Arduino Library Guide](https://learn.sparkfun.com/tutorials/installing-an-arduino-library)** - Basic information on how to install an Arduino library.
* **[SparkFun Qwiic 12 Bit ADC - 4 Channel Product Repository](https://github.com/sparkfun/SparkFun_Qwiic_12_Bit_ADC_-_4_Channel_ADS1015)** - Main repository (including hardware files)
* **[SparkFun Qwiic Flex Glove Controller Product Repository](https://github.com/sparkfun/Qwiic_Flex_Glove_Controller)** - Main repository (including hardware files)

License Information
-------------------

This product is _**open source**_! 

Various bits of the code have different licenses applied. Anything SparkFun wrote is beerware; if you see me (or any other SparkFun employee) at the local, and you've found our code helpful, please buy us a round! Anything Maxim wrote has its own license. Anything that was co-writing with Peter Jansen is BSD.

Please use, reuse, and modify these files as you see fit. Please maintain attribution to SparkFun Electronics and release anything derivative under the same license.

Distributed as-is; no warranty is given.

- Your friends at SparkFun.
