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

uint8_t token[32]; // time to live token, created randomly each authentication event

uint8_t signature[64]; // incoming signature from Alice

int headerCount = 0; // used to count incoming "$", when we reach 3 we know it's a good fresh new message.

// Delete this "blank" public key,
// copy/paste Alice's true unique public key from her terminal printout in Example6_Challenge_Alice.

uint8_t AlicesPublicKey[64] = {
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

void setup() {
  Wire.begin();

  Serial.begin(115200); // debug
  Serial1.begin(115200); // Alice's TX1/RX1 <<-->> Bob's TX1/RX1

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
      headerCount = 0; // reset
      input = 0; // reset
      Serial.println("Message Received!");
      Serial.println();

      Serial.print("Creating a new random TTL-token now...");

      // update library instance public variable.
      atecc.updateRandom32Bytes();

      // copy from library public variable into our local variable
      // also send each byte  to Alice.
      for (int i = 0 ; i < 32 ; i++)
      {
        token[i] = atecc.random32Bytes[i]; // store locally
        Serial1.write(token[i]); // send to alice
      }

      Serial.println("and sent."); // debug

      printToken();

      // wait for Alice to respond with her signature
      // wait timeout ms, or entire message to come into Serial1 buffer (64 bytes at 115200).
      // with initial testing it would take Alice about 98 ms to respond, so 150 is good.
      int timeout = 150;
      while (timeout && (Serial1.available() < 64))
      {
        delay(1);
        timeout--;
      }

      if (timeout == 0)
      {
        Serial.print("Timed Out. Token is NO LONGER VALID.");
        for (int i = 0 ; i < 32 ; i++) token[i] = 0; // zero out token, to ensure future failures.
      }
      else // we didn't time out yet, so let's pull in the signature from Alice.
      {
        Serial.print("response time: ");
        Serial.println(150 - timeout);
        Serial.println();

        for (int i = 0 ; i < 64 ; i++) signature[i] = Serial1.read(); // read in signature.

        printSignature();

        // Let's verirfy!
        if (atecc.verifySignature(token, signature, AlicesPublicKey))
        {
          Serial.println("Success! Signature Verified.");
        }
        else Serial.println("Verification failure.");
      }
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

void printToken()
{
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
