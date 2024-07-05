/*
 * HBWired.h
 *
 *  Created on: 19.11.2016
 *      Author: Thorsten Pferdekaemper
 */

#ifndef HBWired_h
#define HBWired_h

#include "Arduino.h"
#include "hardware.h"
// #include "avr/wdt.h"

#define HBW_DEBUG  // reduce code size, if no serial output is needed (hbwdebug() will be replaced by an emtpy template!)

/* enable the below to allow peering with HBWLinkInfoEventActuator/HBWLinkInfoEventSensor
 * sendInfoEvent() will send data to the peered channel (locally or remote) calling setInfo() */
// #define Support_HBWLink_InfoEvent

// #define Support_ModuleReset  // enable reset comand, to restart module "!!" (hexstring 2121)
// #define Support_WDT  // enable 1 second watchdog timer


class HBWDevice;

// Basisklasse fuer Channels
class HBWChannel {
  public:
    virtual void set(HBWDevice*, uint8_t length, uint8_t const * const data);
  #ifdef Support_HBWLink_InfoEvent
    virtual void setInfo(HBWDevice*, uint8_t length, uint8_t const * const data);  // called by receiveInfoEvent
  #endif
    virtual uint8_t get(uint8_t* data);  // returns length, data must be large enough 
    virtual void loop(HBWDevice*, uint8_t channel);  // channel for feedbacks etc.
    virtual void afterReadConfig();

    void setLock(boolean inhibit);
    boolean getLock();

  private:
    boolean inhibitActive = false;  // disables all peerings, if true (only applicable for actor channels)

  protected:
    uint32_t lastFeedbackTime;  // when did we send the last feedback?
    uint16_t nextFeedbackDelay; // 0 -> no feedback pending
    void setFeedback(HBWDevice*, boolean loggingEnabled, uint16_t loggingTime = 0);  //  set feedback trigger
    void checkFeedback(HBWDevice*, uint8_t channel);  //  send sendInfoMessage, if feedback trigger is set
    void clearFeedback();  //  reset/init feedback trigger values
};


class HBWLinkSender {
  public:
    virtual void sendKeyEvent(HBWDevice* device, uint8_t srcChan, uint8_t keyPressNum, boolean longPress);
  #ifdef Support_HBWLink_InfoEvent
	virtual void sendInfoEvent(HBWDevice* device, uint8_t srcChan, uint8_t length, uint8_t const * const data);
  #endif
};


class HBWLinkReceiver {
  public:
    virtual void receiveKeyEvent(HBWDevice* device, uint32_t senderAddress, uint8_t senderChannel, 
                                 uint8_t targetChannel, uint8_t keyPressNum, boolean longPress);
  #ifdef Support_HBWLink_InfoEvent
    virtual void receiveInfoEvent(HBWDevice* device, uint32_t senderAddress, uint8_t senderChannel,
                                uint8_t targetChannel, uint8_t length, uint8_t const * const data);
  #endif
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
static const boolean NEED_IDLE_BUS = true;  // use for sendFrame

#define DEFAULT_SEND_RETRIES 3
#define PEER_SEND_RETRIES 1  // Send peer message only once


// The HBWired-Device
class HBWDevice {
  public:
    HBWDevice(uint8_t _devicetype, uint8_t _hardware_version, uint16_t _firmware_version,
	          Stream* _rs485, uint8_t _txen, 
	          uint8_t _configSize, void* _config, 
			  uint8_t _numChannels, HBWChannel** _channels,
			  Stream* _debugstream, HBWLinkSender* = NULL, HBWLinkReceiver* = NULL);
  
    void setConfigPins(uint8_t _configPin = 8, uint8_t _ledPin = 13);
    void setStatusLEDPins(uint8_t _txLedPin, uint8_t _rxLedPin);
  
    virtual void loop(); // needs to be called as often as possible

  	// get logging time	
    uint8_t getLoggingTime();
	
  #ifdef Support_HBWLink_InfoEvent
	virtual void setInfo(uint8_t, uint8_t, uint8_t const * const);    // channel, data length, data
  #endif
	virtual void set(uint8_t,uint8_t,uint8_t const * const);    // channel, data length, data
	virtual uint8_t get(uint8_t channel, uint8_t* data);  // returns length
	virtual void afterReadConfig();
						   
	// target_address = 0 sends info message to central address					   
	// sendInfoMessage returns...
	enum sendFrameStatus
	{
		SUCCESS = 0,	//   0 -> ok
		BUS_BUSY,	//   1 -> bus not idle (only if onlyIfIdle)
		NO_ACK		//   2 -> three times no ACK (cannot occur for broadcasts or ACKs)
	};

