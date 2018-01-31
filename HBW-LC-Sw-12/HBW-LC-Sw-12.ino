//*******************************************************************
//
// HBW-LC-Sw-12
//
// Homematic Wired Hombrew Hardware
// Arduino NANO als Homematic-Device
// 6 + 6 bistabile Relais über Shiftregister und [TODO: 6 Kanal Strommessung]
// [TODO: Peering mit Zeitschaltfuntion]
// - Active HIGH oder LOW kann konfiguriert werden

// TODO: Test der Links (HBWLinkSwitchSimple) & XML
//
//*******************************************************************
// Changes
// v0.1
// - initial version


#define HMW_DEVICETYPE 0x93
#define HARDWARE_VERSION 0x01
#define FIRMWARE_VERSION 0x0001
//#define USE_HARDWARE_SERIAL

#define NUM_CHANNELS 12
#define NUM_LINKS 36
#define LINKADDRESSSTART 0x40

#include "FreeRam.h"


// HB Wired protocol and module
#include "HBWired.h"
#include "HBWLinkSwitchSimple.h"
//#include "HBWSwitch.h"

// shift register library
#include "ShiftRegister74HC595.h"

#ifdef USE_HARDWARE_SERIAL
  #define RS485_TXEN 2  // Transmit-Enable
  // 6 realys and LED attached to 3 shiftregisters
  #define shiftRegOne_Data  10       //DS serial data input
  #define shiftRegOne_Clock 3       //SH_CP shift register clock input
  #define shiftRegOne_Latch 4       //ST_CP storage register clock input
  // extension shifregister for another 6 relays and LEDs
  #define shiftRegTwo_Data  8
  #define shiftRegTwo_Clock 7
  #define shiftRegTwo_Latch 9
#else
  #define RS485_RXD 4
  #define RS485_TXD 2
  #define RS485_TXEN 3  // Transmit-Enable
  // 6 realys and LED attached to 3 shiftregisters
  #define shiftRegOne_Data  10       //DS serial data input
  #define shiftRegOne_Clock 9       //SH_CP shift register clock input
  #define shiftRegOne_Latch 11       //ST_CP storage register clock input
  // extension shifregister for another 6 relays and LEDs
  #define shiftRegTwo_Data  5
  #define shiftRegTwo_Clock 7
  #define shiftRegTwo_Latch 6
  
  #include "HBWSoftwareSerial.h"
  // HBWSoftwareSerial can only do 19200 baud
  HBWSoftwareSerial rs485(RS485_RXD, RS485_TXD); // RX, TX
#endif



// Pins
#ifdef USE_HARDWARE_SERIAL
  #define BUTTON A6  // Button fuer Factory-Reset etc.
  #define ANALOG_CONFIG_BUTTON  //tell handleConfigButton() to use analogRead()
#else
  #define BUTTON 8  // Button fuer Factory-Reset etc.
#endif

#define LED LED_BUILTIN        // Signal-LED

#define RELAY_PULSE_DUARTION 80  // HIG duration in ms, to set or reset double coil latching relay



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
    //HBWChanSw(uint8_t _relayPos, uint8_t _ledPos, hbw_config_switch* _config);
    HBWChanSw(uint8_t _relayPos, uint8_t _ledPos, ShiftRegister74HC595* _shiftRegister, hbw_config_switch* _config);
    virtual uint8_t get(uint8_t* data);
    virtual void loop(HBWDevice*, uint8_t channel);
    virtual void set(HBWDevice*, uint8_t length, uint8_t const * const data);
    void initRelays();
  private:
    uint8_t relayPos; // bit position for actual IO port
    uint8_t ledPos;
    ShiftRegister74HC595* shiftRegister;  // allow function calls to the correct shift register
    hbw_config_switch* config; // logging
    uint32_t lastFeedbackTime;  // when did we send the last feedback?
    uint16_t nextFeedbackDelay; // 0 -> no feedback pending
    
//    bool portStatus;	// Port Status, d.h. Port ist auf 0 oder 1
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

