/*
  This is a library written for the ATECCX08A Criptographic Co-Processor (QWIIC).

  Written by Pete Lewis @ SparkFun Electronics, August 5th, 2019

  Contributions for SHA256 support by Gili Yankovitch (github @gili-yankovitch), March 2020
  See details of PR here: https://github.com/sparkfun/SparkFun_ATECCX08a_Arduino_Library/pull/3

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

#pragma once

#if (ARDUINO >= 100)
#include "Arduino.h"
#else
#include "WProgram.h"
#endif

#include "Wire.h"

/* Protocol + Cryptographic defines */
#define RESPONSE_COUNT_SIZE  1
#define RESPONSE_SIGNAL_SIZE 1
#define RESPONSE_SHA_SIZE    32
#define RESPONSE_INFO_SIZE   4
#define RESPONSE_RANDOM_SIZE 32
#define CRC_SIZE             2
#define CONFIG_ZONE_SIZE     128
#define SERIAL_NUMBER_SIZE   10

#define RANDOM_BYTES_BLOCK_SIZE 32
#define SHA256_SIZE          32
#define PUBLIC_KEY_SIZE      64
#define SIGNATURE_SIZE       64
#define BUFFER_SIZE          128

#define DATA_ZONE_SLOTS	     16

#define WRITE_CONFIG(SCONFIG)	(SCONFIG & 0b1111000000000000)
#define WRITE_KEY(SCONFIG)		(SCONFIG & 0b0000111100000000)
#define IS_SECRET(SCONFIG)		(SCONFIG & 0b0000000010000000)
#define ENCRYPT_READ(SCONFIG)	(SCONFIG & 0b0000000001000000)
#define LIMITED_USE(SCONFIG)	(SCONFIG & 0b0000000000100000)
#define NO_MAC(SCONFIG)			(SCONFIG & 0b0000000000010000)
#define READ_KEY(SCONFIG)		(SCONFIG & 0b0000000000001111)

/* Response signals always come after the first count byte */
#define RESPONSE_COUNT_INDEX 0
#define RESPONSE_SIGNAL_INDEX RESPONSE_COUNT_SIZE
#define RESPONSE_SHA_INDEX RESPONSE_COUNT_SIZE
#define RESPONSE_READ_INDEX RESPONSE_COUNT_SIZE
#define RESPONSE_GETINFO_SIGNAL_INDEX (RESPONSE_COUNT_SIZE + 2)

/* Protocol Indices */
#define ATRCC508A_PROTOCOL_FIELD_COMMAND 0
#define ATRCC508A_PROTOCOL_FIELD_LENGTH  1
#define ATRCC508A_PROTOCOL_FIELD_OPCODE  2
#define ATRCC508A_PROTOCOL_FIELD_PARAM1  3
#define ATRCC508A_PROTOCOL_FIELD_PARAM2  4
#define ATRCC508A_PROTOCOL_FIELD_DATA    6

/* Protocl Sizes */
#define ATRCC508A_PROTOCOL_FIELD_SIZE_COMMAND 1
#define ATRCC508A_PROTOCOL_FIELD_SIZE_LENGTH  1
#define ATRCC508A_PROTOCOL_FIELD_SIZE_OPCODE  1
#define ATRCC508A_PROTOCOL_FIELD_SIZE_PARAM1  1
#define ATRCC508A_PROTOCOL_FIELD_SIZE_PARAM2  2
#define ATRCC508A_PROTOCOL_FIELD_SIZE_CRC     CRC_SIZE

/* Protocol overhead at sendCommand(): word address val (1) + count (1) + command opcode (1) param1 (1) + param2 (2) data (0-?) + crc (2) */
#define ATRCC508A_PROTOCOL_OVERHEAD (ATRCC508A_PROTOCOL_FIELD_SIZE_COMMAND + ATRCC508A_PROTOCOL_FIELD_SIZE_LENGTH + ATRCC508A_PROTOCOL_FIELD_SIZE_OPCODE + ATRCC508A_PROTOCOL_FIELD_SIZE_PARAM1 + ATRCC508A_PROTOCOL_FIELD_SIZE_PARAM2 + ATRCC508A_PROTOCOL_FIELD_SIZE_CRC)

