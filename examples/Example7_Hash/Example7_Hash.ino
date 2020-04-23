/*
  Using the SparkFun Cryptographic Co-processor Breakout ATECC508a (Qwiic)
  By: Pete Lewis
  SparkFun Electronics
  Date: April 23rd, 2020
  License: This code is public domain but you can buy me a beer if you use this and we meet someday (Beerware license).

  Contributions for SHA256 support by Gili Yankovitch (github @gili-yankovitch), March 2020
  See details of PR here: https://github.com/sparkfun/SparkFun_ATECCX08a_Arduino_Library/pull/3

  Feel like supporting our work? Please buy a board from SparkFun!
  https://www.sparkfun.com/products/15573

  This example shows how to create a hash on larger amounts of data.
  If you have only 32 bytes of data, then you can simply sign the data,
  But if you have more than that, then you mush send your data into an SHA256 algorithm,
  And this will create a 32-byte hash of the data. It's kind of like a signature, but it's created
  with a standard algorithm, and so anyone can make it. The Cryptographic Co-processor will make it
  for us easily in an embedded system.

  Once you have your 32-byte hash of the data, then you can sign that. With this signature of your hash,
  you can authenticate a message that is any length.

  Note, this requires that your device be configured with SparkFun Standard Configuration settings.
  By default, this example uses the private key securely stored and locked in slot 0.

  Hardware Connections and initial setup:
  Install artemis in boards manager: http://boardsmanager/All#Sparkfun_artemis
  Plug in your controller board (e.g. Artemis Redboard, Nano, ATP) into your computer with USB cable.
  Connect your Cryptographic Co-processor to your controller board via a qwiic cable.
  Select TOOLS>>BOARD>>"SparkFun Redboard Artemis"
  Select TOOLS>>PORT>> "COM 3" (note, yours may be different)
  Click upload, and follow along on serial monitor at 115200.

*/

#include <SparkFun_ATECCX08a_Arduino_Library.h> //Click here to get the library: http://librarymanager/All#SparkFun_ATECCX08a
#include <Wire.h>

ATECCX08A atecc;

// array to hold our message. Note, this can be greater than 32 bytes!!
uint8_t message[] = {
  0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,
  0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F,
};

// larger example message array, uncomment this to try it out. 
// (don't forget to comment out previous message array)
//uint8_t message[] = {
//  0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,
//  0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F,
//  0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2A, 0x2B, 0x2C, 0x2D, 0x2E, 0x2F,
//  0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3A, 0x3B, 0x3C, 0x3D, 0x3E, 0x3F,
//  0x40
//};

// array to hold our 32-byte hash. 
// Note, it must be 32 bytes, no more or less.
// Also note, we will start with it blank, but the crypto co-processor will fill it in when we call atecc.sha256()
uint8_t myHash[32] = {
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};


void setup() {
  Wire.begin();
  Serial.begin(115200);
  if (atecc.begin() == true)
  {
    Serial.println("Successful wakeUp(). I2C connections are good.");
  }
  else
  {
    Serial.println("Device not found. Check wiring.");
    while (1); // stall out forever
  }

  printInfo(); // see function below for library calls and data handling

  // check for configuration
  if (!(atecc.configLockStatus && atecc.dataOTPLockStatus && atecc.slot0LockStatus))
  {
    Serial.print("Device not configured. Please use the configuration sketch.");
    while (1); // stall out forever.
  }

  printMessage(); // nice debug to see what you're hashing. see function below

  //Let's create a hash!
  Serial.println("Creating SHA-256 Hash of data...\n\r");
  
  atecc.sha256(message, sizeof(message), myHash); // send message and hash pointers

  printMyHash(); // nice debug to see your new hash. see function below
}

void loop()
{
  // do nothing.
}

void printMessage()
{
  Serial.println("uint8_t message[] = {");
  for (int i = 0; i < sizeof(message) ; i++)
  {
    Serial.print("0x");
    if ((message[i] >> 4) == 0) Serial.print("0"); // print preceeding high nibble if it's zero
    Serial.print(message[i], HEX);
    if (i < (sizeof(message) - 1)) Serial.print(", ");
    if ((31 - i) % 16 == 0) Serial.println();
  }
  Serial.println("};\n\r");
}

void printMyHash()
{
  Serial.println("uint8_t myHash[32] = {");
  for (int i = 0; i < sizeof(myHash) ; i++)
  {
    Serial.print("0x");
    if ((myHash[i] >> 4) == 0) Serial.print("0"); // print preceeding high nibble if it's zero
    Serial.print(myHash[i], HEX);
    if (i != 31) Serial.print(", ");
    if ((31 - i) % 16 == 0) Serial.println();
  }
  Serial.println("};");
}

void printInfo()
{
  // Read all 128 bytes of Configuration Zone
  // These will be stored in an array within the instance named: atecc.configZone[128]
  atecc.readConfigZone(false); // Debug argument false (OFF)

  // Print useful information from configuration zone data
  Serial.println();

  Serial.print("Serial Number: \t");
  for (int i = 0 ; i < 9 ; i++)
  {
    if ((atecc.serialNumber[i] >> 4) == 0) Serial.print("0"); // print preceeding high nibble if it's zero
    Serial.print(atecc.serialNumber[i], HEX);
  }
  Serial.println();

  Serial.print("Rev Number: \t");
  for (int i = 0 ; i < 4 ; i++)
  {
    if ((atecc.revisionNumber[i] >> 4) == 0) Serial.print("0"); // print preceeding high nibble if it's zero
    Serial.print(atecc.revisionNumber[i], HEX);
  }
  Serial.println();

  Serial.print("Config Zone: \t");
  if (atecc.configLockStatus) Serial.println("Locked");
  else Serial.println("NOT Locked");

  Serial.print("Data/OTP Zone: \t");
  if (atecc.dataOTPLockStatus) Serial.println("Locked");
  else Serial.println("NOT Locked");

  Serial.print("Data Slot 0: \t");
  if (atecc.slot0LockStatus) Serial.println("Locked");
  else Serial.println("NOT Locked");

  Serial.println();

  // if everything is locked up, then configuration is complete, so let's print the public key
  if (atecc.configLockStatus && atecc.dataOTPLockStatus && atecc.slot0LockStatus)
  {
    if (atecc.generatePublicKey() == false)
    {
      Serial.println("Failure to generate this device's Public Key");
      Serial.println();
    }
  }
}