//ShiftRegister74HC595 myShReg_LED(2, shiftRegLED_Data, shiftRegLED_Clock, shiftRegLED_Latch);
//ShiftRegister74HC595 myShReg_RELAY(4, shiftRegRELAY_Data, shiftRegRELAY_Clock, shiftRegRELAY_Latch);

ShiftRegister74HC595 myShReg_one(3, shiftRegOne_Data, shiftRegOne_Clock, shiftRegOne_Latch);
ShiftRegister74HC595 myShReg_two(3, shiftRegTwo_Data, shiftRegTwo_Clock, shiftRegTwo_Latch);


HBWChanSw::HBWChanSw(uint8_t _relayPos, uint8_t _ledPos, ShiftRegister74HC595* _shiftRegister, hbw_config_switch* _config) {
  relayPos = _relayPos;
  ledPos =_ledPos;
  config = _config;
  shiftRegister = _shiftRegister;
  nextFeedbackDelay = 0;
  lastFeedbackTime = 0;
  
  relayOperationTimeStart = 0;
  operateRelay = false;
};


void HBWChanSw::initRelays() {    //need intial reset (or set if inverterted) for all relays - bistable relays may have incorrect state!!!

  if (config->inverted) { // on - perform set
    shiftRegister->set(relayPos +1, LOW);    // reset coil
    shiftRegister->set(relayPos, HIGH);  // set coil
    shiftRegister->set(ledPos, HIGH); // LED  
  }
  else {  // off - perform reset
    shiftRegister->set(relayPos, LOW);      // set coil
    shiftRegister->set(relayPos +1, HIGH);  // reset coil
    shiftRegister->set(ledPos, LOW); // LED
  }
  //TODO: add sleep? setting 12 relays at once would consume high current...
  
  relayOperationTimeStart = millis();  // Relay coils must be set two low after some ms (bistable Relays!!)
  operateRelay = true;
}


void HBWChanSw::set(HBWDevice* device, uint8_t length, uint8_t const * const data) {
  	
	unsigned long now = millis();
 
  if (config->output_unlocked) {  //0=LOCKED, 1=UNLOCKED
    byte level = (*data);

    if (level > 200) // toggle
	      level = !shiftRegister->get(ledPos); // get current state and negate
    else if (level)   // set to 0 or 1
      level = (HIGH ^ config->inverted);
    else
      level = (LOW ^ config->inverted);
// TODO: move to loop? - Needed for zero crossing function. Just set portStatus[]? + add portStatusDesired[]?

    if (level) { // on - perform set
      shiftRegister->set(relayPos +1, LOW);    // reset coil
      shiftRegister->set(relayPos, HIGH);  // set coil
    }
    else {  // off - perform reset
      shiftRegister->set(relayPos, LOW);      // set coil
      shiftRegister->set(relayPos +1, HIGH);  // reset coil
    }
    shiftRegister->set(ledPos, level); // LED
    
//    if (level) { // on - perform set
//      myShReg_RELAY.set(relayPos +1, LOW);  // reset coil
//      myShReg_RELAY.set(relayPos, HIGH);    // set coil
//     // bitWrite(portStatus[pin / 8], pin % 8, HIGH);
////      portStatus = HIGH;
//    myShReg_LED.set(ledPos, HIGH);
//    }
//    else {  // off - perform reset
//      myShReg_RELAY.set(relayPos, LOW);      // set coil
//      myShReg_RELAY.set(relayPos +1, HIGH);  // reset coil
//     // bitWrite(portStatus[pin / 8], pin % 8, LOW);
////      portStatus = LOW;
//    myShReg_LED.set(ledPos, LOW);
    
    relayOperationTimeStart = now;  // Relay coils must be set two low after some ms (bistable Relays!!)
    operateRelay = true;
  }
  // Logging
  // (logging is considered for locked channels)
  if(!nextFeedbackDelay && config->logging) {
//    lastFeedbackTime = millis();
  	lastFeedbackTime = now;
    nextFeedbackDelay = device->getLoggingTime() * 100;
  }
};