/* Protocol codes */
#define ATRCC508A_SUCCESSFUL_TEMPKEY 0x00
#define ATRCC508A_SUCCESSFUL_VERIFY  0x00
#define ATRCC508A_SUCCESSFUL_WRITE   0x00
#define ATRCC508A_SUCCESSFUL_SHA     0x00
#define ATRCC508A_SUCCESSFUL_LOCK    0x00
#define ATRCC508A_SUCCESSFUL_WAKEUP  0x11
#define ATRCC508A_SUCCESSFUL_GETINFO 0x50 /* Revision number */

/* Receive constants */
#define ATRCC508A_MAX_REQUEST_SIZE 32
#define ATRCC508A_MAX_RETRIES 20

/* configZone EEPROM mapping */
#define CONFIG_ZONE_READ_SIZE    32
#define CONFIG_ZONE_SERIAL_PART0    0
#define CONFIG_ZONE_SERIAL_PART1    8
#define CONFIG_ZONE_REVISION_NUMBER 4
#define CONFIG_ZONE_SLOT_CONFIG 20
#define CONFIG_ZONE_OTP_LOCK     86
#define CONFIG_ZONE_LOCK_STATUS  87
#define CONFIG_ZONE_SLOTS_LOCK0   88
#define CONFIG_ZONE_SLOTS_LOCK1   89
#define CONFIG_ZONE_KEY_CONFIG	  96


#define ATECC508A_ADDRESS_DEFAULT 0x60 //7-bit unshifted default I2C Address
// 0x60 on a fresh chip. note, this is software definable

// WORD ADDRESS VALUES
// These are sent in any write sequence to the IC.
// They tell the IC what we are going to do: Reset, Sleep, Idle, Command.
#define WORD_ADDRESS_VALUE_COMMAND 	0x03	// This is the "command" word address,
//this tells the IC we are going to send a command, and is used for most communications to the IC
#define WORD_ADDRESS_VALUE_IDLE 0x02 // used to enter idle mode
//this tells the IC to enter sleep mode
#define WORD_ADDRESS_VALUE_SLEEP	0x01 // used to enter sleep mode

// COMMANDS (aka "opcodes" in the datasheet)
#define COMMAND_OPCODE_INFO 	0x30 // Return device state information.
#define COMMAND_OPCODE_LOCK 	0x17 // Lock configuration and/or Data and OTP zones
#define COMMAND_OPCODE_RANDOM 	0x1B // Create and return a random number (32 bytes of data)
#define COMMAND_OPCODE_READ 	0x02 // Return data at a specific zone and address.
#define COMMAND_OPCODE_WRITE 	0x12 // Return data at a specific zone and address.
#define COMMAND_OPCODE_SHA 		0x47 // Computes a SHA-256 or HMAC/SHA digest for general purpose use by the system.
#define COMMAND_OPCODE_GENKEY 	0x40 // Creates a key (public and/or private) and stores it in a memory key slot
#define COMMAND_OPCODE_NONCE 	0x16 //
#define COMMAND_OPCODE_SIGN 	0x41 // Create an ECC signature with contents of TempKey and designated key slot
#define COMMAND_OPCODE_VERIFY 	0x45 // takes an ECDSA <R,S> signature and verifies that it is correctly generated from a given message and public key

// Lock command PARAM1 zone options (aka Mode). more info at table on datasheet page 75
// 		? _ _ _  _ _ _ _ 	Bits 7 verify zone summary, 1 = ignore summary and write to zone!
// 		_ ? _ _  _ _ _ _ 	Bits 6 Unused, must be zero
// 		_ _ ? ?  ? ? _ _ 	Bits 5-2 Slot number (in this example, we use slot 0, so "0 0 0 0")
// 		_ _ _ _  _ _ ? ? 	Bits 1-0 Zone or locktype. 00=Config, 01=Data/OTP, 10=Single Slot in Data, 11=illegal

