/*
  This is a library written for the ATECCX08A Criptographic Co-Processor (QWIIC).

  Written by Pete Lewis @ SparkFun Electronics, August 5th, 2019

  The IC uses I2C and 1-wire to communicat. This library only supports I2C.

  https://github.com/sparkfun/SparkFun_ATECCX08A_Arduino_Library

  Do you like this library? Help support SparkFun. Buy a board!

  Development environment specifics:
  Arduino IDE 1.8.1

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "SparkFun_ATECCX08A_Arduino_Library.h"

//Returns false if IC does not respond

#if defined(__MK64FX512__) || defined(__MK66FX1M0__)
	//Teensy 3.6
boolean ATECCX08A::begin(uint8_t i2caddr, i2c_t3 &wirePort)
{
  //Bring in the user's choices
  _i2cPort = &wirePort; //Grab which port the user wants us to use

  _i2caddr = i2caddr;

  return ( wakeUp() ); // see if the IC wakes up properly, return responce.
}
#else

boolean ATECCX08A::begin(uint8_t i2caddr, TwoWire &wirePort)
{
  //Bring in the user's choices
  _i2cPort = &wirePort; //Grab which port the user wants us to use

  _i2caddr = i2caddr;

  return ( wakeUp() ); // see if the IC wakes up properly, return responce.
}
#endif


/** \brief 

	wakeUp()
	
	This function wakes up the ATECCX08a IC
	Returns TRUE if the IC responds with correct verification
	Message (0x04, 0x11, 0x33, 0x44) 
	The actual status byte we are looking for is the 0x11.
	The complete message is as follows:
	COUNT, DATA, CRC[0], CRC[1].
	0x11 means that it received the wake condition and is goat to goo.
	
	Note, in most SparkFun Arduino Libraries, we would use a different
	function called isConnected(), but because this IC will ACK and
	respond with a status, we are gonna use wakeUp() for the same purpose.
*/

boolean ATECCX08A::wakeUp()
{
  _i2cPort->beginTransmission(0x00); // set up to write to address "0x00",
  // This creates a "wake condition" where SDA is held low for at least tWLO
  // tWLO means "wake low duration" and must be at least 60 uSeconds (which is acheived by writing 0x00 at 100KHz I2C)
  _i2cPort->endTransmission(); // actually send it

  delayMicroseconds(1500); // required for the IC to actually wake up.
  // 1500 uSeconds is minimum and known as "Wake High Delay to Data Comm." tWHI, and SDA must be high during this time.

  // Now let's read back from the IC and see if it reports back good things.
  countGlobal = 0; 
  if(receiveResponseData(4) == false) return false;
  if(checkCount() == false) return false;
  if(checkCrc() == false) return false;
  if(inputBuffer[1] == 0x11) return true;   // If we hear a "0x11", that means it had a successful wake up.
  else return false;
}

/** \brief

	idleMode()
	
	The ATECCX08A goes into the idle mode and ignores all subsequent I/O transitions
	until the next wake flag. The contents of TempKey and RNG Seed registers are retained.
	Idle Power Supply Current: 800uA.
	Note, it will automatically go into sleep mode after watchdog timer has been reached (1.3-1.7sec).
*/

void ATECCX08A::idleMode()
{
  _i2cPort->beginTransmission(_i2caddr); // set up to write to address
  _i2cPort->write(WORD_ADDRESS_VALUE_IDLE); // enter idle command (aka word address - the first part of every communication to the IC)
  _i2cPort->endTransmission(); // actually send it  
}

/** \brief

	getInfo()
	
	This function sends the INFO Command and listens for the correct version (0x50) within the response.
	The Info command has a mode parameter, and in this function we are using the "Revision" mode (0x00)
	At the time of data sheet creation the Info command will return 0x00 0x00 0x50 0x00. For
	all versions of the ECC508A the 3rd byte will always be 0x50. The fourth byte will indicate the
	silicon revision.
*/

