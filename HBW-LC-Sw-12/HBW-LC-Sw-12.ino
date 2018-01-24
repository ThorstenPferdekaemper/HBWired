//*******************************************************************
//
// HBW-LC-Sw-12
//
// Homematic Wired Hombrew Hardware
// Arduino NANO als Homematic-Device
// 6 + 6 Relais [TODO: über Shiftregister und 6 Kanal Strommessung]
// [TODO: Peering mit Zeitschaltfuntion]
// - Active HIGH oder LOW kann konfiguriert werden

// TODO: Test der Links (HBWLinkSwitchSimple) & XML
//
//*******************************************************************


#define HMW_DEVICETYPE 0x93
#define HARDWARE_VERSION 0x01
#define FIRMWARE_VERSION 0x0100

#define NUM_CHANNELS 12
#define NUM_LINKS 36
#define LINKADDRESSSTART 0x40

#include "HBWSoftwareSerial.h"
#include "FreeRam.h"    


// HB Wired protocol and module
#include "HBWired.h"
#include "HBWLinkSwitchSimple.h"
//#include "HBWSwitch.h"

// shift register library
#include <Shifty.h>

#define RS485_RXD 4
#define RS485_TXD 2
#define RS485_TXEN 3  // Transmit-Enable

// HBWSoftwareSerial can only do 19200 baud
HBWSoftwareSerial rs485(RS485_RXD, RS485_TXD); // RX, TX


// Pins
#define BUTTON 8  // Button fuer Factory-Reset etc.
#define LED LED_BUILTIN        // Signal-LED


#define shiftRegLED_Data  6
#define shiftRegLED_Clock 7
#define shiftRegLED_Latch 8

#define shiftRegRELAY_Data  9
#define shiftRegRELAY_Clock 10
#define shiftRegRELAY_Latch 11

Shifty myShReg_LED;
Shifty myShReg_RELAY;
// unsigned long timerTenMilliSeconds, relayOperationTimeStart[NUM_CHANNELS];
#define RELAY_PULSE_DUARTION 60  // HIG duration in ms, to set or reset double coil latching relay
/////#define RELAY_MAX_PULSE_DUARTION 200  // maximum HIG duration in ms, to un

// Port Status, d.h. Port ist auf 0 oder 1
// byte portStatus[NUM_CHANNELS];
//TODO: check if this should/could be stored in EEPROM, to restore output relay values


struct hbw_config_switch {
  uint8_t logging:1;              // 0x0000
  uint8_t output_unlocked:1;      // 0x07:1    0=LOCKED, 1=UNLOCKED
  uint8_t inverted:1;             // 0x07:2
  uint8_t        :5;              // 0x0000
  uint8_t dummy;
};

struct hbw_config {
  uint8_t logging_time;     // 0x01
  uint32_t central_address;  // 0x02 - 0x05
  uint8_t direct_link_deactivate:1;   // 0x06:0
  uint8_t              :7;   // 0x06:1-7
  hbw_config_switch switchcfg[NUM_CHANNELS]; // 0x07-0x... ?
} hbwconfig;



class HBWChanSw : public HBWChannel {
  public:
    HBWChanSw(uint8_t _relayPos, uint8_t _ledPos,hbw_config_switch* _config);
    virtual uint8_t get(uint8_t* data);   
    virtual void loop(HBWDevice*, uint8_t channel);   
    virtual void set(HBWDevice*, uint8_t length, uint8_t const * const data);
    void initRelays();
  private:
    uint8_t relayPos;
    uint8_t ledPos;
    hbw_config_switch* config; // logging
    uint32_t lastFeedbackTime;  // when did we send the last feedback?
    uint16_t nextFeedbackDelay; // 0 -> no feedback pending
    
    bool portStatus;	// Port Status, d.h. Port ist auf 0 oder 1
    bool operateRelay;
    unsigned long relayOperationTimeStart;
};


HBWChanSw* switches[NUM_CHANNELS];


class HBSwDevice : public HBWDevice {
    public: 
    HBSwDevice(uint8_t _devicetype, uint8_t _hardware_version, uint16_t _firmware_version,
            Stream* _rs485, uint8_t _txen, 
            uint8_t _configSize, void* _config, 
        uint8_t _numChannels, HBWChannel** _channels,
        Stream* _debugstream, HBWLinkSender* linksender = NULL, HBWLinkReceiver* linkreceiver = NULL) :
          HBWDevice(_devicetype, _hardware_version, _firmware_version,
            _rs485, _txen, _configSize, _config, _numChannels, ((HBWChannel**)(_channels)),
            _debugstream, linksender, linkreceiver) {
      // looks like virtual methods are not properly called here
      afterReadConfig();
    };