// SHA Params
#define SHA_START						0b00000000
#define SHA_UPDATE						0b00000001
#define SHA_END							0b00000010
#define SHA_BLOCK_SIZE					64

#define LOCK_MODE_ZONE_CONFIG 			0b10000000
#define LOCK_MODE_ZONE_DATA_AND_OTP 	0b10000001
#define LOCK_MODE_SLOT0					0b10000010

#define KEY_CONFIG_OFFSET_X509ID		14
#define KEY_CONFIG_OFFSET_RFU			13
#define KEY_CONFIG_OFFSET_INTRUSION_DIS	12
#define KEY_CONFIG_OFFSET_AUTH_KEY		8
#define KEY_CONFIG_OFFSET_REQ_AUTH		7
#define KEY_CONFIG_OFFSET_REQ_RANDOM	6
#define KEY_CONFIG_OFFSET_LOCKABLE		5
#define KEY_CONFIG_OFFSET_KEY_TYPE		2
#define KEY_CONFIG_OFFSET_PUB_INFO		1
#define KEY_CONFIG_OFFSET_PRIVATE		0
#define KEY_CONFIG_SET(data, config)	((data) << (config))

// GenKey command PARAM1 zone options (aka Mode). more info at table on datasheet page 71
#define GENKEY_MODE_PUBLIC 			0b00000000
#define GENKEY_MODE_NEW_PRIVATE 	0b00000100

#define NONCE_MODE_PASSTHROUGH		0b00000011 // Operate in pass-through mode and Write TempKey with NumIn. datasheet pg 79
#define SIGN_MODE_TEMPKEY			0b10000000 // The message to be signed is in TempKey. datasheet pg 85
#define VERIFY_MODE_EXTERNAL		0b00000010 // Use an external public key for verification, pass to command as data post param2, ds pg 89
#define VERIFY_MODE_STORED			0b00000000 // Use an internally stored public key for verification, param2 = keyID, ds pg 89
#define VERIFY_PARAM2_KEYTYPE_ECC 	0x0004 // When verify mode external, param2 should be KeyType, ds pg 89
#define VERIFY_PARAM2_KEYTYPE_NONECC 	0x0007 // When verify mode external, param2 should be KeyType, ds pg 89

#define ZONE_CONFIG 0x00
#define ZONE_OTP 0x01
#define ZONE_DATA 0x02

#define SLOT_CONFIG_ADDRESS(SLOT)					(((CONFIG_ZONE_SLOT_CONFIG) + sizeof(uint16_t) * (SLOT)) >> 2)
#define KEY_CONFIG_ADDRESS(SLOT)					(((CONFIG_ZONE_KEY_CONFIG) + sizeof(uint16_t) * (SLOT)) >> 2)
#define EEPROM_CONFIG_ADDRESS(OFFSET)				((OFFSET) >> 2)
#define EEPROM_DATA_ADDRESS(SLOT, BLOCK, OFFSET)	((((BLOCK) & 0b00001111) << 8) | ((((SLOT) & 0b01111) << 3) | ((OFFSET) & 0b00000111)))

#define ADDRESS_CONFIG_READ_BLOCK_0 0x0000 // 00000000 00000000 // param2 (byte 0), address block bits: _ _ _ 0  0 _ _ _
#define ADDRESS_CONFIG_READ_BLOCK_1 0x0008 // 00000000 00001000 // param2 (byte 0), address block bits: _ _ _ 0  1 _ _ _
#define ADDRESS_CONFIG_READ_BLOCK_2 0x0010 // 00000000 00010000 // param2 (byte 0), address block bits: _ _ _ 1  0 _ _ _
#define ADDRESS_CONFIG_READ_BLOCK_3 0x0018 // 00000000 00011000 // param2 (byte 0), address block bits: _ _ _ 1  1 _ _ _

class ATECCX08A {
  public:

