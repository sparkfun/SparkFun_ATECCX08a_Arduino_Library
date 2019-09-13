
#include <SparkFun_ATECCX08a_Arduino_Library.h> //Click here to get the library: http://librarymanager/All#SparkFun_ATECCX08a
#include <Wire.h>

ATECCX08A atecc;

uint8_t message[32];

void setup() {
  Serial.begin(9600);
  printMenu();

  for (uint8_t i = 0 ; i < 32 ; i++)
  {
    message[i] = i;
  }
}

void loop()
{
  if (Serial.available() > 0)
  {
    char input = Serial.read();
    switch (input) {
      case '1':
        Serial.println("Option 1 selected.");
        menu_Begin();
        break;
      case '2':
        Serial.println("Option 2 selected.");
        atecc.getInfo();
        break;
      case '3':
        Serial.println("Option 3 selected.");
        atecc.readConfigZone(true);
        break;
      case '4':
        Serial.println("Option 4 selected.");
        menu_keyConfigSlot9and10();
        break;
      case '5':
        Serial.println("Option 5 selected.");
        atecc.lockConfig();
      case '6':
        Serial.println("Option 6 selected.");
        atecc.updateRandom32Bytes();
        break;
      case '7':
        Serial.println("Option 7 selected.");
        menu_PrintRandom32Bytes();
        break;
      case '8':
        Serial.println("Option 8 selected.");
        atecc.createNewKeyPair();
        break;
      case '9':
        Serial.println("Option 9 selected.");
        atecc.loadTempKey(message);
        break;
      case 's':
        Serial.println("Option s selected.");
        atecc.createSignature();
        break;
      case 'v':
        Serial.println("Option v selected.");
        Serial.println(atecc.verifySignatureExternal(message, atecc.signature, atecc.publicKey64Bytes));
        break;
      case 'z':
        Serial.println("Option z selected.");
        menu_Begin();
        atecc.createNewKeyPair();
        atecc.loadTempKey(message);
        atecc.createSignature();
        //message[0] = 0xFF; // mess it up
        Serial.println(atecc.verifySignatureExternal(message, atecc.signature, atecc.publicKey64Bytes));
        //Serial.println(atecc.verifySignature(message, atecc.signature, 0));
        break;
      case 'w':
        Serial.println("Option w selected.");
        Serial.println(atecc.write(ZONE_DATA, ADDRESS_DATA_SLOT9, 32, atecc.publicKey64Bytes));
        break;
      case 'r':
        Serial.println("Option r selected.");
        Serial.println(atecc.read(ZONE_DATA, ADDRESS_DATA_SLOT9, 32, true));
        break;        
      case 'p':
        Serial.println("Option p selected.");
        menu_printPublicKey();
        break;
      default:
        // if nothing else matches, do the default
        // default is optional
        Serial.println("Menu input invalid.");
        break;
    }
    printMenu();
  }
}

void printMenu()
{
  Serial.println();
  Serial.println("SparkFun ATECCX08a Configuration Menu");
  Serial.println("(type a number to select an option)");
  Serial.println("1) atecc.begin()");
  Serial.println("2) atecc.getInfo()");
  Serial.println("3) atecc.readConfigZone()");
  Serial.println("4) KeyConfig 9 and 10 for public keys.");
  Serial.println("5) atecc.lockConfig()");
  Serial.println("6) atecc.updateRandom32Bytes()");
  Serial.println("7) Print current local array atecc.random32Bytes[]");
  Serial.println("8) Create Key Pair (private and public).");
  Serial.println("9) Load TempKey with message[].");
  Serial.println("s) Create Signature of TempKey using private key in slot 0.");
  Serial.println("z) RUN sequence: createpair, loadtempkey with message, create signature, verify signature");
  Serial.println("w) Write current publicKey64Bytes[] to slot 9.");
  Serial.println("r) Read slot 9.");
  Serial.println("p) Print current local variable publicKey64Bytes[]");

}

void menu_Begin()
{
  Wire.begin();

  if (atecc.begin() == true)
  {
    Serial.println("Successful wakeUp(). I2C connections are good.");
  }
  else
  {
    Serial.println("Device not found. Check wiring.");
    while (1); // stall out forever
  }
}

void menu_PrintRandom32Bytes()
{
  Serial.print("random32Bytes: ");
  for (int i = 0; i < sizeof(atecc.random32Bytes) ; i++)
  {
    Serial.print(atecc.random32Bytes[i], HEX);
    Serial.print(",");
  }
  Serial.println();
}

void menu_keyConfigSlot9and10()
{
  uint8_t data[4];
  data[0] = 0b00110000; // config address 112. Slot9: Lockable, ECC, pubkey can be used without being validated, DOES NOT contain private key - just a public. (default: 0b00110011)
  data[1] = 0b00000000;
  data[2] = 0b00110000; // config address 114. Slot10: Lockable, ECC, pubkey can be used without being validated, DOES NOT contain private key - just a public. (default: 0b00110011)
  data[3] = 0b00000000;
  Serial.println(atecc.write(ZONE_CONFIG, (112 / 4), 4, data)); // divide address by 4 to command properly in a write command.
}


void menu_WriteToDataSlot0()
{
  Serial.println(atecc.write(ZONE_DATA, 0x00, 32, atecc.random32Bytes)); // address is zero for slot 0 (block 0). Note, currently write does not support other blocks
}

void menu_ReadDataSlot0()
{
  atecc.cleanInputBuffer();
  Serial.println(atecc.read(ZONE_DATA, 0x00, 32)); // address is zero for slot 0 (block 0). Note, currently read does not support other blocks
  Serial.print("inputBuffer[]: ");
  for (int i = 0; i < 32 ; i++)
  {
    Serial.print(atecc.inputBuffer[i], HEX);
    Serial.print(",");
  }
  Serial.println();
}

void menu_printPublicKey()
{
  Serial.print("publicKey64Bytes: ");
  for (int i = 0; i < sizeof(atecc.publicKey64Bytes) ; i++)
  {
    Serial.print(atecc.publicKey64Bytes[i], HEX);
    Serial.print(",");
  }
  Serial.println();
}