	virtual uint8_t sendInfoMessage(uint8_t channel, uint8_t length, uint8_t const * const data, uint32_t target_address = 0);
  #ifdef Support_HBWLink_InfoEvent
	// link/peer via i-message
	virtual uint8_t sendInfoEvent(uint8_t srcChan, uint8_t length, uint8_t const * const data, boolean busState = NEED_IDLE_BUS);
	virtual uint8_t sendInfoEvent(uint8_t channel, uint8_t length, uint8_t const * const data, uint32_t target_address, uint8_t target_channel, boolean busState = NEED_IDLE_BUS, uint8_t retries = 2);
	virtual void receiveInfoEvent(uint32_t senderAddress, uint8_t srcChan, uint8_t dstChan, uint8_t length, uint8_t const * const data);
  #endif
	// Allgemeiner "Key Event"
    virtual uint8_t sendKeyEvent(uint8_t channel, uint8_t keyPressNum, boolean longPress);
	// Key Event Routine mit Target fuer LinkSender 
    virtual uint8_t sendKeyEvent(uint8_t channel, uint8_t keyPressNum, boolean longPress,
				uint32_t target_address, uint8_t target_channel, boolean busState = NEED_IDLE_BUS, uint8_t retries = DEFAULT_SEND_RETRIES);
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
	boolean busIsIdle();
	
  private:
	uint8_t numChannels;    // number of channels
	HBWChannel** channels;  // channels
	HBWLinkSender* linkSender;
	HBWLinkReceiver* linkReceiver;
	
	// pins of config button and config LED
	uint8_t configPin;
	uint8_t ledPin;
	uint8_t txLedPin;
	uint8_t rxLedPin;
	
	// sendFrame macht ggf. Wiederholungen
	// onlyIfIdle: If this is set, then the bus must have been idle since 210+rand(0..100) ms
	// sendFrame returns...
	//   0 -> ok
	//   1 -> bus not idle (only if onlyIfIdle)
	//   2 -> three times no ACK (cannot occur for broadcasts or ACKs)
	sendFrameStatus sendFrame(boolean onlyIfIdle = false, uint8_t retries = DEFAULT_SEND_RETRIES);
	void sendAck();  // ACK fuer gerade verarbeitete Message senden

	// eigene Adresse setzen und damit auch random seed
	void setOwnAddress(unsigned long address);

	void readConfig();         // read config from EEPROM
    // get central address
    uint32_t getCentralAddress();
    void handleBroadcastAnnounce();
    void handleAfterReadConfig();
    void handleResetSystem();
	
	// the broadcast methods return...
	// 0 -> everything ok
	// 1 -> nothing sent because bus busy
	uint8_t broadcastAnnounce(uint8_t);  // channel

	uint8_t deviceType;        

	// write to EEPROM, but only if not "value" anyway
	// the uppermost 4 bytes are reserved for the device address and can only be changed if privileged = true
	void writeEEPROM(uint16_t address, uint8_t value, bool privileged = false );

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
	boolean frameComplete;
    uint32_t targetAddress;
	uint8_t frameDataLength;                 // Laenge der Daten
	uint8_t frameData[MAX_RX_FRAME_LENGTH];
	uint8_t frameControlByte;
    // Senderadresse beim Empfangen
	uint32_t senderAddress;

// carrier sense
//  last time we have received anything
    unsigned long lastReceivedTime;
//  current minimum idle time
//  will be initialized in constructor
    uint16_t minIdleTime;
	void receive();  // wird zyklisch aufgerufen
	boolean parseFrame();
	void sendFrameSingle();
	void sendFrameByte(uint8_t, uint16_t* crc = NULL);
	void crc16Shift(uint8_t, uint16_t*);

	void readAddressFromEEPROM();
	void determineSerial(uint8_t*, uint32_t address);

	void processEventGetLevel(uint8_t channel, uint8_t command);
	void processEventSetLock(uint8_t channel, boolean inhibit);
	void processEmessage(uint8_t const * const frameData);
	
    void factoryReset();
	void handleConfigButton();	// handle config button and config LED
	static uint8_t configButtonStatus;
	void handleStatusLEDs();	// handle Tx and Rx LEDs
	boolean txLEDStatus;
	boolean rxLEDStatus;

	
	struct s_PendingActions
	{
		uint8_t afterReadConfig : 1;
		uint8_t announced : 1;
		uint8_t resetSystem : 1;
		// uint8_t startBooter : 1;
		// uint8_t startFirmware : 1;
		uint8_t zeroCommunicationActive : 1;
	};
	static s_PendingActions pendingActions;
	
   #if defined(BOOTSTART)
	void (*bootloader_start) = (void *) BOOTSTART;   // TODO: Add bootloader?
   #endif
	// Arduino Reset via Software function declaration, point to address 0 (reset vector)
	void (* resetSoftware)(void) = 0;
	
	// Senden
	struct s_txFrame
	{
		uint32_t targetAddress;
		uint8_t controlByte;
		uint8_t dataLength;              // Laenge der Daten
		uint8_t data[MAX_RX_FRAME_LENGTH];
	};
	s_txFrame txFrame;

};


/*
*****************************
** DEBUG
*****************************
*/

extern Stream* hbwdebugstream;

void hbwdebughex(uint8_t b);

template <typename T>
#ifdef HBW_DEBUG
void hbwdebug(T msg) { if(hbwdebugstream) hbwdebugstream->print(msg); };
#else
void hbwdebug(T msg) { };
#endif

template <typename T>
#ifdef HBW_DEBUG
void hbwdebug(T msg, int base) { if(hbwdebugstream) hbwdebugstream->print(msg,base); };
#else
void hbwdebug(T msg, int base) { };
#endif

#endif /* HBWired_h */
