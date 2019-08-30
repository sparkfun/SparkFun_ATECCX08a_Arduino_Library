
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
  atecc.getInfo();
  //atecc.lockConfig();
  //Serial.println(atecc.getRandomByte(true),HEX);
  //Serial.print(atecc.generatePublicKey());
  //Serial.print(atecc.nOnce(true));

  //const byte data[] = {0x3F, 0x00, 0x3F, 0x00}; // 0x3F sets the keyconfig.keyType for slots 1 and 2 to NON-ecc types (111) datasheet pg 20
  //Serial.println(atecc.write(ZONE_CONFIG, ADDRESS_CONFIG_BLOCK_3, 4, data));

  Serial.print(atecc.readConfigZone(true));

  
}

void loop()
{
  delay(1000);
}