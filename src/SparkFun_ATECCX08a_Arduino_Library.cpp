/*
  This is a library written for the ATECCX08A Criptographic Co-Processor (QWIIC).

  Written by Pete Lewis @ SparkFun Electronics, August 5th, 2019

  Contributions for SHA256 support by Gili Yankovitch (github @gili-yankovitch), March 2020
  See details of PR here: https://github.com/sparkfun/SparkFun_ATECCX08a_Arduino_Library/pull/3

  The IC uses I2C or 1-wire to communicate. This library only supports I2C.

  https://github.com/sparkfun/SparkFun_ATECCX08A_Arduino_Library

  Do you like this library? Help support SparkFun. Buy a board!

  Development environment specifics:
  Arduino IDE 1.8.10

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "SparkFun_ATECCX08a_Arduino_Library.h"

/** \brief

	begin(uint8_t i2caddr, TwoWire &wirePort, Stream &serialPort)

	returns false if IC does not respond,
	returns true if wake() function is successful

	Note, in most SparkFun Arduino Libraries, begin would call a different
	function called isConnected() to check status on the bus, but because
	this IC will ACK and respond with a status, we are gonna use wakeUp()
	for the same purpose.
*/

bool ATECCX08A::begin(uint8_t i2caddr, TwoWire &wirePort, Stream &serialPort)
{
  //Bring in the user's choices
  _i2cPort = &wirePort; //Grab which port the user wants us to use

  _debugSerial = &serialPort; //Grab which port the user wants us to use

  _i2caddr = i2caddr;

  return ( wakeUp() ); // see if the IC wakes up properly, return responce.
}

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

bool ATECCX08A::wakeUp()
{
  _i2cPort->beginTransmission(0x00); // set up to write to address "0x00",
  // This creates a "wake condition" where SDA is held low for at least tWLO
  // tWLO means "wake low duration" and must be at least 60 uSeconds (which is acheived by writing 0x00 at 100KHz I2C)
  _i2cPort->endTransmission(); // actually send it

  delayMicroseconds(1500); // required for the IC to actually wake up.
  // 1500 uSeconds is minimum and known as "Wake High Delay to Data Comm." tWHI, and SDA must be high during this time.

  // Now let's read back from the IC and see if it reports back good things.
  countGlobal = 0;

  if (!receiveResponseData(RESPONSE_COUNT_SIZE + RESPONSE_SIGNAL_SIZE + CRC_SIZE))
    return false;

  if (!checkCount() || !checkCrc())
    return false;

  // If we hear a "0x11", that means it had a successful wake up.
  if (inputBuffer[RESPONSE_SIGNAL_INDEX] != ATRCC508A_SUCCESSFUL_WAKEUP)
    return false;

  return true;
}

/** \brief

	idleMode()

	The ATECCX08A goes into the idle mode and ignores all subsequent I/O transitions
	until the next wake flag. The contents of TempKey and RNG Seed registers are retained.
	Idle Power Supply Current: 800uA.
	Note, it will automatically go into sleep mode after watchdog timer has been reached (1.3-1.7sec).
*/

bool ATECCX08A::idleMode()
{
  _i2cPort->beginTransmission(_i2caddr); // set up to write to address
  _i2cPort->write(WORD_ADDRESS_VALUE_IDLE); // enter idle command (aka word address - the first part of every communication to the IC)
  return (_i2cPort->endTransmission() == 0); // actually send it
}

/** \brief

	sleep()

	The ATECCX08A is forcefully put into sleep (LOW POWER) mode and ignores all subsequent I/O transitions
	until the next wake flag. The contents of TempKey and RNG Seed registers are NOT retained.
	Idle Power Supply Current: 150nA.
	This helps avoid waiting for watchdog timer and immidiately puts the device in sleep mode.
	With this sleep/wakeup cycle, RNG seed registers are updated from internal entropy.
*/

bool ATECCX08A::sleep()
{
  idleMode();
  _i2cPort->beginTransmission(_i2caddr); // set up to write to address
  _i2cPort->write(WORD_ADDRESS_VALUE_SLEEP); // enter sleep command (aka word address - the first part of every communication to the IC)
  return (_i2cPort->endTransmission() == 0); // actually send it
}

/** \brief

	getInfo()

	This function sends the INFO Command and listens for the correct version (0x50) within the response.
	The Info command has a mode parameter, and in this function we are using the "Revision" mode (0x00)
	At the time of data sheet creation the Info command will return 0x00 0x00 0x50 0x00. For
	all versions of the ECC508A the 3rd byte will always be 0x50. The fourth byte will indicate the
	silicon revision.
*/