boolean ATECCX08A::getInfo()
{
  // build packet array to complete a communication to IC
  // It expects to see word address, count, command, param1, param2a, param2b, CRC[0], CRC[1].
  uint8_t count = 0x07;
  uint8_t command = COMMAND_OPCODE_INFO;
  uint8_t param1 = 0x00; // "Revision mode"
  uint8_t param2a = 0x00;
  uint8_t param2b = 0x00;

  // update CRCs
  uint8_t packet_to_CRC[] = {count, command, param1, param2a, param2b};
  atca_calculate_crc((count - 2), packet_to_CRC); // count includes crc[0] and crc[1], so we must subtract 2 before creating crc
  //Serial.println(crc[0], HEX);
  //Serial.println(crc[1], HEX);

  // create complete message using newly created/updated crc values
  byte complete_message[9] = {WORD_ADDRESS_VALUE_COMMAND, count, command, param1, param2a, param2b, crc[0], crc[1]};

  wakeUp();
 
  _i2cPort->beginTransmission(_i2caddr);
  _i2cPort->write(complete_message, 8);
  _i2cPort->endTransmission();

  delay(1); // time for IC to process command and exectute
  
    // Now let's read back from the IC and see if it reports back good things.
  countGlobal = 0; 
  if(receiveResponseData(7, true) == false) return false;
  idleMode();
  if(checkCount() == false) return false;
  if(checkCrc() == false) return false;
  if(inputBuffer[3] == 0x50) return true;   // If we hear a "0x50", that means it had a successful version response.
  else return false;
}

/** \brief

	lockConfig()
	
	This function sends the LOCK Command with the configuration zone parameter, 
	and listens for success response (0x00).
*/

boolean ATECCX08A::lockConfig()
{
  return lock(LOCK_ZONE_CONFIG);
}

/** \brief

	readConfigZone()
	
	This function reads the entire configuration zone EEPROM memory on the device.
	It stores them for vewieing in a large array called configZone[128].
*/

boolean ATECCX08A::readConfigZone(boolean debug)
{
  // read block 0, the first 32 bytes of config zone into inputBuffer
  read(ZONE_CONFIG, ADDRESS_CONFIG_BLOCK_0, 32); 
  
  // copy current contents of inputBuffer into configZone[] (for later viewing/comparing)
  memcpy(&configZone[0], &inputBuffer[1], 32);
  
  read(ZONE_CONFIG, ADDRESS_CONFIG_BLOCK_1, 32); 	// read block 1
  memcpy(&configZone[32], &inputBuffer[1], 32); 	// copy block 1
  
  read(ZONE_CONFIG, ADDRESS_CONFIG_BLOCK_2, 32); 	// read block 2
  memcpy(&configZone[64], &inputBuffer[1], 32); 	// copy block 2
  
  read(ZONE_CONFIG, ADDRESS_CONFIG_BLOCK_3, 32); 	// read block 3
  memcpy(&configZone[96], &inputBuffer[1], 32); 	// copy block 3  
  
  if(debug)
  {
    Serial.println("configZone: ");
    for (int i = 0; i < sizeof(configZone) ; i++)
    {
      Serial.print(i);
	  Serial.print(": 0x");
	  if((configZone[i] >> 4) == 0) Serial.print("0"); // print preceeding high byte if it's zero
	  Serial.print(configZone[i], HEX); 
	  Serial.print(" \t0b");
	  for(int bit = 7; bit >= 0; bit--) Serial.print(bitRead(configZone[i],bit)); // print binary WITH preceding '0' bits
	  Serial.println();
    }
    Serial.println();
  }
  
  return true;
}

/** \brief

	lockDataAndOTP()
	
	This function sends the LOCK Command with the Data and OTP (one-time-programming) zone parameter, 
	and listens for success response (0x00).
*/

boolean ATECCX08A::lockDataAndOTP()
{
  return lock(LOCK_ZONE_DATA_AND_OTP);
}

/** \brief

	lock(byte zone)
	
	This function sends the LOCK Command using hte argument zone as parameter 1, 
	and listens for success response (0x00).
*/

