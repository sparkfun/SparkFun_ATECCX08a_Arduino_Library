
#include <SparkFun_ATECCX08a_Arduino_Library.h> //Click here to get the library: http://librarymanager/All#SparkFun_ATECCX08a
#include <Wire.h>

ATECCX08A atecc;

uint8_t message[32];

void setup() {
  SerialUSB.begin(9600);
  printMenu();

  for (uint8_t i = 0 ; i < 32 ; i++)
  {
    message[i] = i;
  }
}

void loop()
{
  if (SerialUSB.available() > 0)
  {
    char input = SerialUSB.read();
    switch (input) {
      case '1':
        SerialUSB.println("Option 1 selected.");
        menu_Begin();
        break;
      case '2':
        SerialUSB.println("Option 2 selected.");
        atecc.getInfo();
        break;
      case '3':
        SerialUSB.println("Option 3 selected.");
        atecc.readConfigZone(true);
        break;
      case '4':
        SerialUSB.println("Option 4 selected.");
        menu_configDataSlot0();
        break;
      case '5':
        SerialUSB.println("Option 5 selected.");
        atecc.lockConfig();
      case '6':
        SerialUSB.println("Option 6 selected.");
        atecc.updateRandom32Bytes();
        break;
      case '7':
        SerialUSB.println("Option 7 selected.");
        menu_PrintRandom32Bytes();
        break;
      case '8':
        SerialUSB.println("Option 8 selected.");
        atecc.createNewKeyPair();
        break;
      case '9':
        SerialUSB.println("Option 9 selected.");
        atecc.loadTempKey(message);
        break;
      case 's':
        SerialUSB.println("Option s selected.");
        atecc.createSignature();
        break;
      case 'v':
        SerialUSB.println("Option v selected.");
        SerialUSB.println(atecc.verifySignatureExternal(message, atecc.signature, atecc.publicKey64Bytes));
        break;
      case 'r':
        SerialUSB.println("Option r selected.");
        menu_Begin();
        atecc.createNewKeyPair();
        atecc.loadTempKey(message);
        atecc.createSignature();
        //message[0] = 0xFF; // mess it up
        SerialUSB.println(atecc.verifySignatureExternal(message, atecc.signature, atecc.publicKey64Bytes));
        //SerialUSB.println(atecc.verifySignature(message, atecc.signature, 0));
        break;        
      default:
        // if nothing else matches, do the default
        // default is optional
        SerialUSB.println("Menu input invalid.");
        break;
    }
    printMenu();
  }
}

void printMenu()
{
  SerialUSB.println();
  SerialUSB.println("SparkFun ATECCX08a Configuration Menu");
  SerialUSB.println("(type a number to select an option)");
  SerialUSB.println("1) atecc.begin()");
  SerialUSB.println("2) atecc.getInfo()");
  SerialUSB.println("3) atecc.readConfigZone()");
  SerialUSB.println("4) Configure DATA SLOT 1 to accept non-ECC keys");
  SerialUSB.println("5) atecc.lockConfig()");
  SerialUSB.println("6) atecc.updateRandom32Bytes()");
  SerialUSB.println("7) Print current local array atecc.random32Bytes[]");
  SerialUSB.println("8) Create Key Pair (private and public).");
  SerialUSB.println("9) Load TempKey with message[].");
  SerialUSB.println("s) Create Signature of TempKey using private key in slot 0.");

}

void menu_Begin()
{
  Wire.begin();

  if (atecc.begin() == true)
  {
    SerialUSB.println("Successful wakeUp(). I2C connections are good.");
  }
  else
  {
    SerialUSB.println("Device not found. Check wiring.");
    while (1); // stall out forever
  }
}

void menu_PrintRandom32Bytes()
{
  SerialUSB.print("random32Bytes: ");
  for (int i = 0; i < sizeof(atecc.random32Bytes) ; i++)
  {
    SerialUSB.print(atecc.random32Bytes[i], HEX);
    SerialUSB.print(",");
  }
  SerialUSB.println();
}

void menu_configDataSlot0()
{
  const byte data[] = {0x3F, 0x00, 0x3F, 0x00}; // 0x3F sets the keyconfig.keyType for slots 0 and 1 to NON-ecc types (111) datasheet pg 20
  SerialUSB.println(atecc.write(ZONE_CONFIG, ADDRESS_CONFIG_BLOCK_3, 4, data));
}


void menu_WriteToDataSlot0()
{
  SerialUSB.println(atecc.write(ZONE_DATA, 0x00, 32, atecc.random32Bytes)); // address is zero for slot 0 (block 0). Note, currently write does not support other blocks
}

void menu_ReadDataSlot0()
{
  atecc.cleanInputBuffer();
  SerialUSB.println(atecc.read(ZONE_DATA, 0x00, 32)); // address is zero for slot 0 (block 0). Note, currently read does not support other blocks
  SerialUSB.print("inputBuffer[]: ");
  for (int i = 0; i < 32 ; i++)
  {
    SerialUSB.print(atecc.inputBuffer[i], HEX);
    SerialUSB.print(",");
  }
  SerialUSB.println();
}