    void afterReadConfig() {
        // defaults setzen
        if(hbwconfig.logging_time == 0xFF) hbwconfig.logging_time = 20;
        // if(config.central_address == 0xFFFFFFFF) config.central_address = 0x00000001;
        for(uint8_t channel = 0; channel < NUM_CHANNELS; channel++){
            switches[channel]->initRelays();
        };
    };
};


HBSwDevice* device = NULL;


// refresh LED status (shift register)
//void setLEDs() {
//  myShReg_LED.batchWriteBegin();
//  for (byte i = 0; i < NUM_CHANNELS; i++) {
//    myShReg_LED.writeBit(i, (bitRead(portStatus[i / 8], i % 8)));
//  }
//  myShReg_LED.batchWriteEnd();
//}

// handle relays and LEDs (via shift register)
// void operateRelays() {
  // myShReg_RELAY.batchWriteBegin();
  // myShReg_LED.batchWriteBegin();

  // for (byte i = 0; i < NUM_CHANNELS; i++) {
    // byte RelayBitPos = i * 2;
    // if ((relayOperationTimeStart[i] + RELAY_PULSE_DUARTION) < millis() && relayOperationTimeStart[i] != 0) {  // time to remove power from coil?
      // myShReg_RELAY.writeBit(RelayBitPos, LOW);
      // myShReg_RELAY.writeBit(RelayBitPos + 1, LOW);
      // relayOperationTimeStart[i] = 0;
    // }
    // myShReg_LED.writeBit(i, (bitRead(portStatus[i / 8], i % 8)));   // refresh LED status (shift register)
  // }
  // myShReg_RELAY.batchWriteEnd();
  // myShReg_LED.batchWriteEnd();

  // //setLEDs();    // use (portStatus[] to update LEDs
// }


HBWChanSw::HBWChanSw(uint8_t _relayPos, uint8_t _ledPos, hbw_config_switch* _config) {
  relayPos = _relayPos;
  ledPos =_ledPos;
  config = _config;
  nextFeedbackDelay = 0;
  lastFeedbackTime = 0;
  
  relayOperationTimeStart = 0;
  operateRelay = false;
};
 
void HBWChanSw::initRelays() {    //need intial reset (or set if inverterted) for all relays - bistable relays may have incorrect state!!!
//  digitalWrite(pin, config->inverted ? HIGH : LOW);
  //bitWrite(portStatus[pin / 8], pin % 8, config->inverted ? HIGH : LOW);

// TODO: function start?
  // byte RelayBitPos = relayPos * 2;
  if (config->inverted) { // on - perform set
    myShReg_RELAY.writeBit(relayPos +1, LOW);  // reset coil
    myShReg_RELAY.writeBit(relayPos, HIGH);    // set coil
    // bitWrite(portStatus[pin / 8], pin % 8, HIGH);
	portStatus = HIGH;
  }
  else {  // off - perform reset
    myShReg_RELAY.writeBit(relayPos, LOW);      // set coil
    myShReg_RELAY.writeBit(relayPos +1, HIGH);  // reset coil
    // bitWrite(portStatus[pin / 8], pin % 8, LOW);
	portStatus = LOW;
  }
  relayOperationTimeStart = millis();  // Relay coils must be set two low after some ms (bistable Relays!!)
  operateRelay = true;
// TODO: function end?
}


void HBWChanSw::set(HBWDevice* device, uint8_t length, uint8_t const * const data) {
  	
//	unsigned long now = millis();
  if (config->output_unlocked) {  //0=LOCKED, 1=UNLOCKED
    byte level = (*data);
    
    if (level > 200) // toggle
      // level = !(bitRead(portStatus[pin / 8], pin % 8)); // get current state and negate
	  level = !portStatus; // get current state and negate
    else if (level)   // set to 0 or 1
      level = (HIGH ^ config->inverted);
    else
      level = (LOW ^ config->inverted);
// TODO: function start?  ----- or move operations to "operateRelays()"? - Needed for zero crossing function. Just set portStatus[]? + add portStatusDesired[]?
    // byte RelayBitPos = relayPos * 2;
    if (level) { // on - perform set
      myShReg_RELAY.writeBit(relayPos +1, LOW);  // reset coil
      myShReg_RELAY.writeBit(relayPos, HIGH);    // set coil
     // bitWrite(portStatus[pin / 8], pin % 8, HIGH);
      portStatus = HIGH;
    }
    else {  // off - perform reset
      myShReg_RELAY.writeBit(relayPos, LOW);      // set coil
      myShReg_RELAY.writeBit(relayPos +1, HIGH);  // reset coil
     // bitWrite(portStatus[pin / 8], pin % 8, LOW);
      portStatus = LOW;
    }
    
    relayOperationTimeStart = millis();  // Relay coils must be set two low after some ms (bistable Relays!!)
    operateRelay = true;
// TODO: function end?

    //setLEDs();    // use (portStatus[] to update LEDs
        
//    if(*data > 200) {   // toggle
//      digitalWrite(pin, digitalRead(pin) ? LOW : HIGH);
//    }else{   // on or off
//      if (*data)
//        digitalWrite(pin, HIGH ^ config->inverted);
//      else
//        digitalWrite(pin, LOW ^ config->inverted);
//    }    
  }
  // Logging
  // (logging is considered for locked channels)
  if(!nextFeedbackDelay && config->logging) {
    lastFeedbackTime = millis();
//	lastFeedbackTime = now;
    nextFeedbackDelay = device->getLoggingTime() * 100;
  }
};