boolean ATECCX08A::lock(byte zone)
{
  // build packet array to complete a communication to IC
  // It expects to see word address, count, command, param1, param2a, param2b, CRC[0], CRC[1].
  uint8_t count = 0x07;
  uint8_t command = COMMAND_OPCODE_LOCK;
  uint8_t param1 = zone;
  uint8_t param2a = 0x00;
  uint8_t param2b = 0x00;

  // update CRCs
  uint8_t packet_to_CRC[] = {count, command, param1, param2a, param2b};
  atca_calculate_crc((count - 2), packet_to_CRC); // count includes crc[0] and crc[1], so we must subtract 2 before creating crc
  //Serial.println(crc[0], HEX);
  //Serial.println(crc[1], HEX);

  // create complete message using newly created/updated crc values
  byte complete_message[9] = {WORD_ADDRESS_VALUE_COMMAND, count, command, param1, param2a, param2b, crc[0], crc[1]};

  wakeUp();
 
  _i2cPort->beginTransmission(_i2caddr);
  _i2cPort->write(complete_message, 8);
  _i2cPort->endTransmission();

  delay(32); // time for IC to process command and exectute
  
  // Now let's read back from the IC and see if it reports back good things.
  countGlobal = 0; 
  if(receiveResponseData(4, true) == false) return false;
  idleMode();
  if(checkCount() == false) return false;
  if(checkCrc() == false) return false;
  if(inputBuffer[1] == 0x00) return true;   // If we hear a "0x00", that means it had a successful lock
  else return false;
}




/** \brief

	updateRandom32Bytes(boolean debug)
	
    This function pulls a complete random number (all 32 bytes)
    It stores it in a global array called random32Bytes[]
    If you wish to access this global variable and use as a 256 bit random number,
    then you will need to access this array and combine it's elements as you wish.
    In order to keep compatibility with ATmega328 based arduinos,
    We have offered some other functions that return variables more usable (i.e. byte, int, long)
    They are getRandomByte(), getRandomInt(), and getRandomLong().
*/

boolean ATECCX08A::updateRandom32Bytes(boolean debug)
{
  // build packet array to complete a communication to IC
  // It expects to see word address, count, command, param1, param2, CRC1, CRC2
  uint8_t count = 0x07;
  uint8_t command = COMMAND_OPCODE_RANDOM;
  uint8_t param1 = 0x00;
  uint8_t param2a = 0x00;
  uint8_t param2b = 0x00;

  // update CRCs
  uint8_t packet_to_CRC[] = {count, command, param1, param2a, param2b};
  atca_calculate_crc((count - 2), packet_to_CRC); // count includes crc[0] and crc[1], so we must subtract 2 before creating crc
  //Serial.println(crc[0], HEX);
  //Serial.println(crc[1], HEX);

  // create complete message using newly created/updated crc values
  byte complete_message[9] = {WORD_ADDRESS_VALUE_COMMAND, count, command, param1, param2a, param2b, crc[0], crc[1]};

  wakeUp();
  
  _i2cPort->beginTransmission(_i2caddr);
  _i2cPort->write(complete_message, 8);
  _i2cPort->endTransmission();

  delay(23); // time for IC to process command and exectute

  // Now let's read back from the IC. This will be 35 bytes of data (count + 32_data_bytes + crc[0] + crc[1])

  if(receiveResponseData(35, debug) == false) return false;
  idleMode();
  if(checkCount(debug) == false) return false;
  if(checkCrc(debug) == false) return false;
  
  
  // update random32Bytes[] array
  // we don't need the count value (which is currently the first byte of the inputBuffer)
  for (int i = 0 ; i < 32 ; i++) // for loop through to grab all but the first position (which is "count" of the message)
  {
    random32Bytes[i] = inputBuffer[i + 1];
  }

  if(debug)
  {
    Serial.print("random32Bytes: ");
    for (int i = 0; i < sizeof(random32Bytes) ; i++)
    {
      Serial.print(random32Bytes[i], HEX);
      Serial.print(",");
    }
    Serial.println();
  }
  
  return true;
}

/** \brief

	getRandomByte(boolean debug)
	
    This function returns a random byte.
	It calls updateRandom32Bytes(), then uses the first byte in that array for a return value.
*/

byte ATECCX08A::getRandomByte(boolean debug)
{
  updateRandom32Bytes(debug);
  return random32Bytes[0];
}

/** \brief

	getRandomInt(boolean debug)
	
    This function returns a random Int.
	It calls updateRandom32Bytes(), then uses the first 2 bytes in that array for a return value.
	It bitwize ORS the first two bytes of the array into the return value.
*/

int ATECCX08A::getRandomInt(boolean debug)
{
  updateRandom32Bytes(debug);
  int return_val;
  return_val = random32Bytes[0]; // store first randome byte into return_val
  return_val <<= 8; // shift it over, to make room for the next byte
  return_val |= random32Bytes[1]; // "or in" the next byte in the array
  return return_val;
}

