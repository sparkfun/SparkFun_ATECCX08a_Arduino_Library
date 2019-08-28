
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
  atecc.lockConfig();
  Serial.println(atecc.getRandomByte(true),HEX);
  
}

void loop()
{
  delay(1000);
}