uint8_t HBWChanSw::get(uint8_t* data) {
  
  //if (portStatus ^ config->inverted)
  if (shiftRegister->get(ledPos) ^ config->inverted)
    (*data) = 200;
  else
    (*data) = 0;
  return 1;
};


void HBWChanSw::loop(HBWDevice* device, uint8_t channel) {
	
	unsigned long now = millis();

	if (((now - relayOperationTimeStart) >= RELAY_PULSE_DUARTION) && operateRelay == true) {  // time to remove power from coil?

    shiftRegister->setNoUpdate(relayPos +1, LOW);    // reset coil
    shiftRegister->setNoUpdate(relayPos, LOW);  // set coil
    shiftRegister->updateRegisters();
    
		operateRelay = false;
	}
	
  if(!nextFeedbackDelay)  // feedback trigger set?
    return;
  if (now - lastFeedbackTime < nextFeedbackDelay)
    return;
  lastFeedbackTime = now;  // at least last time of trying
  // sendInfoMessage returns 0 on success, 1 if bus busy, 2 if failed
  // we know that the level has only 1 byte here
  uint8_t level;
  get(&level);
  uint8_t errcode = device->sendInfoMessage(channel, 1, &level);   
  if (errcode == 1)  // bus busy
  // try again later, but insert a small delay
    nextFeedbackDelay = 250;
  else
    nextFeedbackDelay = 0;
}

  
void setup()
{
   // assing switches (relay) pins
//   uint8_t RelayBitPos[NUM_CHANNELS] = {0, 2, 4, 8, 10, 12, 16, 18, 20, 24, 26, 28};
//   uint8_t LEDBitPos[NUM_CHANNELS] = {0, 1, 2, 3, 4, 5, 8, 9, 10, 11, 12, 13};
   uint8_t LEDBitPos[6] = {0, 1, 2, 3, 4, 5};    // shift register 1: 6 LEDs // not only used for the LEDs, but also to keep track of the output state!
   uint8_t RelayBitPos[6] = {8, 10, 12,          // shift register 2: 3 relays (with 2 coils each)
                             16, 18, 20};        // shift register 3: 3 relays (with 2 coils each)
  // create channels
  for(uint8_t i = 0; i < NUM_CHANNELS; i++){
    if (i < 6)
      switches[i] = new HBWChanSw(RelayBitPos[i], LEDBitPos[i], &myShReg_one, &(hbwconfig.switchcfg[i]));
    else
      switches[i] = new HBWChanSw(RelayBitPos[i %6], LEDBitPos[i %6], &myShReg_two, &(hbwconfig.switchcfg[i]));
    //switches[i] = new HBWChanSw(BitPos[i], BitPos[i/8], &(hbwconfig.switchcfg[i]));
  };

  #ifdef USE_HARDWARE_SERIAL
    pinMode(LED, OUTPUT);
    Serial.begin(19200, SERIAL_8E1);
    
    device = new HBSwDevice(HMW_DEVICETYPE, HARDWARE_VERSION, FIRMWARE_VERSION,
                           &Serial, RS485_TXEN, sizeof(hbwconfig), &hbwconfig,
                           NUM_CHANNELS,(HBWChannel**)switches,
                           NULL,
                           NULL, new HBWLinkSwitchSimple(NUM_LINKS,LINKADDRESSSTART));
  #else
    Serial.begin(19200);
    rs485.begin();    // RS485 via SoftwareSerial
    
    device = new HBSwDevice(HMW_DEVICETYPE, HARDWARE_VERSION, FIRMWARE_VERSION,
                           &rs485, RS485_TXEN, sizeof(hbwconfig), &hbwconfig,
                           NUM_CHANNELS, (HBWChannel**)switches,
                           &Serial,
                           NULL, new HBWLinkSwitchSimple(NUM_LINKS,LINKADDRESSSTART));
     
    device->setConfigPins();  // 8 (button) and 13 (led) is the default
  #endif
   
  hbwdebug(F("B: 2A "));
  hbwdebug(freeRam());
  hbwdebug(F("\n"));

}


void loop()
{
  device->loop();
};