    //By default use Wire, standard I2C speed, and the default ADS1015 address
	#if defined(ARDUINO_ARCH_SAMD) // checking which board we are using and selecting a Serial debug that will work.
	bool begin(uint8_t i2caddr = ATECC508A_ADDRESS_DEFAULT, TwoWire &wirePort = Wire, Stream &serialPort = SerialUSB);  // SamD21 boards
	#else
	bool begin(uint8_t i2caddr = ATECC508A_ADDRESS_DEFAULT, TwoWire &wirePort = Wire, Stream &serialPort = Serial); // Artemis
	#endif

	byte inputBuffer[BUFFER_SIZE]; // used to store messages received from the IC as they come in
	byte configZone[CONFIG_ZONE_SIZE]; // used to store configuration zone bytes read from device EEPROM
	uint8_t revisionNumber[5]; // used to store the complete revision number, pulled from configZone[4-7]
	uint8_t serialNumber[SERIAL_NUMBER_SIZE]; // used to store the complete Serial number, pulled from configZone[0-3] and configZone[8-12]
	bool configLockStatus; // pulled from configZone[87], then set according to status (0x55=UNlocked, 0x00=Locked)
	bool dataOTPLockStatus; // pulled from configZone[86], then set according to status (0x55=UNlocked, 0x00=Locked)
	bool slot0LockStatus; // pulled from configZone[88], then set according to slot (bit 0) status
	uint16_t SlotConfig[DATA_ZONE_SLOTS];
	uint16_t KeyConfig[DATA_ZONE_SLOTS];

	byte publicKey64Bytes[PUBLIC_KEY_SIZE]; // used to store the public key returned when you (1) create a keypair, or (2) read a public key
	uint8_t signature[SIGNATURE_SIZE];

	bool receiveResponseData(uint8_t length = 0, bool debug = false);
	bool checkCount(bool debug = false);
	bool checkCrc(bool debug = false);
	uint8_t countGlobal = 0; // used to add up all the bytes on a long message. Important to reset before each new receiveMessageData();
	void cleanInputBuffer();

	bool wakeUp();
	bool idleMode();
	bool getInfo();
	bool writeConfigSparkFun();
	bool lockConfig(); // note, this PERMINANTLY disables changes to config zone - including changing the I2C address!
	bool lockDataAndOTP();
	bool lockDataSlot0();
	bool lock(uint8_t zone);
	bool sleep();

	// Random array and fuctions
	byte random32Bytes[32]; // used to store the complete data return (32 bytes) when we ask for a random number from chip.
	bool updateRandom32Bytes(bool debug = false);
	byte getRandomByte(bool debug = false);
	int getRandomInt(bool debug = false);
	long getRandomLong(bool debug = false);
	long random(long max);
	long random(long min, long max);

	// SHA256
	bool sha256(uint8_t * data, size_t len, uint8_t * hash);

	uint8_t crc[CRC_SIZE] = {0, 0};
	void atca_calculate_crc(uint8_t length, uint8_t *data);

	// Key functions
	bool createNewKeyPair(uint16_t slot = 0x0000);
	bool generatePublicKey(uint16_t slot = 0x0000, bool debug = true);

	bool createSignature(uint8_t *data, uint16_t slot = 0x0000);
	bool loadTempKey(uint8_t *data);  // load 32 bytes of data into tempKey (a temporary memory spot in the IC)
	bool signTempKey(uint16_t slot = 0x0000); // create signature using contents of TempKey and PRIVATE KEY in slot
	bool verifySignature(uint8_t *message, uint8_t *signature, uint8_t *publicKey); // external ECC publicKey only

	bool read(uint8_t zone, uint16_t address, uint8_t length, bool debug = false);
	bool read_output(uint8_t zone, uint16_t address, uint8_t length, uint8_t * output, bool debug = false);
	bool write(uint8_t zone, uint16_t address, uint8_t *data, uint8_t length_of_data);

	bool readConfigZone(bool debug = true);
	bool sendCommand(uint8_t command_opcode, uint8_t param1, uint16_t param2, uint8_t *data = NULL, size_t length_of_data = 0);

  private:

	TwoWire *_i2cPort;

	uint8_t _i2caddr;

	Stream *_debugSerial; //The generic connection to user's chosen serial hardware

};