/** \brief

	getRandomLong(boolean debug)
	
    This function returns a random Long.
	It calls updateRandom32Bytes(), then uses the first 4 bytes in that array for a return value.
	It bitwize ORS the first 4 bytes of the array into the return value.
*/

long ATECCX08A::getRandomLong(boolean debug)
{
  updateRandom32Bytes(debug);
  long return_val;
  return_val = random32Bytes[0]; // store first randome byte into return_val
  return_val <<= 8; // shift it over, to make room for the next byte
  return_val |= random32Bytes[1]; // "or in" the next byte in the array
  return_val <<= 8; // shift it over, to make room for the next byte
  return_val |= random32Bytes[2]; // "or in" the next byte in the array
  return_val <<= 8; // shift it over, to make room for the next byte
  return_val |= random32Bytes[3]; // "or in" the next byte in the array
  return return_val;
}



/** \brief

	receiveResponseData(uint8_t length, boolean debug)
	
	This function receives messages from the ATECCX08a IC (up to 32 Bytes)
	It will return true if it receives the correct amount of data and good CRCs.
	What we hear back from the IC is always formatted with the following series of bytes:
	COUNT, DATA, CRC[0], CRC[1]
	Note, the count number includes itself, the num of data bytes, and the two CRC bytes in the total, 
	so a simple response message from the IC that indicates that it heard the wake 
	condition properly is like so:
	EXAMPLE Wake success response: 0x04, 0x11, 0x33, 0x44
	It needs length argument:
	length: length of data to receive (includes count + DATA + 2 crc bytes)
*/

boolean ATECCX08A::receiveResponseData(uint8_t length, boolean debug)
{	

  // pull in data 32 bytes at at time. (necessary to avoid overflow on atmega328)
  // if length is less than or equal to 32, then just pull it in.
  // if length is greater than 32, then we must first pull in 32, then pull in remainder.
  // lets use length as our tracker and we will subtract from it as we pull in data.
  
  countGlobal = 0; // reset for each new message (most important, like wensleydale at a cheese party)
  cleanInputBuffer();
  byte requestAttempts = 0; // keep track of how many times we've attempted to request, to break out if necessary
  
  while(length)
  {
    byte requestAmount; // amount of bytes to request, needed to pull in data 32 bytes at a time (for AVR atmega328s)  
	if(length > 32) requestAmount = 32; // as we have more than 32 to pull in, keep pulling in 32 byte chunks
	else requestAmount = length; // now we're ready to pull in the last chunk.
	_i2cPort->requestFrom(_i2caddr, requestAmount);    // request bytes from slave
	requestAttempts++;

	while (_i2cPort->available())   // slave may send less than requested
	{
	  inputBuffer[countGlobal] = _i2cPort->read();    // receive a byte as character
	  length--; // keep this while loop active until we've pulled in everything
	  countGlobal++; // keep track of the count of the entire message.
	}  
	if(requestAttempts == 256) break; // this probably means that the device is not responding.
  }

  if(debug)
  {
    Serial.print("inputBuffer: ");
	for (int i = 0; i < countGlobal ; i++)
	{
	  Serial.print(inputBuffer[i], HEX);
	  Serial.print(",");
	}
	Serial.println();	  
  }
  return true;
}

/** \brief

	checkCount(boolean debug)
	
	This function checks that the count byte received in the most recent message equals countGlobal
	Use it after you call receiveResponseData as many times as you need,
	and then finally you can check the count of the complete message.
*/

boolean ATECCX08A::checkCount(boolean debug)
{
  if(debug)
  {
    Serial.print("countGlobal: 0x");
	Serial.println(countGlobal, HEX);
	Serial.print("count heard from IC (inpuBuffer[0]): 0x");
    Serial.println(inputBuffer[0], HEX);
  }
  // Check count; the first byte sent from IC is count, and it should be equal to the actual message count
  if(inputBuffer[0] != countGlobal) 
  {
	Serial.println("Message Count Error");
	return false;
  }  
  return true;
}

/** \brief

	checkCrc(boolean debug)
	
	This function checks that the CRC bytes received in the most recent message equals a calculated CRCs
	Use it after you call receiveResponseData as many times as you need,
	and then finally you can check the CRCs of the complete message.
*/

