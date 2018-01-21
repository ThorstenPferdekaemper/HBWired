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
unsigned long timerTenMilliSeconds, lastRelayOperation[NUM_CHANNELS];
#define RELAY_PULSE_DUARTION 90  // HIG duration in ms, to set or reset double coil latching relay

// Port Status, d.h. Port ist auf 0 oder 1
byte portStatus[NUM_CHANNELS];


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




class HBWChSw : public HBWChannel {
  public:
    HBWChSw(uint8_t _pin, hbw_config_switch* _config);
    virtual uint8_t get(uint8_t* data);   
    virtual void loop(HBWDevice*, uint8_t channel);   
    virtual void set(HBWDevice*, uint8_t length, uint8_t const * const data);
    void initRelays();
  private:
    uint8_t pin;
    hbw_config_switch* config; // logging
    uint32_t lastFeedbackTime;  // when did we send the last feedback?
    uint16_t nextFeedbackDelay; // 0 -> no feedback pending
};


//HBWChannel* switches[NUM_CHANNELS];
HBWChSw* switches[NUM_CHANNELS];

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


//HBSenDevice* device = NULL;
HBSwDevice* device = NULL;



// refresh LED status (shift register)
//void setLEDs() {
//  myShReg_LED.batchWriteBegin();
//  for (byte i = 0; i < NUM_CHANNELS; i++) {
//    myShReg_LED.writeBit(i, (bitRead(portStatus[i / 8], i % 8)));
//  }
//  myShReg_LED.batchWriteEnd();
//}

// handle Relays (via shift register)
void operateRelays() {
  myShReg_RELAY.batchWriteBegin();
  myShReg_LED.batchWriteBegin();

  for (byte i = 0; i < NUM_CHANNELS; i++) {
    byte RelayBitPos = i * 2;
    if ((lastRelayOperation[i] + RELAY_PULSE_DUARTION) < millis() && lastRelayOperation[i] != 0) {  // time to remove power from coil?
      myShReg_RELAY.writeBit(RelayBitPos, LOW);
      myShReg_RELAY.writeBit(RelayBitPos + 1, LOW);
      lastRelayOperation[i] = 0;
    }
    myShReg_LED.writeBit(i, (bitRead(portStatus[i / 8], i % 8)));   // refresh LED status (shift register)
  }
  myShReg_RELAY.batchWriteEnd();
  myShReg_LED.batchWriteEnd();

  //setLEDs();    // use (portStatus[] to update LEDs
}



HBWChSw::HBWChSw(uint8_t _pin, hbw_config_switch* _config) {
  pin = _pin;
  config = _config;
  nextFeedbackDelay = 0;
  lastFeedbackTime = 0;
  // Pin auf OUTPUT
  // ...und auf HIGH (also 0) setzen, da sonst die Relais anziehen
  // TODO: das sollte einstellbar sein
//  digitalWrite(pin, HIGH);
//    pinMode(pin,OUTPUT);
};
 
void HBWChSw::initRelays() {    //need intial reset (or set if inverterted) for all relays - bistable relays may have incorrect state!!!
//  digitalWrite(pin, config->inverted ? HIGH : LOW);
  //bitWrite(portStatus[pin / 8], pin % 8, config->inverted ? HIGH : LOW);

// TODO: function start?
  byte RelayBitPos = pin * 2;
  if (config->inverted) { // on - perform set
    myShReg_RELAY.writeBit(RelayBitPos +1, LOW);  // reset coil
    myShReg_RELAY.writeBit(RelayBitPos, HIGH);    // set coil
    bitWrite(portStatus[pin / 8], pin % 8, HIGH);    
  }
  else {  // off - perform reset
    myShReg_RELAY.writeBit(RelayBitPos, LOW);      // set coil
    myShReg_RELAY.writeBit(RelayBitPos +1, HIGH);  // reset coil
    bitWrite(portStatus[pin / 8], pin % 8, LOW);
  }
  if (millis() == 0)  // TODO: check for possible overflow condition as well (or check if difference is too high? e.g. max coild swicht on time < 1 second?
    lastRelayOperation[pin] = 1;
  else
    lastRelayOperation[pin] = millis();  // Relay coils must be set two low after some ms (bistable Relays!!)
// TODO: function end?
}


void HBWChSw::set(HBWDevice* device, uint8_t length, uint8_t const * const data) {
  if (config->output_unlocked) {  //0=LOCKED, 1=UNLOCKED
    byte level = (*data);
    
    if (level > 200) // toggle
      level = !(bitRead(portStatus[pin / 8], pin % 8)); // get current state and negate
    else if (level)   // set to 0 or 1
      level = (HIGH ^ config->inverted);
    else
      level = (LOW ^ config->inverted);
// TODO: function start?  ----- or move operations to "operateRelays()"? - Needed for zero crossing function. Just set portStatus[]? + add portStatusDesired[]?
    byte RelayBitPos = pin * 2;
    if (level) { // on - perform set
      myShReg_RELAY.writeBit(RelayBitPos +1, LOW);  // reset coil
      myShReg_RELAY.writeBit(RelayBitPos, HIGH);    // set coil
      bitWrite(portStatus[pin / 8], pin % 8, HIGH);
    }
    else {  // off - perform reset
      myShReg_RELAY.writeBit(RelayBitPos, LOW);      // set coil
      myShReg_RELAY.writeBit(RelayBitPos +1, HIGH);  // reset coil
      bitWrite(portStatus[pin / 8], pin % 8, LOW);
    }
    
    if (millis() == 0)
      lastRelayOperation[pin] = 1;
    else
      lastRelayOperation[pin] = millis();  // Relay coils must be set two low after some ms (bistable Relays!!)
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
    nextFeedbackDelay = device->getLoggingTime() * 100;
  }
};


uint8_t HBWChSw::get(uint8_t* data) {
  if (bitRead(portStatus[pin / 8], pin % 8) ^ config->inverted)
    (*data) = 200;
  else
    (*data) = 0;
  return 1;
};

void HBWChSw::loop(HBWDevice* device, uint8_t channel) {
  // feedback trigger set?
    if(!nextFeedbackDelay) return;
    unsigned long now = millis();
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
  
  myShReg_LED.setBitCount(8);
  myShReg_LED.setPins(shiftRegLED_Data, shiftRegLED_Clock, shiftRegLED_Latch);
  myShReg_RELAY.setBitCount(16);
  myShReg_RELAY.setPins(shiftRegRELAY_Data, shiftRegRELAY_Clock, shiftRegRELAY_Latch);


  Serial.begin(19200);
  rs485.begin();    // RS485 via SoftwareSerial

   // create channels
   uint8_t RelayNumber[NUM_CHANNELS] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11}; // TODO: remove? just need channel info? - or use this to freely map relays to channels?
  // assing switches (relay) pins
   for(uint8_t i = 0; i < NUM_CHANNELS; i++){
      switches[i] = new HBWChSw(RelayNumber[i], &(hbwconfig.switchcfg[i]));
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
  
  if (timerTenMilliSeconds + 10 < millis()) { // check about every 10 ms
    timerTenMilliSeconds = millis();
    operateRelays();
  }
};

