/*
  Using the SparkFun Cryptographic Co-processor Breakout ATECC508a (Qwiic)
  By: Pete Lewis
  SparkFun Electronics
  Date: August 5th, 2019
  License: This code is public domain but you can buy me a beer if you use this and we meet someday (Beerware license).

  Feel like supporting our work? Please buy a board from SparkFun!
  https://www.sparkfun.com/products/15573

  This example shows how to verify a digital ECC signature of a message using an external public key.
  By "external" public key, we mean that the key lives at the top of this sketch in code, not INSIDE the crypto device.
  Note, this requires that your device be configured with SparkFun Standard Configuration settings.

  ***THIS EXAMPLE WILL FAIL AS IS***
  Every SparkFun Cryptographic Co-processor has a unique private/public keypair.
  You must complete Example_2_Sign. This will print out the public key, message and signature.
  Copy/paste them into the top of this sketch, upload, and verify.

  Try changing a byte (or single bit) in either the message or the signature, and it should fail verification.

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

// publicKey, message, and signature come from example 2.
// delete these, and then copy and paste your unique versions here.

uint8_t publicKey[64] = {
  0x25, 0x43, 0x3E, 0xD4, 0xF9, 0xED, 0x8E, 0x8E, 0xC8, 0xAE, 0x4D, 0xE4, 0x02, 0xB2, 0x89, 0x3E,
  0x2B, 0xBD, 0xE8, 0xA3, 0x5A, 0x58, 0x9F, 0xA8, 0x65, 0x26, 0x81, 0x55, 0x17, 0x64, 0x35, 0x9E,
  0x0C, 0x0F, 0x47, 0x00, 0x38, 0xFC, 0xFD, 0xC5, 0xD4, 0xDD, 0xE9, 0x1D, 0xB2, 0xA2, 0x04, 0xB1,
  0x3B, 0x6E, 0x40, 0x30, 0x60, 0xF6, 0x17, 0x3B, 0xA0, 0xBB, 0x1C, 0x4F, 0x9F, 0xE3, 0x0F, 0x4D
};

uint8_t message[32] = {
  0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,
  0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F
};

uint8_t signature[64] = {
  0x32, 0x0E, 0x5A, 0x7D, 0xA7, 0x9A, 0xCF, 0x2B, 0xC8, 0xA9, 0xD1, 0xD9, 0xA3, 0x04, 0x80, 0x65,
  0xA0, 0xFE, 0x8B, 0xD4, 0xD0, 0xFD, 0xED, 0x3C, 0x83, 0x53, 0x70, 0xCB, 0xDD, 0xEE, 0x70, 0x35,
  0xF9, 0xEE, 0x05, 0x1C, 0x6F, 0x60, 0xC2, 0x54, 0x83, 0x69, 0x0B, 0x86, 0x74, 0x35, 0x94, 0xED,
  0x92, 0xED, 0x84, 0x13, 0xF1, 0xC6, 0x4E, 0xF9, 0x51, 0x09, 0x66, 0x7D, 0xC2, 0x64, 0x9F, 0xD3
};

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

  printInfo(); // see function below for library calls and data handling

  // check for configuration
  if (!(atecc.configLockStatus && atecc.dataOTPLockStatus && atecc.slot0LockStatus))
  {
    Serial.print("Device not configured. Please use the configuration sketch.");
    while (1); // stall out forever.
  }

  printMessage(); // nice debug to see what you're verifying. see function below
  printSignature(); // nice debug to see what you're verifying. see function below

  // Let's verirfy!
  if (atecc.verifySignature(message, signature, publicKey)) Serial.println("Success! Signature Verified.");
  else Serial.println("Verification failure.");
}

void loop()
{
  // do nothing.
}

void printMessage()
{
  Serial.println("uint8_t message[32] = {");
  for (int i = 0; i < sizeof(message) ; i++)
  {
    Serial.print("0x");
    if ((message[i] >> 4) == 0) Serial.print("0"); // print preceeding high nibble if it's zero
    Serial.print(message[i], HEX);
    if (i != 31) Serial.print(", ");
    if ((31 - i) % 16 == 0) Serial.println();
  }
  Serial.println("};");
  Serial.println();
}

void printSignature()
{
  Serial.println("uint8_t signature[64] = {");
  for (int i = 0; i < sizeof(signature) ; i++)
  {
    Serial.print("0x");
    if ((signature[i] >> 4) == 0) Serial.print("0"); // print preceeding high nibble if it's zero
    Serial.print(signature[i], HEX);
    if (i != 63) Serial.print(", ");
    if ((63 - i) % 16 == 0) Serial.println();
  }
  Serial.println("};");
  Serial.println();
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
  if (atecc.configLockStatus && atecc.dataOTPLockStatus && atecc.slot0LockStatus) atecc.generatePublicKey();
}