boolean ATECCX08A::checkCrc(boolean debug)
{
  // Check CRC[0] and CRC[1] are good to go.
  
  atca_calculate_crc(countGlobal-2, inputBuffer);   // first calculate it
  
  if(debug)
  {
    Serial.print("CRC[0] Calc: 0x");
	Serial.println(crc[0], HEX);
	Serial.print("CRC[1] Calc: 0x");
    Serial.println(crc[1], HEX);
  }
  
  if( (inputBuffer[countGlobal-1] != crc[1]) || (inputBuffer[countGlobal-2] != crc[0]) )   // then check the CRCs.
  {
	Serial.println("Message CRC Error");
	return false;
  }
  
  return true;
}

/** \brief

	atca_calculate_crc(uint8_t length, uint8_t *data)
	
    This function calculates CRC.
    It was copied directly from the App Note provided from Microchip.
    Note, it seems to be their own unique type of CRC cacluation.
    View the entire app note here:
    http://ww1.microchip.com/downloads/en/AppNotes/Atmel-8936-CryptoAuth-Data-Zone-CRC-Calculation-ApplicationNote.pdf
    \param[in] length number of bytes in buffer
    \param[in] data pointer to data for which CRC should be calculated
*/

void ATECCX08A::atca_calculate_crc(uint8_t length, uint8_t *data)
{
  uint8_t counter;
  uint16_t crc_register = 0;
  uint16_t polynom = 0x8005;
  uint8_t shift_register;
  uint8_t data_bit, crc_bit;
  for (counter = 0; counter < length; counter++) {
    for (shift_register = 0x01; shift_register > 0x00; shift_register <<= 1) {
      data_bit = (data[counter] & shift_register) ? 1 : 0;
      crc_bit = crc_register >> 15;
      crc_register <<= 1;
      if (data_bit != crc_bit)
        crc_register ^= polynom;
    }
  }
  crc[0] = (uint8_t) (crc_register & 0x00FF);
  crc[1] = (uint8_t) (crc_register >> 8);
}


/** \brief

	cleanInputBuffer()
	
    This function sets the entire inputBuffer to zeros.
	It is helpful for debugging message/count/CRCs errors.
*/

void ATECCX08A::cleanInputBuffer()
{
  for (int i = 0; i < sizeof(inputBuffer) ; i++)
  {
    inputBuffer[i] = 0xFF;
  }
}

boolean ATECCX08A::createNewKeyPair(byte slot)
{
  // build packet array to complete a communication to IC
  // It expects to see word address, count, command, param1, param2, CRC1, CRC2
  uint8_t count = 0x07;
  uint8_t command = COMMAND_OPCODE_GENKEY;
  uint8_t param1 = GENKEY_MODE_NEW_PRIVATE;
  uint8_t param2a = 0x00;
  uint8_t param2b = slot; // defult is 0

  // update CRCs
  uint8_t packet_to_CRC[] = {count, command, param1, param2a, param2b};
  atca_calculate_crc((count - 2), packet_to_CRC); // count includes crc[0] and crc[1], so we must subtract 2 before creating crc
  //Serial.println(crc[0], HEX);
  //Serial.println(crc[1], HEX);

  // create complete message using newly created/updated crc values
  byte complete_message[9] = {WORD_ADDRESS_VALUE_COMMAND, count, command, param1, param2a, param2b, crc[0], crc[1]};

  wakeUp();
  
  _i2cPort->beginTransmission(_i2caddr);
  _i2cPort->write(complete_message, 8);
  _i2cPort->endTransmission();

  delay(115); // time for IC to process command and exectute

  // Now let's read back from the IC.
  
  if(receiveResponseData(64 + 2 + 1, true) == false) return false; // public key (64), plus crc (2), plus count (1)
  idleMode();
  boolean checkCountResult = checkCount(true);
  boolean checkCrcResult = checkCrc(true);
  
  // update publicKey64Bytes[] array
  // we don't need the count value (which is currently the first byte of the inputBuffer)
  for (int i = 0 ; i < 64 ; i++) // for loop through to grab all but the first position (which is "count" of the message)
  {
    publicKey64Bytes[i] = inputBuffer[i + 1];
  }
  
  Serial.print("publicKey64Bytes: ");
  for (int i = 0; i < sizeof(publicKey64Bytes) ; i++)
  {
    Serial.print(publicKey64Bytes[i], HEX);
    Serial.print(",");
  }
  Serial.println();
  
  
  if( (checkCountResult == false) || (checkCrcResult == false) ) return false;
  
  return true;
}

