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

uint8_t message[32]; // incoming message from Alice
uint8_t signature[64]; // incoming signature from Alice
int headerCount = 0; // used to count incoming "$", when we reach 3 we know it's a good fresh new message.

// Delete this "blank" public key,
// copy/paste Alice's true unique public key from her terminal printout in Example4_Alice.

uint8_t AlicesPublicKey[64] = {
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};


void setup() {
  Wire.begin();

  Serial.begin(115200); // debug
  Serial1.begin(115200); // Alice's TX1 pin -->> Bob's RX1 pin

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

  Serial.println("Hi I'm Bob, I'm listening for incoming messages from Alice on my RX1 pin.");
  Serial.println();
}

void loop()
{
  if (Serial1.available() > 0) // listen on Serial1
  {
    // check for start header
    byte input = Serial1.read();
    //Serial.print(input, HEX);
    if (input == '$') headerCount++;
    if (headerCount == 3)
    {
      delay(100); // wait for entire message to come into Serial1 buffer (96 bytes at 9600 baud).

      headerCount = 0; // reset
      Serial.println("Message Received!");
      Serial.println();

      for (int bytes = 0 ; bytes < 32 ; bytes++) message[bytes] = Serial1.read();

      for (int bytes = 0 ; bytes < 64 ; bytes++) signature[bytes] = Serial1.read();

      printMessage();

      printSignature();

      // Let's verirfy!
      if (atecc.verifySignature(message, signature, AlicesPublicKey)) Serial.println("Success! Signature Verified.");
      else Serial.println("Verification failure.");
    }
  }
}

// print out this devices public key (Alice's Public Key)
// with the array named perfectly for copy/pasting: "AlicesPublicKey"
void printAlicesPublicKey()
{
  Serial.println();
  Serial.println("uint8_t AlicesPublicKey[64] = {");
  for (int i = 0; i < sizeof(AlicesPublicKey) ; i++)
  {
    Serial.print("0x");
    if ((AlicesPublicKey[i] >> 4) == 0) Serial.print("0"); // print preceeding high nibble if it's zero
    Serial.print(AlicesPublicKey[i], HEX);
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