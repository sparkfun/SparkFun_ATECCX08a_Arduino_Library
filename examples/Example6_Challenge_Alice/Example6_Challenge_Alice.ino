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

  This example shows how to setup a "challenge and response" for authentication between two systems.
  
  It uses two SparkFun Artemis Boards. One as an authentication key (Alice) and one as a controller (Bob).
  For any new authentication cycle, Alice will initiate. User must reset Alice and send a "y" over serial at 115200.
  Bob will create a new random array of 32 bytes (this will be called a token - aka NONCE).
  It is also known as a "time-to-live token", because it will become invalid after a set amount of time.
  Bob sends the token to Alice.
  Alice will sign the token using her private ECC key, then send the signature to Bob.
  Bob will use the token, the signature, and Alice's Public Key to verify everying.
  
  **Bob will also invalidate the token after a set amount of time. For our example it will be 150ms.
  This prevents any attacker from intercepting the message, and hanging on to it for later use.
  So if Alice doesn't respond within the set window of time (150ms), then Bob will clear out the token,
  and Alice must ask for another.
  
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
  Alice's RX1 pin -->> Bob's TX1 pin.
  Alice's GND pin -->> Bob's GND pin.

  //////////////////////////////
  /////////////// SOFTWARE
  //////////////////////////////  

  In order for Bob to verify the token+Signature, he will need Alice's publicKey.
  We need to paste Alice's public key into the top of Bob's sketch.
  First, upload Example6_Alice to Alice, then watch your serial terminal at 115200.
  Alice's publicKey will be printed to the Serial Terminal.
  Copy this into the top of Bob's sketch (Example6_Challenge_Bob.ino).
  
  Note, using your mouse, you can highlight the text in the serial terminal with Alice's public key,
  But you will need to use "CTRL C" to copy to your computers clipboard.
  
  Then upload Example6_Challenge_Bob to the "Bob" Artemis board/setup.
  Reset both Alice and Bob and watch their serial terminals for messages.
  To engage a new authentication cycle, type a "y" into Alice's terminal.
  Ultimately, Bob will attempt to Verify the signature and print results to the terminal.

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

uint8_t token[32]; // not defined here.
// Bob is going to send us a unique token each time we start a new authentication challenge/response.

void setup() {
  Wire.begin();

  Serial.begin(115200);   // debug
  Serial1.begin(115200);  // Alice's TX1/RX1 <<-->> Bob's TX1/RX1

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

  Serial.println("Hi I'm Alice, Would you like me to request a new token from Bob? (y/n)");

  while (Serial.available() == 0); // wait for user input

  if (Serial.read() == 'y')
  {
    Serial.println();
    Serial.println("Okay. Sending request for token now...");
    Serial.println();
  }
  else
  {
    Serial.print("I don't understand.");
    while (1);
  }

  // Send command to request new token
  Serial1.print("$$$"); // send request command

  // wait for Bob to respond with the new token
  // wait timeout ms, or entire token to come into Serial1 bugger (32 bytes at 115200).
  // with initial testing it would take Bob 38 ms to respond, so 50 ms is good.
  int timeout = 50;
  while (timeout && (Serial1.available() < 32))
  {
    delay(1);
    timeout--;
  }

  if (timeout == 0) Serial.print("Timed Out. Bob never responded.");
  else // we didn't time out yet, so let's pull in the token from Bob.
  {
    Serial.print("response time: ");
    Serial.println(50 - timeout);
    
    for (int i = 0 ; i < 32 ; i++) token[i] = Serial1.read(); // read in token from Bob.

    //Let's create a digital signature!
    atecc.createSignature(token); // by default, this uses the private key securely stored and locked in slot 0.

    // Now let's send the signature to Bob.
    // Note, in Example6_Challenge_Alice we are printing the signature we JUST created with token,
    // and it lives inside the library as a public array called "atecc.signature"
    for (int i = 0; i < 64 ; i++) Serial1.write(atecc.signature[i]);

    printToken(); // nice debug to see what token we just sent. see function below
  }
}

void loop()
{
  // do nothing.
}

// print out this devices public key (Alice's Public Key)
// with the array named perfectly for copy/pasting: "AlicesPublicKey"
void printAlicesPublicKey()
{
  Serial.println("**Copy/paste the following public key (alice's) into the top of Example6_Challenge_Bob sketch.**");
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

void printToken()
{
  Serial.println();
  Serial.println("uint8_t token[32] = {");
  for (int i = 0; i < sizeof(token) ; i++)
  {
    Serial.print("0x");
    if ((token[i] >> 4) == 0) Serial.print("0"); // print preceeding high nibble if it's zero
    Serial.print(token[i], HEX);
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