boolean ATECCX08A::loadTempKey(uint8_t *data)
{
  // We will use the NONCE command in passthrough mode to load tempKey with our message.
  // Note, the datasheet warns that this does not provide protection agains replay attacks,
  // but we will protect again this because our server is going to send us it's own unique NONCE,
  // when it requests data, and we will add this into our message.
  uint8_t count = 0x27; // 7 for standard command + 32 bytes of mesage = 39 (or 0x27)
  uint8_t command = COMMAND_OPCODE_NONCE;
  uint8_t param1 = NONCE_MODE_PASSTHROUGH;
  uint8_t param2a = 0x00;
  uint8_t param2b = 0x00;

  uint8_t complete_message_length = (8 + 32);
  uint8_t complete_message[complete_message_length];
  complete_message[0] = WORD_ADDRESS_VALUE_COMMAND; // word address value (type command)
  complete_message[1] = complete_message_length-1; 						// count
  complete_message[2] = COMMAND_OPCODE_NONCE; 		// command
  complete_message[3] = NONCE_MODE_PASSTHROUGH;		// param1
  complete_message[4] = 0x00;						// param2a
  complete_message[5] = 0x00;						// param2b
  memcpy(&complete_message[6], &data[0], 32);	// data
  
  
  // update CRCs
  uint8_t packet_to_CRC[5+32];
  // append data
  memcpy(&packet_to_CRC[0], &complete_message[1], (5+32));
  
      Serial.println("packet_to_CRC: ");
    for (int i = 0; i < sizeof(packet_to_CRC) ; i++)
    {
	  Serial.print(packet_to_CRC[i], HEX);
	  Serial.print(",");
    }
    Serial.println();
  
  atca_calculate_crc((5+32), packet_to_CRC); // count includes crc[0] and crc[1], so we must subtract 2 before creating crc
  Serial.println(crc[0], HEX);
  Serial.println(crc[1], HEX);

  // append crcs
  memcpy(&complete_message[6+32], &crc[0], 2);  

  wakeUp();
  
  Serial.print("complete_message_length: ");
  Serial.println(complete_message_length);
  
  // begin I2C sending - 
  
  _i2cPort->beginTransmission(_i2caddr);
  _i2cPort->write(complete_message, complete_message_length); 
  _i2cPort->endTransmission();

  // end I2C sending - 

  delay(7); // time for IC to process command and exectute

  // Now let's read back from the IC.
  
  if(receiveResponseData(4, true) == false) return false; // responds with "0x00" if NONCE executed properly
  idleMode();
  boolean checkCountResult = checkCount(true);
  boolean checkCrcResult = checkCrc(true);
  
  if( (checkCountResult == false) || (checkCrcResult == false) ) return false;
  
  if(inputBuffer[1] == 0x00) return true;   // If we hear a "0x00", that means it had a successful nonce
  else return false;

}

boolean ATECCX08A::readKeySlot(byte slot)
{

}

boolean ATECCX08A::storeKeyInSlot(byte slot)
{

}

boolean ATECCX08A::read(byte zone, byte address, byte length, boolean debug)
{
  // build packet array to complete a communication to IC
  // It expects to see word address, count, command, param1, param2, CRC1, CRC2
  uint8_t count = 0x07;
  uint8_t command = COMMAND_OPCODE_READ;
  uint8_t param1 = zone;
  uint8_t param2a = address;
  uint8_t param2b = 0x00;
  
  // adjust param1 as needed for whether it's 4 or 32 bytes length read
  // bit 7 of param1 needs to be set correctly 
  // (0 = 4 Bytes are read) 
  // (1 = 32 Bytes are read)
  if(length == 32) 
  {
	param1 |= 0b10000000; // set bit 7
  }
  else if(length == 4)
  {
	param1 &= ~0b10000000; // clear bit 7
  }
  else
  {
	return 0; // invalid length, abort.
  }

  // update CRCs
  uint8_t packet_to_CRC[] = {count, command, param1, param2a, param2b};
  atca_calculate_crc((count - 2), packet_to_CRC); // count includes crc[0] and crc[1], so we must subtract 2 before creating crc
  //Serial.println(crc[0], HEX);
  //Serial.println(crc[1], HEX);

  // create complete message using newly created/updated crc values
  byte complete_message[9] = {WORD_ADDRESS_VALUE_COMMAND, count, command, param1, param2a, param2b, crc[0], crc[1]};

  wakeUp();
  
  _i2cPort->beginTransmission(_i2caddr);
  _i2cPort->write(complete_message, 8);
  _i2cPort->endTransmission();

  delay(1); // time for IC to process command and exectute

  // Now let's read back from the IC. 
  
  if(receiveResponseData(length + 3, debug) == false) return false;
  idleMode();
  if(checkCount(debug) == false) return false;
  if(checkCrc(debug) == false) return false;
  
  return true;
}