bool ATECCX08A::getInfo()
{
  if (!sendCommand(COMMAND_OPCODE_INFO, 0x00, 0x0000)) // param1 - 0x00 (revision mode).
  {
    return false;
  }

  delay(1); // time for IC to process command and exectute

    // Now let's read back from the IC and see if it reports back good things.
  countGlobal = 0;
  if (!receiveResponseData(RESPONSE_COUNT_SIZE + RESPONSE_INFO_SIZE + CRC_SIZE, true))
    return false;

  idleMode();

  if (!checkCount()|| !checkCrc())
    return false;

  // If we hear a "0x50", that means it had a successful version response.
  if (inputBuffer[RESPONSE_GETINFO_SIGNAL_INDEX] != ATRCC508A_SUCCESSFUL_GETINFO)
    return false;

  return true;
}

/** \brief

	lockConfig()

	This function sends the LOCK Command with the configuration zone parameter,
	and listens for success response (0x00).
*/

bool ATECCX08A::lockConfig()
{
  return lock(LOCK_MODE_ZONE_CONFIG);
}

/** \brief

	readConfigZone()

	This function reads the entire configuration zone EEPROM memory on the device.
	It stores them for vewieing in a large array called configZone[128].
	In addition to configuration settings, the configuration memory on the IC also
	contains the serial number, revision number, lock statuses, and much more.
	This function also updates global variables for these other things.
*/

bool ATECCX08A::readConfigZone(bool debug)
{
  // read block 0, the first 32 bytes of config zone into inputBuffer
  read(ZONE_CONFIG, ADDRESS_CONFIG_READ_BLOCK_0, CONFIG_ZONE_READ_SIZE);

  // copy current contents of inputBuffer into configZone[] (for later viewing/comparing)
  memcpy(&configZone[CONFIG_ZONE_READ_SIZE * 0], &inputBuffer[1], CONFIG_ZONE_READ_SIZE);

  read(ZONE_CONFIG, ADDRESS_CONFIG_READ_BLOCK_1, CONFIG_ZONE_READ_SIZE); 	// read block 1
  memcpy(&configZone[CONFIG_ZONE_READ_SIZE * 1], &inputBuffer[1], CONFIG_ZONE_READ_SIZE); 	// copy block 1

  read(ZONE_CONFIG, ADDRESS_CONFIG_READ_BLOCK_2, CONFIG_ZONE_READ_SIZE); 	// read block 2
  memcpy(&configZone[CONFIG_ZONE_READ_SIZE * 2], &inputBuffer[1], CONFIG_ZONE_READ_SIZE); 	// copy block 2

  read(ZONE_CONFIG, ADDRESS_CONFIG_READ_BLOCK_3, CONFIG_ZONE_READ_SIZE); 	// read block 3
  memcpy(&configZone[CONFIG_ZONE_READ_SIZE * 3], &inputBuffer[1], CONFIG_ZONE_READ_SIZE); 	// copy block 3

  // pull out serial number from configZone, and copy to public variable within this instance
  memcpy(&serialNumber[0], &configZone[CONFIG_ZONE_SERIAL_PART0], 4); 	// copy SN<0:3>
  memcpy(&serialNumber[4], &configZone[CONFIG_ZONE_SERIAL_PART1], 5); 	// copy SN<4:8>

  // pull out revision number from configZone, and copy to public variable within this instance
  memcpy(&revisionNumber[0], &configZone[CONFIG_ZONE_REVISION_NUMBER], 4); 	// copy RevNum<0:3>

  // set lock statuses for config, data/otp, and slot 0
  if (configZone[CONFIG_ZONE_LOCK_STATUS] == 0x00) configLockStatus = true;
  else configLockStatus = false;

  if (configZone[CONFIG_ZONE_OTP_LOCK] == 0x00) dataOTPLockStatus = true;
  else dataOTPLockStatus = false;

  if ( (configZone[CONFIG_ZONE_SLOTS_LOCK0] & (1 << 0) ) == true) slot0LockStatus = false; // LSB is slot 0. if bit set = UN-locked.
  else slot0LockStatus = true;

  memcpy(SlotConfig, &configZone[CONFIG_ZONE_SLOT_CONFIG], sizeof(uint16_t) * DATA_ZONE_SLOTS);
  memcpy(KeyConfig, &configZone[CONFIG_ZONE_KEY_CONFIG], sizeof(uint16_t) * DATA_ZONE_SLOTS);

  if (debug)
  {
    _debugSerial->println("configZone: ");
    for (int i = 0; i < sizeof(configZone) ; i++)
    {
      _debugSerial->print(i);
	  _debugSerial->print(": 0x");
	  if ((configZone[i] >> 4) == 0) _debugSerial->print("0"); // print preceeding high nibble if it's zero
	  _debugSerial->print(configZone[i], HEX);
	  _debugSerial->print(" \t0b");
	  for(int bit = 7; bit >= 0; bit--) _debugSerial->print(bitRead(configZone[i],bit)); // print binary WITH preceding '0' bits
	  _debugSerial->println();
    }
    _debugSerial->println();
  }

  return true;
}