uint8_t HBWChanSw::get(uint8_t* data) {
  // if (bitRead(portStatus[pin / 8], pin % 8) ^ config->inverted)
  if (portStatus ^ config->inverted)
    (*data) = 200;
  else
    (*data) = 0;
  return 1;
};

void HBWChanSw::loop(HBWDevice* device, uint8_t channel) {
	
	unsigned long now = millis();
	// operate relays and LEDs (via shift register)
	// myShReg_LED.batchWriteBegin();

	if (((now - relayOperationTimeStart) >= RELAY_PULSE_DUARTION) && operateRelay == true) {  // time to remove power from coil?
		myShReg_RELAY.batchWriteBegin();		
		myShReg_RELAY.writeBit(relayPos, LOW);
		myShReg_RELAY.writeBit(relayPos + 1, LOW);
		myShReg_RELAY.batchWriteEnd();
		operateRelay = false;

		myShReg_LED.writeBit(ledPos, portStatus);   // refresh LED status (shift register)
	}

	// myShReg_LED.batchWriteEnd();
	
	
  // feedback trigger set?
    if(!nextFeedbackDelay) return;
//    unsigned long now = millis();
    if(now - lastFeedbackTime < nextFeedbackDelay) return;
    lastFeedbackTime = now;  // at least last time of trying
    // sendInfoMessage returns 0 on success, 1 if bus busy, 2 if failed
  // we know that the level has only 1 byte here
  uint8_t level;
    get(&level);
    uint8_t errcode = device->sendInfoMessage(channel, 1, &level);   
    if(errcode == 1) {  // bus busy
    // try again later, but insert a small delay
      nextFeedbackDelay = 250;
    }else{
      nextFeedbackDelay = 0;
    }
}


void setup()
{
  pinMode(BUTTON, INPUT_PULLUP);
  pinMode(LED, OUTPUT);
  
  myShReg_LED.setBitCount(NUM_CHANNELS);
  myShReg_LED.setPins(shiftRegLED_Data, shiftRegLED_Clock, shiftRegLED_Latch);
  myShReg_RELAY.setBitCount(NUM_CHANNELS *2);
  myShReg_RELAY.setPins(shiftRegRELAY_Data, shiftRegRELAY_Clock, shiftRegRELAY_Latch);


  Serial.begin(19200);
  rs485.begin();    // RS485 via SoftwareSerial

   // create channels
   uint8_t RelayBitPos[NUM_CHANNELS] = {0, 2, 4, 8, 10, 12, 16, 18, 20, 24, 26, 28}; // TODO: keep like this?? -  use this to freely map relays to channels?
   uint8_t LEDBitPos[NUM_CHANNELS] = {0, 1, 2, 3, 4, 5, 8, 9, 10, 11, 12, 13};
  // assing switches (relay) pins
   for(uint8_t i = 0; i < NUM_CHANNELS; i++){
      switches[i] = new HBWChanSw(RelayBitPos[i], LEDBitPos[i], &(hbwconfig.switchcfg[i]));
   };

  device = new HBSwDevice(HMW_DEVICETYPE, HARDWARE_VERSION, FIRMWARE_VERSION,
                         &rs485,RS485_TXEN,sizeof(hbwconfig),&hbwconfig,
                         NUM_CHANNELS,(HBWChannel**)switches,
                         &Serial,
                         NULL, new HBWLinkSwitchSimple(NUM_LINKS,LINKADDRESSSTART));
   
  device->setConfigPins();  // 8 and 13 is the default
 
  hbwdebug(F("B: 2A "));
  hbwdebug(freeRam());
  hbwdebug(F("\n"));
}


void loop()
{
  device->loop();
  
//  if (timerTenMilliSeconds + 10 < millis()) { // check about every 10 ms
//    timerTenMilliSeconds = millis();
//    operateRelays();
//  }
};

