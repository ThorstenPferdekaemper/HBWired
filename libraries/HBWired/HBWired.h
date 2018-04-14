/*
 * HBWired.h
 *
 *  Created on: 19.11.2016
 *      Author: Thorsten Pferdekaemper
 */

#ifndef HBWired_h
#define HBWired_h

#include "Arduino.h"

class HBWDevice;

// Basisklasse fuer Channels
class HBWChannel {
  public:	
    virtual void set(HBWDevice*, uint8_t length, uint8_t const * const data);
    virtual uint8_t get(uint8_t* data);  // returns length, data must be big enough 
    virtual void loop(HBWDevice*, uint8_t channel);  // channel for feedbacks etc.
	virtual void afterReadConfig();
};


class HBWLinkSender {
  public:
	  virtual void sendKeyEvent(HBWDevice* device, uint8_t srcChan, 
	               uint8_t keyPressNum, boolean longPress) = 0;
};	  


class HBWLinkReceiver {
  public:
	  virtual void receiveKeyEvent(HBWDevice* device, uint32_t senderAddress, uint8_t senderChannel, 
                                   uint8_t targetChannel, uint8_t keyPressNum, boolean longPress) = 0;
};


// Events to be processed by the device
// - Read Config: (re-)Read Config selbst ist immer gleich, 
//                aber ggf. Device-spezifische "nacharbeiten"
// - processKeyEvent: (i.e. incoming 0x4B or 0xCB) For peerings, always module specific so far
//                    Unclear, whether 0x4B and 0xCB should really be handled differently 
// - processGetLevel: (S) Normalerweise geht das an den Kanal, aber spezielle 
//                    "virtuelle" Kanaele sollten erlaubt sein -> get()
// - processSetLock: (l) Aehnlich wie S, aber weniger wichtig. Moeglicherweise immer ok, 
//                   das an den Kanal zu geben (bzw. es ist eh sinnvoller, das uebers 
//                   EEPROM zu machen) -> gar nicht unterstuetzen
// - processSetLevel: (s,x) Siehe Siehe S -> set()

// Commands triggered through the device
// - Key Event 0x4B (Broadcast/Central Address/Peers)
// - Info-Message 0x69 (Broadcast/Central Address. Peers unclear )

#define MAX_RX_FRAME_LENGTH 64


// The HBWired-Device
class HBWDevice {
  public:
    HBWDevice(uint8_t _devicetype, uint8_t _hardware_version, uint16_t _firmware_version,
	          Stream* _rs485, uint8_t _txen, 
	          uint8_t _configSize, void* _config, 
			  uint8_t _numChannels, HBWChannel** _channels,
			  Stream* _debugstream, HBWLinkSender* = NULL, HBWLinkReceiver* = NULL);
  
    void setConfigPins(uint8_t _configPin = 8, uint8_t _ledPin = 13);
  
    virtual void loop(); // needs to be called as often as possible

  	// get logging time	
    uint8_t getLoggingTime();	

	virtual void set(uint8_t,uint8_t,uint8_t const * const);    // channel, data length, data
	virtual uint8_t get(uint8_t channel, uint8_t* data);  // returns length
	virtual void afterReadConfig();
						   
	// target_address = 0 sends info message to central address					   
	// sendInfoMessage returns...
	//  0 -> ok
	//  1 -> bus not idle
	//  2 -> missing ACK (three times)
	virtual uint8_t sendInfoMessage(uint8_t channel, uint8_t length, uint8_t const * const data, uint32_t target_address = 0); 			   
	// Allgemeiner "Key Event"
    virtual uint8_t sendKeyEvent(uint8_t channel, uint8_t keyPressNum, boolean longPress);
	// Key Event Routine mit Target fuer LinkSender 
    virtual uint8_t sendKeyEvent(uint8_t channel, uint8_t keyPressNum, boolean longPress,
								 uint32_t target_address, uint8_t target_channel);
    // Key-Event senden mit Geraetespezifischen Daten (nur Broadcast)
    virtual uint8_t sendKeyEvent(uint8_t srcChan, uint8_t length, void* data);
 								 