/** \brief

	lockDataAndOTP()

	This function sends the LOCK Command with the Data and OTP (one-time-programming) zone parameter,
	and listens for success response (0x00).
*/

bool ATECCX08A::lockDataAndOTP()
{
  return lock(LOCK_MODE_ZONE_DATA_AND_OTP);
}

/** \brief

	lockDataSlot0()

	This function sends the LOCK Command with the Slot 0 zone parameter,
	and listens for success response (0x00).
*/

bool ATECCX08A::lockDataSlot0()
{
  return lock(LOCK_MODE_SLOT0);
}

/** \brief

	lock(byte zone)

	This function sends the LOCK Command using the argument zone as parameter 1,
	and listens for success response (0x00).
*/

bool ATECCX08A::lock(uint8_t zone)
{
  if (!sendCommand(COMMAND_OPCODE_LOCK, zone, 0x0000))
    return false;

  delay(32); // time for IC to process command and exectute

  // Now let's read back from the IC and see if it reports back good things.
  countGlobal = 0;

  if (!receiveResponseData(RESPONSE_COUNT_SIZE + RESPONSE_SIGNAL_SIZE + CRC_SIZE))
    return false;

  idleMode();

  if (!checkCount() || !checkCrc())
    return false;

  // If we hear a "0x00", that means it had a successful lock
  if (inputBuffer[RESPONSE_SIGNAL_INDEX] != ATRCC508A_SUCCESSFUL_LOCK)
    return false;

  return true;
}

/** \brief

	updateRandom32Bytes(bool debug)

    This function pulls a complete random number (all 32 bytes)
    It stores it in a global array called random32Bytes[]
    If you wish to access this global variable and use as a 256 bit random number,
    then you will need to access this array and combine it's elements as you wish.
    In order to keep compatibility with ATmega328 based arduinos,
    We have offered some other functions that return variables more usable (i.e. byte, int, long)
    They are getRandomByte(), getRandomInt(), and getRandomLong().
*/

bool ATECCX08A::updateRandom32Bytes(bool debug)
{
  if (!sendCommand(COMMAND_OPCODE_RANDOM, 0x00, 0x0000))
    return false;

  // param1 = 0. - Automatically update EEPROM seed only if necessary prior to random number generation. Recommended for highest security.
  // param2 = 0x0000. - must be 0x0000.

  delay(23); // time for IC to process command and exectute

  // Now let's read back from the IC. This will be 35 bytes of data (count + 32_data_bytes + crc[0] + crc[1])

  if (!receiveResponseData(RESPONSE_COUNT_SIZE + RESPONSE_RANDOM_SIZE + CRC_SIZE, debug))
    return false;

  idleMode();

  if (!checkCount(debug) || !checkCrc(debug))
    return false;

  // update random32Bytes[] array
  // we don't need the count value (which is currently the first byte of the inputBuffer)
  for (int i = 0 ; i < RESPONSE_RANDOM_SIZE ; i++) // for loop through to grab all but the first position (which is "count" of the message)
  {
    random32Bytes[i] = inputBuffer[RESPONSE_COUNT_SIZE + i];
  }

  if (debug)
  {
    _debugSerial->print("random32Bytes: ");
    for (int i = 0; i < sizeof(random32Bytes) ; i++)
    {
      _debugSerial->print(random32Bytes[i], HEX);
      _debugSerial->print(",");
    }
    _debugSerial->println();
  }

  return true;
}

/** \brief

	getRandomByte(bool debug)

    This function returns a random byte.
	It calls updateRandom32Bytes(), then uses the first byte in that array for a return value.
*/

byte ATECCX08A::getRandomByte(bool debug)
{
  updateRandom32Bytes(debug);
  return random32Bytes[0];
}

/** \brief

	getRandomInt(bool debug)

    This function returns a random Int.
	It calls updateRandom32Bytes(), then uses the first 2 bytes in that array for a return value.
	It bitwize ORS the first two bytes of the array into the return value.
*/

int ATECCX08A::getRandomInt(bool debug)
{
  updateRandom32Bytes(debug);
  int return_val;
  return_val = random32Bytes[0]; // store first randome byte into return_val
  return_val <<= 8; // shift it over, to make room for the next byte
  return_val |= random32Bytes[1]; // "or in" the next byte in the array
  return return_val;
}

/** \brief

	getRandomLong(bool debug)

    This function returns a random Long.
	It calls updateRandom32Bytes(), then uses the first 4 bytes in that array for a return value.
	It bitwize ORS the first 4 bytes of the array into the return value.
*/

long ATECCX08A::getRandomLong(bool debug)
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

	random(long max)

    This function returns a positive random Long between 0 and max
	max can be up to the larges positive value of a long: 2147483647
*/

long ATECCX08A::random(long max)
{
  return random(0, max);
}

