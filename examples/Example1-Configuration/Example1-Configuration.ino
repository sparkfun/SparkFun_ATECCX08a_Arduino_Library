
/*
  Using the SparkFun Cryptographic Co-processor Breakout ATECC508a (Qwiic)
  By: Pete Lewis
  SparkFun Electronics
  Date: August 5th, 2019
  License: This code is public domain but you can buy me a beer if you use this and we meet someday (Beerware license).

  Feel like supporting our work? Please buy a board from SparkFun!
  https://www.sparkfun.com/products/15573

  This example shows how to setup your Cryptographic Co-processor with SparkFun's standard settings.
  ***Configurations settings are PERMENANT***
  We highly encourage advanced users to do their own configuration settings.

  Hardware Connections and initial setup:
  Install artemis in boards manager: http://boardsmanager/All#Sparkfun_artemis
  Plug in your controller board (e.g. Artemis Blackboard, Nano, ATP) into your computer with USB cable.
  Connect your Cryptographic Co-processor to your controller board via a qwiic cable.
  Select TOOLS>>BOARD>>"SparkFun Blackboard Artemis"
  Select TOOLS>>PORT>> "COM 3" (note, yours may be different)
  Click upload, and follow configuration prompt on serial monitor at 9600.

*/

#include <SparkFun_ATECCX08a_Arduino_Library.h> //Click here to get the library: http://librarymanager/All#SparkFun_ATECCX08a
#include <Wire.h>

ATECCX08A atecc;

void setup() {
  Wire.begin();
  Serial.begin(9600);
  if (atecc.begin() == true)
  {
    Serial.println("Successful wakeUp(). I2C connections are good.");
  }
  else
  {
    Serial.println("Device not found. Check wiring.");
    while (1); // stall out forever
  }
  
  Serial.println("Would you like to configure your Cryptographic Co-processor with SparkFun Stadard settings? (y/n)");
  
  while (Serial.available() == 0); // wait for user input
  
  if (Serial.read() == 'y')
  {
    Serial.println("Configuration beginning.");
    if(atecc.writeConfigSparkFun() == false) Serial.println("Write failure (config)");
    if(atecc.lockConfig() == false) Serial.println("Lock Failure (config)");   
    if(atecc.createNewKeyPair() == false) Serial.println("Key Creation Failure");
    if(atecc.lockDataAndOTP() == false) Serial.println("Lock Failure (Data/OTP)");
    if(atecc.lockDataSlot0() == false) Serial.println("Lock Failure (Data Slot 0)");
    atecc.readConfigZone(true);
    //while(1);
    atecc.generatePublicKey();
    while(1);
    
    
    Serial.println("Configuration done.");
  }
  else
  {
    Serial.println("Unfortunately, you cannot use any features of the ATECCX08A without configuration and locking.");
  }
}

void loop()
{
  // do nothing.
}