boolean ATECCX08A::write(byte zone, byte address, byte length, const byte data[])
{

  // adjust zone as needed for whether it's 4 or 32 bytes length read
  // bit 7 of param1 needs to be set correctly 
  // (0 = 4 Bytes are read) 
  // (1 = 32 Bytes are read)
  if(length == 32) 
  {
	zone |= 0b10000000; // set bit 7
  }
  else if(length == 4)
  {
	zone &= ~0b10000000; // clear bit 7
  }
  else
  {
	return 0; // invalid length, abort.
  }
  
  uint8_t complete_message_length = (8 + length);
  uint8_t complete_message[complete_message_length];
  complete_message[0] = WORD_ADDRESS_VALUE_COMMAND; // word address value (type command)
  complete_message[1] = complete_message_length-1; 						// count
  complete_message[2] = COMMAND_OPCODE_WRITE; 		// command
  complete_message[3] = zone;						// param1
  complete_message[4] = address;					// param2a
  complete_message[5] = 0x00;						// param2b
  memcpy(&complete_message[6], &data[0], length);	// data
  
  
  // update CRCs
  uint8_t packet_to_CRC[5+length];
  // append data
  memcpy(&packet_to_CRC[0], &complete_message[1], (5+length));
  
      Serial.println("packet_to_CRC: ");
    for (int i = 0; i < sizeof(packet_to_CRC) ; i++)
    {
	  Serial.print(packet_to_CRC[i], HEX);
	  Serial.print(",");
    }
    Serial.println();
  
  atca_calculate_crc((5+length), packet_to_CRC); // count includes crc[0] and crc[1], so we must subtract 2 before creating crc
  Serial.println(crc[0], HEX);
  Serial.println(crc[1], HEX);

  // append crcs
  memcpy(&complete_message[6+length], &crc[0], 2);  

  wakeUp();
  
  Serial.print("complete_message_length: ");
  Serial.println(complete_message_length);
  
  // begin I2C sending - 
  
  _i2cPort->beginTransmission(_i2caddr);
  _i2cPort->write(complete_message, complete_message_length); 
  _i2cPort->endTransmission();

  // end I2C sending - 

  delay(26); // time for IC to process command and exectute
  
  // Now let's read back from the IC and see if it reports back good things.
  countGlobal = 0; 
  if(receiveResponseData(4, true) == false) return false;
  idleMode();
  if(checkCount() == false) return false;
  if(checkCrc() == false) return false;
  if(inputBuffer[1] == 0x00) return true;   // If we hear a "0x00", that means it had a successful write
  else return false;
}

boolean ATECCX08A::createSignature(uint8_t slot)
{
  uint8_t count = 0x07;
  uint8_t command = COMMAND_OPCODE_SIGN;
  uint8_t param1 = SIGN_MODE_TEMPKEY;
  uint8_t param2a = 0x00;
  uint8_t param2b = slot; // default is 0

  // update CRCs
  uint8_t packet_to_CRC[] = {count, command, param1, param2a, param2b};
  atca_calculate_crc((count - 2), packet_to_CRC); // count includes crc[0] and crc[1], so we must subtract 2 before creating crc
  //Serial.println(crc[0], HEX);
  //Serial.println(crc[1], HEX);

  // create complete message using newly created/updated crc values
  byte complete_message[9] = {WORD_ADDRESS_VALUE_COMMAND, count, command, param1, param2a, param2b, crc[0], crc[1]};

  wakeUp();
  
  _i2cPort->beginTransmission(_i2caddr);
  _i2cPort->write(complete_message, 8);
  _i2cPort->endTransmission();

  delay(50); // time for IC to process command and exectute

  // Now let's read back from the IC.
  
  if(receiveResponseData(64 + 2 + 1, true) == false) return false; // signature (64), plus crc (2), plus count (1)
  idleMode();
  boolean checkCountResult = checkCount(true);
  boolean checkCrcResult = checkCrc(true);
  
  // update signature[] array
  // we don't need the count value (which is currently the first byte of the inputBuffer)
  for (int i = 0 ; i < 64 ; i++) // for loop through to grab all but the first position (which is "count" of the message)
  {
    signature[i] = inputBuffer[i + 1];
  }
  
  Serial.print("signature: ");
  for (int i = 0; i < sizeof(signature) ; i++)
  {
    Serial.print(signature[i], HEX);
    Serial.print(",");
  }
  Serial.println();
  
  
  if( (checkCountResult == false) || (checkCrcResult == false) ) return false;
  
  return true;
}