	virtual void receiveKeyEvent(uint32_t senderAddress, uint8_t srcChan, 
                                 uint8_t dstChan, uint8_t keyPressNum, boolean longPress);
    virtual void processEvent(uint8_t const * const frameData, uint8_t frameDataLength,
                              boolean isBroadcast = false);
	// read EEPROM
	void readEEPROM(void* dst, uint16_t address, uint16_t length, 
                           boolean lowByteFirst = false);
	uint32_t getOwnAddress();

  private:							 
	uint8_t numChannels;    // number of channels
	HBWChannel** channels;  // channels
    HBWLinkSender* linkSender;
	HBWLinkReceiver* linkReceiver;
	
	// pins of config button and config LED
	uint8_t configPin;
	uint8_t ledPin;
	
	// sendFrame macht ggf. Wiederholungen
	// onlyIfIdle: If this is set, then the bus must have been idle since 210+rand(0..100) ms
	// sendFrame returns...
	//   0 -> ok
	//   1 -> bus not idle (only if onlyIfIdle)
	//   2 -> three times no ACK (cannot occur for broadcasts or ACKs)
	uint8_t sendFrame(boolean onlyIfIdle = false);
	void sendAck();  // ACK fuer gerade verarbeitete Message senden

	// eigene Adresse setzen und damit auch random seed
	void setOwnAddress(unsigned long address);

	void readConfig();         // read config from EEPROM
    // get central address
    uint32_t getCentralAddress();
    void handleBroadcastAnnounce();
	
    // Senderadresse beim Empfangen
	uint32_t senderAddress;
	// Senden
	uint32_t txTargetAddress;        // Adresse des Moduls, zu dem gesendet wird
	
	uint8_t txFrameControlByte;
    uint8_t txFrameDataLength;              // Laenge der Daten + Checksum
	uint8_t txFrameData[MAX_RX_FRAME_LENGTH];


	// the broadcast methods return...
	// 0 -> everything ok
	// 1 -> nothing sent because bus busy
	uint8_t broadcastAnnounce(uint8_t);  // channel

	uint8_t deviceType;        

	// write to EEPROM, but only if not "value" anyway
	// the uppermost 4 bytes are reserved for the device address and can only be changed if privileged = true
	void writeEEPROM(int16_t address, uint8_t value, bool privileged = false );

	uint8_t configSize;     // size of config object without peerings
	uint8_t* config;        // pointer to config object 

  uint8_t hardware_version;
  uint16_t firmware_version;
  
// Das eigentliche RS485-Interface, kann z.B. HBWSoftwareSerial oder (Hardware)Serial sein
	Stream* serial;
// Pin-Nummer fuer "TX-Enable"
	uint8_t txEnablePin;
	// Empfangs-Status
	uint8_t frameStatus;
// eigene Adresse
	unsigned long ownAddress;
// Empfangene Daten
	// Empfangen
	uint8_t frameComplete;
    uint32_t targetAddress;
	uint8_t frameDataLength;                 // Laenge der Daten
	uint8_t frameData[MAX_RX_FRAME_LENGTH];
	uint8_t frameControlByte;
// carrier sense
//  last time we have received anything
    unsigned long lastReceivedTime;
//  current minimum idle time
//  will be initialized in constructor
    unsigned int minIdleTime;
	void receive();  // wird zyklisch aufgerufen
	boolean parseFrame();
	void sendFrameSingle();
	void sendFrameByte(uint8_t, uint16_t* crc = NULL);
	void crc16Shift(uint8_t, uint16_t*);

	void readAddressFromEEPROM();
	void determineSerial(uint8_t*);

	void processEventKey(uint32_t senderAddress, uint8_t const * const frameData);
    void processEventSetLevel(uint8_t channel, uint8_t dataLength, uint8_t const * const data);  //TODO: rename?
	void processEventGetLevel(uint8_t channel, uint8_t command);
	void processEventSetLock();
	void processEmessage(uint8_t const * const frameData);

    void factoryReset();
	void handleConfigButton();
	boolean afterReadConfigPending;
};


/*
*****************************
** DEBUG
*****************************
*/

extern Stream* hbwdebugstream;

void hbwdebughex(uint8_t b);

template <typename T>
void hbwdebug(T msg) { if(hbwdebugstream) hbwdebugstream->print(msg); };

template <typename T>
void hbwdebug(T msg, int base) { if(hbwdebugstream) hbwdebugstream->print(msg,base); };

#endif /* HBWired_h */