/** \brief

	random(long min, long max)

    This function returns a random Long with set boundaries of min and max.
	If you flip min and max, it still works!
	Also, it can handle negative numbers. Wahoo!
*/

long ATECCX08A::random(long min, long max)
{
  long randomLong = getRandomLong();
  long halfFSR = (max - min) / 2; // half of desired full scale range
  long midPoint = (max + min) / 2; // where we "start" out output value, then add in a fraction of halfFSR
  float fraction = float(randomLong) / 2147483647;
  return (midPoint + (halfFSR * fraction) );
}

/** \brief

	receiveResponseData(uint8_t length, bool debug)

	This function receives messages from the ATECCX08a IC (up to 128 Bytes)
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
bool ATECCX08A::receiveResponseData(uint8_t length, bool debug)
{

  // pull in data 32 bytes at at time. (necessary to avoid overflow on atmega328)
  // if length is less than or equal to 32, then just pull it in.
  // if length is greater than 32, then we must first pull in 32, then pull in remainder.
  // lets use length as our tracker and we will subtract from it as we pull in data.
  countGlobal = 0; // reset for each new message (most important, like wensleydale at a cheese party)
  cleanInputBuffer();
  byte requestAttempts = 0; // keep track of how many times we've attempted to request, to break out if necessary

  /* Normalize length according to buffer size */
  if (length > sizeof(inputBuffer))
    length = sizeof(inputBuffer);

  while(length)
  {
    byte requestAmount; // amount of bytes to request, needed to pull in data 32 bytes at a time
    if (length > ATRCC508A_MAX_REQUEST_SIZE)
    {
      requestAmount = ATRCC508A_MAX_REQUEST_SIZE; // as we have more than 32 to pull in, keep pulling in 32 byte chunks
    }
    else
    {
      requestAmount = length; // now we're ready to pull in the last chunk.
    }

    _i2cPort->requestFrom(_i2caddr, requestAmount);    // request bytes from peripheral

    requestAttempts++;

    while (_i2cPort->available())   // peripheral may send less than requested
    {
      uint8_t value = _i2cPort->read();

      /* Make sure not to read beyond buffer size */
      if (countGlobal < sizeof(inputBuffer))
        inputBuffer[countGlobal] = value;    // receive a byte as character

      length--; // keep this while loop active until we've pulled in everything
      countGlobal++; // keep track of the count of the entire message.
    }

    if (requestAttempts == ATRCC508A_MAX_RETRIES)
      break; // this probably means that the device is not responding.
  }

  if (debug)
  {
    _debugSerial->print("inputBuffer: ");
    for (int i = 0; i < countGlobal ; i++)
    {
      _debugSerial->print(inputBuffer[i], HEX);
      _debugSerial->print(",");
    }
    _debugSerial->println();
  }

  return true;
}

/** \brief

	checkCount(bool debug)

	This function checks that the count byte received in the most recent message equals countGlobal
	Call receiveResponseData, and then imeeditately call this to check the count of the complete message.
	Returns true if inputBuffer[0] == countGlobal.
*/

bool ATECCX08A::checkCount(bool debug)
{
  if (debug)
  {
    _debugSerial->print("countGlobal: 0x");
	_debugSerial->println(countGlobal, HEX);
	_debugSerial->print("count heard from IC (inpuBuffer[0]): 0x");
    _debugSerial->println(inputBuffer[0], HEX);
  }

  // Check count; the first byte sent from IC is count, and it should be equal to the actual message count
  if (inputBuffer[RESPONSE_COUNT_INDEX] != countGlobal)
  {
	if (debug) _debugSerial->println("Message Count Error");
	  return false;
  }

  return true;
}

/** \brief

	checkCrc(bool debug)

	This function checks that the CRC bytes received in the most recent message equals a calculated CRCs
	Call receiveResponseData, then call immediately call this to check the CRCs of the complete message.
*/

