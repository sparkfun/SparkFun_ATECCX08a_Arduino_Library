/*
  Using the SparkFun Cryptographic Co-processor Breakout ATECC508a (Qwiic)
  By: Pete Lewis
  SparkFun Electronics
  Date: August 5th, 2019
  License: This code is public domain but you can buy me a beer if you use this and we meet someday (Beerware license).

  Feel like supporting our work? Please buy a board from SparkFun!
  https://www.sparkfun.com/products/15573

  ////////////////////////////// 
  /////////////// ABOUT
  ////////////////////////////// 
  
  This example shows how to setup two SparkFun Artemis Boards as a sender (Alice) and a receiver (Bob).
  Alice will sign a message, then send message+signature to Bob.
  Bob will use the message, the signature, and Alice's Public Key to verify everying.

  Alice will send the Message_Signature via Serial1 TX pin.
  Bob will listen to the message+Signature on his Serial1 RX pin.
  The Serial message includes a start token ("$$$"), the message and the signature.
  Note, this requires that your device be configured with SparkFun Standard Configuration settings.

  //////////////////////////////  
  /////////////// HARDWARE
  //////////////////////////////  

  Setup two Artemis boards. Connect USB cables from computer to each.
  We will call these two boards "Alice" and "Bob", for easier understanding.

  Connect Alice to your computer via USB-C cable.
  Connect one Cryptographic Co-processor to Alice.

  Connect Bob to your computer via a second USB-C cable.
  Connect a second Cryptographic Co-processor to Bob.

  Connect the following pins:
  Alice's TX1 pin -->> Bob's RX1 pin.
  Alice's GND pin -->> Bob's GND pin.
  
  //////////////////////////////
  /////////////// SOFTWARE
  //////////////////////////////  
  
  In order for Bob to verify this message+Signature, he will need Alice's publicKey.
  Upload Example4_Alice to Alice, then reset Alice and watch your serial terminal at 115200.
  Alice's publicKey will be printed to the Serial Terminal.
  Copy this into the top of the Receivers Sketch ("Example4_Bob");
  
  Note, using your mouse, you can highlight the text in the serial terminal with Alice's public key,
  But you will need to use "CTRL C" to copy to your computers clipboard.  
  
  Then upload Example4_Bob to the "Bob" Artemis board/setup.
  Reset both Alice and Bob and watch their serial terminals for messages.  
  Type a "y" into Alice's terminal to tell her to send a signed message.
  Watch Bob's terminal, and see when he receives a message from Alice, and then verify's it!

  //////////////////////////////
  /////////////// CONFIGURATION
  //////////////////////////////  
  
  Note, this requires that your device be configured with SparkFun Standard Configuration settings.
  If you haven't already, configure both devices using Example1_Configuration.
  Install artemis in boards manager: http://boardsmanager/All#Sparkfun_artemis
  Plug in your controller board (e.g. Artemis Redboard, Nano, ATP) into your computer with USB cable.
  Connect your Cryptographic Co-processor to your controller board via a qwiic cable.
  Select TOOLS>>BOARD>>"SparkFun Redboard Artemis"
  Select TOOLS>>PORT>> "COM 3" (note, Alice and Bob will each have a unique COM PORT)
  Click upload, and follow prompts on serial monitor at 115200.
*/

#include <SparkFun_ATECCX08a_Arduino_Library.h> //Click here to get the library: http://librarymanager/All#SparkFun_ATECCX08a
#include <Wire.h>

ATECCX08A atecc;

uint8_t message[32] = {
  0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,
  0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F
};

void setup() {
  Wire.begin();

  Serial.begin(115200);   // debug
  Serial1.begin(115200);  // Alice's TX1 pin -->> Bob's RX1 pin

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

  Serial.println("Hi I'm Alice, Would you like me to send a signed message to Bob via my TX1 pin? (y/n)");

  while (Serial.available() == 0); // wait for user input

  if (Serial.read() == 'y')
  {
    Serial.println();
    Serial.println("Okay. I'll send it now.");
    Serial.println();
  }
  else Serial.print("I don't understand.");

  printMessage(); // nice debug to see what you're sending. see function below

  //Let's create a digital signature!
  atecc.createSignature(message); // by default, this uses the private key securely stored and locked in slot 0.

  // Now let's send the message to Bob.
  // this will include three things:
  // (1) start header ("$$$") ASCII "$" = 0x24 HEX
  // (2) message (32 bytes)
  // (3) signature (64 bytes)

  // start header
  Serial1.print("$$$");

  // message
  // note, we use "Serial.write" because we are sending bytes of data (not characters)
  for (int i = 0; i < sizeof(message) ; i++) Serial1.write(message[i]);

  // signature
  // Note, in Example4_Alice we are printing the signature we JUST created,
  // and it lives inside the library as a public array called "atecc.signature"
  for (int i = 0; i < sizeof(atecc.signature) ; i++) Serial1.write(atecc.signature[i]);
}

void loop()
{
  // do nothing.
}

// print out this devices public key (Alice's Public Key)
// with the array named perfectly for copy/pasting: "AlicesPublicKey"
void printAlicesPublicKey()
{
  Serial.println("**Copy/paste the following public key (alice's) into the top of Example4_Bob sketch.**");
  Serial.println("(Bob needs this to verify her signature)");
  Serial.println();
  Serial.println("uint8_t AlicesPublicKey[64] = {");
  for (int i = 0; i < sizeof(atecc.publicKey64Bytes) ; i++)
  {
    Serial.print("0x");
    if ((atecc.publicKey64Bytes[i] >> 4) == 0) Serial.print("0"); // print preceeding high nibble if it's zero
    Serial.print(atecc.publicKey64Bytes[i], HEX);
    if (i != 63) Serial.print(", ");
    if ((63 - i) % 16 == 0) Serial.println();
  }
  Serial.println("};");
  Serial.println();
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

// Note, in Example4_Alice we are printing the signature we JUST created,
// and it lives inside the library as a public array called "atecc.signature"
void printSignature()
{
  Serial.println("uint8_t signature[64] = {");
  for (int i = 0; i < sizeof(atecc.signature) ; i++)
  {
    Serial.print("0x");
    if ((atecc.signature[i] >> 4) == 0) Serial.print("0"); // print preceeding high nibble if it's zero
    Serial.print(atecc.signature[i], HEX);
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
  if (atecc.configLockStatus && atecc.dataOTPLockStatus && atecc.slot0LockStatus)
  {
    if (atecc.generatePublicKey(0, false) == false) // slot 0, debug false (we will print "alice's public Key manually
    {
      Serial.println("Failure to generate Public Key");
      Serial.println();
    }

    // print out this devices public key (Alice's Public Key)
    // with the array named perfectly for copy/pasting: "AlicesPublicKey"
    printAlicesPublicKey();
  }
}