boolean ATECCX08A::verifySignature(uint8_t *message, uint8_t *signature, uint8_t slot, uint8_t type)
{
  return verify(message, signature, NULL, slot, type); // public key arg is ignored when using interally stored keys
}

boolean ATECCX08A::verifySignatureExternal(uint8_t *message, uint8_t *signature, uint8_t *publicKey, uint8_t type)
{
  return verify(message, signature, publicKey, 0, type); // slot is ignored when using external public key
}



boolean ATECCX08A::verify(uint8_t *message, uint8_t *signature, uint8_t *publicKey, uint8_t slot, uint8_t type)
{
  // first, let's load the message into TempKey on the device, this uses NONCE command in passthrough mode.
  boolean loadTempKeyResult = loadTempKey(message);
  if(loadTempKeyResult == false) 
  {
    Serial.println("Load TempKey Failure");
    return false;
  }
  
  // second, let's run the verify command using TempKey as our data, also sending it slot and type (default slot 0, type ECC).

  // check to see if we're external or stored, this will determine if we need to send data3 or data4 (the external public key)
  // this effects our data length.
  
  uint8_t complete_message_length;
  uint8_t mode;
  
  if(publicKey != NULL)
  {
    complete_message_length = (8 + 128);
	mode = VERIFY_MODE_EXTERNAL;
  }
  else
  {
    complete_message_length = (8 + 64); // stored, so we don't have to send public key (64 more bytes).
	mode = VERIFY_MODE_STORED;	
  }
  
  uint8_t complete_message[complete_message_length];
  complete_message[0] = WORD_ADDRESS_VALUE_COMMAND; // word address value (type command)
  complete_message[1] = complete_message_length-1; 						// count
  complete_message[2] = COMMAND_OPCODE_VERIFY; 		// command
  complete_message[3] = mode;						// param1
  complete_message[4] = type;						// param2a
  complete_message[5] = 0x00;						// param2b
  memcpy(&complete_message[6], &signature[0], 64);	// append signature
  
  if(publicKey != NULL)
  {
    memcpy(&complete_message[70], &publicKey[0], 64);	// append external public key
  }
  
  
  // update CRCs
  uint8_t packet_to_CRC[complete_message_length-3]; // minus word address and two crcs.
  // append data
  memcpy(&packet_to_CRC[0], &complete_message[1], (complete_message_length-3));
  
      Serial.println("packet_to_CRC: ");
    for (int i = 0; i < sizeof(packet_to_CRC) ; i++)
    {
	  Serial.print(packet_to_CRC[i], HEX);
	  Serial.print(",");
    }
    Serial.println();
  
  atca_calculate_crc((complete_message_length-3), packet_to_CRC); // count includes crc[0] and crc[1], so we must subtract 2 before creating crc
  Serial.println(crc[0], HEX);
  Serial.println(crc[1], HEX);

  // append crcs
  memcpy(&complete_message[complete_message_length-2], &crc[0], 2);  

  wakeUp();
  
  Serial.print("complete_message_length: ");
  Serial.println(complete_message_length);
  
  // begin I2C sending - 
  
  _i2cPort->beginTransmission(_i2caddr);
  _i2cPort->write(complete_message, complete_message_length); 
  _i2cPort->endTransmission();

  // end I2C sending - 

  delay(58); // time for IC to process command and exectute

  // Now let's read back from the IC.
  
  if(receiveResponseData(4, true) == false) return false;
  idleMode();
  boolean checkCountResult = checkCount(true);
  boolean checkCrcResult = checkCrc(true);
  
  if( (checkCountResult == false) || (checkCrcResult == false) ) return false;
  
  if(inputBuffer[1] == 0x00) return true;   // If we hear a "0x00", that means it had a successful verify
  else return false;
  
}











