bool ATECCX08A::checkCrc(bool debug)
{
  // Check CRC[0] and CRC[1] are good to go.
  atca_calculate_crc(countGlobal - CRC_SIZE, inputBuffer);   // first calculate it

  if (debug)
  {
    _debugSerial->print("CRC[0] Calc: 0x");
	_debugSerial->println(crc[0], HEX);
	_debugSerial->print("CRC[1] Calc: 0x");
    _debugSerial->println(crc[1], HEX);
  }

  if ( (inputBuffer[countGlobal - (CRC_SIZE - 1)] != crc[1]) || (inputBuffer[countGlobal - CRC_SIZE] != crc[0]) )   // then check the CRCs.
  {
	if (debug) _debugSerial->println("Message CRC Error");
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

    This function sets the entire inputBuffer to 0xFFs.
	It is helpful for debugging message/count/CRCs errors.
*/

void ATECCX08A::cleanInputBuffer()
{
  for (int i = 0; i < sizeof(inputBuffer) ; i++)
  {
    inputBuffer[i] = 0xFF;
  }
}

/** \brief

	createNewKeyPair(uint16_t slot)

    This function sends the command to create a new key pair (private AND public)
	in the slot designated by argument slot (default slot 0).
	Sparkfun Default Configuration Sketch calls this, and then locks the data/otp zones and slot 0.
*/

bool ATECCX08A::createNewKeyPair(uint16_t slot)
{
  if (!sendCommand(COMMAND_OPCODE_GENKEY, GENKEY_MODE_NEW_PRIVATE, slot))
    return false;

  delay(115); // time for IC to process command and exectute

  // Now let's read back from the IC.

  if (!receiveResponseData(RESPONSE_COUNT_SIZE + PUBLIC_KEY_SIZE + CRC_SIZE)) // public key (64), plus crc (2), plus count (1)
    return false;

  idleMode();

  // update publicKey64Bytes[] array
  if (!checkCount() || !checkCrc()) // check that it was a good message
    return false;

  // we don't need the count value (which is currently the first byte of the inputBuffer)
  for (int i = 0 ; i < PUBLIC_KEY_SIZE ; i++) // for loop through to grab all but the first position (which is "count" of the message)
  {
    publicKey64Bytes[i] = inputBuffer[i + RESPONSE_COUNT_SIZE];
  }

  return true;
}

/** \brief

	generatePublicKey(uint16_t slot, bool debug)

    This function uses the GENKEY command in "Public Key Computation" mode.

    Generates an ECC public key based upon the private key stored in the slot defined by the KeyID
	parameter (aka slot). Defaults to slot 0.

	Note, if you haven't created a private key in the slot already, then this will fail.

	The generated public key is read back from the device, and then copied from inputBuffer to
	a global variable named publicKey64Bytes for later use.
*/

bool ATECCX08A::generatePublicKey(uint16_t slot, bool debug)
{
  if (!sendCommand(COMMAND_OPCODE_GENKEY, GENKEY_MODE_PUBLIC, slot))
    return false;

  delay(115); // time for IC to process command and exectute

  // Now let's read back from the IC.
  // public key (64), plus crc (2), plus count (1)
  if (!receiveResponseData(RESPONSE_COUNT_SIZE + PUBLIC_KEY_SIZE + CRC_SIZE))
    return false;

  idleMode();

  // update publicKey64Bytes[] array
  if (!checkCount() || !checkCrc()) // check that it was a good message
    return false;

  // we don't need the count value (which is currently the first byte of the inputBuffer)
  for (int i = 0 ; i < PUBLIC_KEY_SIZE ; i++) // for loop through to grab all but the first position (which is "count" of the message)
  {
    publicKey64Bytes[i] = inputBuffer[RESPONSE_COUNT_SIZE + i];
  }

  if (debug)
  {
    _debugSerial->println("This device's Public Key:");
    _debugSerial->println();
    _debugSerial->println("uint8_t publicKey[64] = {");
    for (int i = 0; i < sizeof(publicKey64Bytes) ; i++)
    {
      _debugSerial->print("0x");
      if ((publicKey64Bytes[i] >> 4) == 0) _debugSerial->print("0"); // print preceeding high nibble if it's zero
      _debugSerial->print(publicKey64Bytes[i], HEX);
      if (i != 63) _debugSerial->print(", ");
      if ((63-i) % 16 == 0) _debugSerial->println();
    }

    _debugSerial->println("};");
    _debugSerial->println();
  }

  return true;
}

/** \brief

	read(uint8_t zone, uint16_t address, uint8_t length, bool debug)

    Reads data from the IC at a specific zone and address.
	Your data response will be available at inputBuffer[].

	For more info on address encoding, see datasheet pg 58.
*/

bool ATECCX08A::read(uint8_t zone, uint16_t address, uint8_t length, bool debug)
{
	return read_output(zone, address, length, NULL, debug);
}

bool ATECCX08A::read_output(uint8_t zone, uint16_t address, uint8_t length, uint8_t * output, bool debug)
{
  // adjust zone as needed for whether it's 4 or 32 bytes length read
  // bit 7 of zone needs to be set correctly
  // (0 = 4 Bytes are read)
  // (1 = 32 Bytes are read)
  if (length == 32)
  {
	zone |= 0b10000000; // set bit 7
  }
  else if (length == 4)
  {
	zone &= ~0b10000000; // clear bit 7
  }
  else
  {
	return false; // invalid length, abort.
  }

  if (!sendCommand(COMMAND_OPCODE_READ, zone, address))
    return false;

  delay(1); // time for IC to process command and exectute

  // Now let's read back from the IC. ( + CRC_SIZE + count)
  if (!receiveResponseData(RESPONSE_COUNT_SIZE + length + CRC_SIZE, debug))
    return false;

  idleMode();

  if (!checkCount(debug) || !checkCrc(debug))
    return false;

  /* Copy data to output */
  if (output)
  {
	  memcpy(output, inputBuffer + RESPONSE_READ_INDEX, length);
  }

  return true;
}

/** \brief

	write(uint8_t zone, uint16_t address, uint8_t *data, uint8_t length_of_data)

    Writes data to a specific zone and address on the IC.

	For more info on zone and address encoding, see datasheet pg 58.
*/

bool ATECCX08A::write(uint8_t zone, uint16_t address, uint8_t *data, uint8_t length_of_data)
{
  // adjust zone as needed for whether it's 4 or 32 bytes length write
  // bit 7 of param1 needs to be set correctly
  // (0 = 4 Bytes are written)
  // (1 = 32 Bytes are written)
  if (length_of_data == 32)
  {
	zone |= 0b10000000; // set bit 7
  }
  else if (length_of_data == 4)
  {
	zone &= ~0b10000000; // clear bit 7
  }
  else
  {
	return false; // invalid length, abort.
  }

  if (!sendCommand(COMMAND_OPCODE_WRITE, zone, address, data, length_of_data))
    return false;

  delay(26); // time for IC to process command and exectute

  // Now let's read back from the IC and see if it reports back good things.
  countGlobal = 0;
  if (!receiveResponseData(RESPONSE_COUNT_SIZE + RESPONSE_SIGNAL_SIZE + CRC_SIZE))
    return false;

  idleMode();

  if (!checkCount() || !checkCrc())
    return false;

  // If we hear a "0x00", that means it had a successful write
  if (inputBuffer[RESPONSE_SIGNAL_INDEX] != ATRCC508A_SUCCESSFUL_WRITE)
  {
    return false;
  }


  return true;
}

/** \brief

	createSignature(uint8_t *data, uint16_t slot)

    Creates a 64-byte ECC signature on 32 bytes of data.
	Defaults to use private key located in slot 0.
	Your signature will be available at global variable signature[].

	Note, the IC actually needs you to store your data in a temporary memory location
	called TempKey. This function first loads TempKey, and then signs TempKey. Then it
	receives the signature and copies it to signature[].
*/

bool ATECCX08A::createSignature(uint8_t *data, uint16_t slot)
{
  if (!loadTempKey(data) || !signTempKey(slot))
    return false;

  return true;
}

/** \brief

	loadTempKey(uint8_t *data)

	Writes 32 bytes of data to memory location "TempKey" on the IC.
	Note, the data is provided externally by you, the user, and is included in the
	command NONCE.

    We will use the NONCE command in passthrough mode to load tempKey with our data (aka message).
    Note, the datasheet warns that this does not provide protection agains replay attacks,
    but we will protect again this because our server (Bob) is going to send us it's own unique random TOKEN,
    when it requests data, and this will allow us to create a unique data + signature for every communication.
*/

bool ATECCX08A::loadTempKey(uint8_t *data)
{
  if (!sendCommand(COMMAND_OPCODE_NONCE, NONCE_MODE_PASSTHROUGH, 0x0000, data, 32))
    return false;

  // note, param2 is 0x0000 (and param1 is PASSTHROUGH), so OutData will be just a single byte of zero upon completion.
  // see ds pg 77 for more info

  delay(7); // time for IC to process command and exectute

  // Now let's read back from the IC.
  if (!receiveResponseData(RESPONSE_COUNT_SIZE + RESPONSE_SIGNAL_SIZE + CRC_SIZE))
    return false; // responds with "0x00" if NONCE executed properly

  idleMode();

  if (!checkCount() || !checkCrc())
    return false;

  // If we hear a "0x00", that means it had a successful nonce
  if (inputBuffer[RESPONSE_SIGNAL_INDEX] != ATRCC508A_SUCCESSFUL_TEMPKEY)
    return false;

  return true;
}

/** \brief

	signTempKey(uint16_t slot)

	Create a 64 byte ECC signature for the contents of TempKey using the private key in Slot.
	Default slot is 0

	The response from this command (the signature) is stored in global varaible signature[].
*/

bool ATECCX08A::signTempKey(uint16_t slot)
{
  if (!sendCommand(COMMAND_OPCODE_SIGN, SIGN_MODE_TEMPKEY, slot))
    return false;

  delay(70); // time for IC to process command and exectute

  // Now let's read back from the IC.
  if (!receiveResponseData(RESPONSE_COUNT_SIZE + SIGNATURE_SIZE + CRC_SIZE)) // signature (64), plus crc (2), plus count (1)
    return false;

  idleMode();

  // update signature[] array and print it to serial terminal nicely formatted for easy copy/pasting between sketches
  if (!checkCount() || !checkCrc())  // check that it was a good message
    return false;

   // we don't need the count value (which is currently the first byte of the inputBuffer)
  for (int i = 0 ; i < SIGNATURE_SIZE ; i++) // for loop through to grab all but the first position (which is "count" of the message)
  {
    signature[i] = inputBuffer[RESPONSE_COUNT_SIZE + i];
  }

  _debugSerial->println();
  _debugSerial->println("uint8_t signature[64] = {");
  for (int i = 0; i < sizeof(signature) ; i++)
  {
    _debugSerial->print("0x");
    if ((signature[i] >> 4) == 0) _debugSerial->print("0"); // print preceeding high nibble if it's zero
    _debugSerial->print(signature[i], HEX);
    if (i != 63) _debugSerial->print(", ");
    if ((63-i) % 16 == 0) _debugSerial->println();
  }
  _debugSerial->println("};");

	return true;
}

/** \brief

	verifySignature(uint8_t *message, uint8_t *signature, uint8_t *publicKey)

	Verifies a ECC signature using the message, signature and external public key.
	Returns true if successful.

	Note, it acutally uses loadTempKey, then uses the verify command in "external public key" mode.
*/

bool ATECCX08A::verifySignature(uint8_t *message, uint8_t *signature, uint8_t *publicKey)
{
  uint8_t data_sigAndPub[128];

  // first, let's load the message into TempKey on the device, this uses NONCE command in passthrough mode.
  if (!loadTempKey(message))
  {
    _debugSerial->println("Load TempKey Failure");
    return false;
  }

  // We can only send one *single* data array to sendCommand as Param2, so we need to combine signature and public key.
  memcpy(&data_sigAndPub[0], &signature[0], SIGNATURE_SIZE);	// append signature
  memcpy(&data_sigAndPub[SIGNATURE_SIZE], &publicKey[0], PUBLIC_KEY_SIZE);	// append external public key

  if (!sendCommand(COMMAND_OPCODE_VERIFY, VERIFY_MODE_EXTERNAL, VERIFY_PARAM2_KEYTYPE_ECC, data_sigAndPub, sizeof(data_sigAndPub)))
    return false;

  delay(58); // time for IC to process command and exectute

  // Now let's read back from the IC.
  if (!receiveResponseData(RESPONSE_COUNT_SIZE + RESPONSE_SIGNAL_SIZE + CRC_SIZE))
    return false;

  idleMode();

  if (!checkCount() || !checkCrc())
    return false;

  // If we hear a "0x00", that means it had a successful verify
  if (inputBuffer[RESPONSE_SIGNAL_INDEX] != ATRCC508A_SUCCESSFUL_VERIFY)
    return false;

  return true;
}

bool ATECCX08A::sha256(uint8_t * plain, size_t len, uint8_t * hash)
{
	int i;
	size_t chunks = len / SHA_BLOCK_SIZE + !!(len % SHA_BLOCK_SIZE);
  if((len % SHA_BLOCK_SIZE) == 0) chunks += 1; // END command can only accept up to 63 bytes, so we must add a "blank chunk" for the end command

  // Serial.print("chunks:");
  // Serial.println(chunks);

	if (!sendCommand(COMMAND_OPCODE_SHA, SHA_START, 0))
		return false;

	/* Divide into blocks of 64 bytes per chunk */
	for (i = 0; i < chunks; ++i)
	{
		size_t data_size = SHA_BLOCK_SIZE;

		delay(9);

		if (!receiveResponseData(RESPONSE_COUNT_SIZE + RESPONSE_SIGNAL_SIZE + CRC_SIZE))
			return false;

		idleMode();

		if (!checkCount() || !checkCrc())
			return false;

		// If we hear a "0x00", that means it had a successful load
		if (inputBuffer[RESPONSE_SIGNAL_INDEX] != ATRCC508A_SUCCESSFUL_SHA)
			return false;

		if (i + 1 == chunks) // if we're on the last chunk, there will be a remainder or 0 (and 0 is okay for an end command)
			data_size = len % SHA_BLOCK_SIZE;

    // Serial.print("chunk:");
    // Serial.println(i);
    // Serial.print("data_size:");
    // Serial.println(data_size);
    // Serial.print("update vs end:");
    // Serial.println((i + 1 != chunks) ? SHA_UPDATE : SHA_END);

		/* Send next */
		if (!sendCommand(COMMAND_OPCODE_SHA, (i + 1 != chunks) ? SHA_UPDATE : SHA_END, data_size, plain + i * SHA_BLOCK_SIZE, data_size))
			return false;
	}

	/* Read digest */
	delay(9);

	if (!receiveResponseData(RESPONSE_COUNT_SIZE + RESPONSE_SHA_SIZE + CRC_SIZE))
		return false;

	idleMode();

	if (!checkCount() || !checkCrc())
		return false;

	/* Copy digest */
	for (i = 0; i < SHA256_SIZE; ++i)
	{
		hash[i] = inputBuffer[RESPONSE_SHA_INDEX + i];
	}

	return true;
}
/** \brief

	writeConfigSparkFun()

	Writes the necessary configuration settings to the IC in order to work with the SparkFun Arduino Library examples.
	For key slots 0 and 1, this enables ECC private key pairs,public key generation, and external signature verifications.

	Returns true if write commands were successful.
*/

bool ATECCX08A::writeConfigSparkFun()
{
  // keep track of our write command results.
  bool result1;
  bool result2;

  // set keytype on slot 0 and 1 to 0x3300
  // Lockable, ECC, PuInfo set (public key always allowed to be generated), contains a private Key
  uint8_t data1[] = {0x33, 0x00, 0x33, 0x00}; // 0x3300 sets the keyconfig.keyType, see datasheet pg 20
  result1 = write(ZONE_CONFIG, (96 / 4), data1, 4);
  // set slot config on slot 0 and 1 to 0x8320
  // EXT signatures, INT signatures, IsSecret, Write config never
  uint8_t data2[] = {0x83, 0x20, 0x83, 0x20}; // for slot config bit definitions see datasheet pg 20
  result2 = write(ZONE_CONFIG, (20 / 4), data2, 4);

  return (result1 && result2);
}

/** \brief

	sendCommand(uint8_t command_opcode, uint8_t param1, uint16_t param2, uint8_t *data, size_t length_of_data)

	Generic function for sending commands to the IC.

	This function handles creating the "total transmission" to the IC.
	This contains WORD_ADDRESS_VALUE, COUNT, OPCODE, PARAM1, PARAM2, DATA (optional), and CRCs.

	Note, it always calls the "wake()" function, assuming that you have let the IC fall asleep (default 1.7 sec)

	Note, for anything other than a command (reset, sleep and idle), you need a different "Word Address Value",
	So those specific transmissions are handled in unique functions.
*/

bool ATECCX08A::sendCommand(uint8_t command_opcode, uint8_t param1, uint16_t param2, uint8_t *data, size_t length_of_data)
{
  // build packet array (total_transmission) to send a communication to IC, with opcode COMMAND
  // It expects to see: word address, count, command opcode, param1, param2, data (optional), CRC[0], CRC[1]
  uint8_t transmission_without_crc_length;
  uint8_t total_transmission_length;
  uint8_t total_transmission[UINT8_MAX];
  uint8_t packet_to_CRC[UINT8_MAX]; // minus word address (1) and crc (2).

  /* Validate no integer overflow */
  if (length_of_data > UINT8_MAX - ATRCC508A_PROTOCOL_OVERHEAD)
    return false;

  total_transmission_length = length_of_data + ATRCC508A_PROTOCOL_OVERHEAD;

  total_transmission[ATRCC508A_PROTOCOL_FIELD_COMMAND] = WORD_ADDRESS_VALUE_COMMAND;      // word address value (type command)
  total_transmission[ATRCC508A_PROTOCOL_FIELD_LENGTH] = total_transmission_length - ATRCC508A_PROTOCOL_FIELD_SIZE_LENGTH;    // count, does not include itself, so "-1"
  total_transmission[ATRCC508A_PROTOCOL_FIELD_OPCODE] = command_opcode;                   // command
  total_transmission[ATRCC508A_PROTOCOL_FIELD_PARAM1] = param1;                           // param1
  memcpy(&total_transmission[ATRCC508A_PROTOCOL_FIELD_PARAM2], &param2, sizeof(param2));  // append param2
  memcpy(&total_transmission[ATRCC508A_PROTOCOL_FIELD_DATA], &data[0], length_of_data);   // append data

  // update CRCs
  transmission_without_crc_length = total_transmission_length - (ATRCC508A_PROTOCOL_FIELD_SIZE_COMMAND + ATRCC508A_PROTOCOL_FIELD_SIZE_CRC); // copy over just what we need to CRC starting at index 1

  memcpy(packet_to_CRC, &total_transmission[ATRCC508A_PROTOCOL_FIELD_LENGTH], transmission_without_crc_length);

  //  _debugSerial->println("packet_to_CRC: ");
  //  for (int i = 0; i < sizeof(packet_to_CRC) ; i++)
  //  {
  //  _debugSerial->print(packet_to_CRC[i], HEX);
  //  _debugSerial->print(",");
  //  }
  //  _debugSerial->println();

  atca_calculate_crc(transmission_without_crc_length, packet_to_CRC); // count includes crc[0] and crc[1], so we must subtract 2 before creating crc
  //_debugSerial->println(crc[0], HEX);
  //_debugSerial->println(crc[1], HEX);

  memcpy(&total_transmission[total_transmission_length - ATRCC508A_PROTOCOL_FIELD_SIZE_CRC], crc, ATRCC508A_PROTOCOL_FIELD_SIZE_CRC);  // append crcs

  wakeUp();

  _i2cPort->beginTransmission(_i2caddr);
  _i2cPort->write(total_transmission, total_transmission_length);
  return (_i2cPort->endTransmission() == 0);
}
