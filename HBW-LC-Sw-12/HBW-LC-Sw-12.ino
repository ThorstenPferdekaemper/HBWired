//*******************************************************************
//
// HBW-LC-Sw-12
//
// Homematic Wired Hombrew Hardware
// Arduino NANO als Homematic-Device
// 6 + 6 bistabile Relais über Shiftregister und [TODO: 6 Kanal Strommessung]
// 
// - Active HIGH oder LOW kann konfiguriert werden
// - Direktes peering mit Zeitschaltfuntion (on/off delay, on/off time, toggle/toggle to counter)

// TODO: Test der hardware...
//
// http://loetmeister.de/Elektronik/homematic/index.htm#modules
//
//*******************************************************************
// Changes
// v0.1
// - initial version
// v0.2
// - Added enhanced peering and basic state machine
// v0.3
// - Added analog inputs


#define HMW_DEVICETYPE 0x93
#define HARDWARE_VERSION 0x01
#define FIRMWARE_VERSION 0x0003
//#define USE_HARDWARE_SERIAL   // use hardware serial (USART), this disables debug output

#define NUM_SW_CHANNELS 12
#define NUM_AD_CHANNELS 6
#define NUM_CHANNELS 18
#define NUM_LINKS 36
#define LINKADDRESSSTART 0x40

#include "FreeRam.h"


// HB Wired protocol and module
#include "HBWired.h"
#include "HBWLinkSwitchAdvanced.h"
//#include "HBWSwitchSerialAdvanced.h"
#include "HBWAnalogIn.h"

// shift register library
#include "ShiftRegister74HC595.h"


// Pins
#ifdef USE_HARDWARE_SERIAL
  #define RS485_TXEN 2  // Transmit-Enable
  // 6 realys and LED attached to 3 shiftregisters
  #define shiftRegOne_Data  10       //DS serial data input
  #define shiftRegOne_Clock 3       //SH_CP shift register clock input
  #define shiftRegOne_Latch 4       //ST_CP storage register clock input
  // extension shifregister for another 6 relays and LEDs
  #define shiftRegTwo_Data  12
  #define shiftRegTwo_Clock 8
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

#ifdef USE_HARDWARE_SERIAL
  #define BUTTON A6  // Button fuer Factory-Reset etc.
#else
  #define BUTTON 8  // Button fuer Factory-Reset etc.
#endif

#define LED LED_BUILTIN        // Signal-LED

#define CT_PIN1 A0  // analog input for current transformer, switch channel 1
#define CT_PIN2 A1  // analog input for current transformer, switch channel 2
#define CT_PIN3 A2
#define CT_PIN4 A3
#define CT_PIN5 A4
#define CT_PIN6 A5


#define RELAY_PULSE_DUARTION 80  // HIG duration in ms, to set or reset double coil latching relay

// peering/link values must match the XML/EEPROM values!
#define JT_ONDELAY 0x00
#define JT_ON 0x01
#define JT_OFFDELAY 0x02
#define JT_OFF 0x03
#define JT_NO_JUMP_IGNORE_COMMAND 0x04
#define ON_TIME_ABSOLUTE 0x0A
#define OFF_TIME_ABSOLUTE 0x0B
#define ON_TIME_MINIMAL 0x0C
#define OFF_TIME_MINIMAL 0x0D
#define FORCE_STATE_CHANGE 0xFF


struct hbw_config_switch {
  uint8_t logging:1;              // 0x0000
  uint8_t output_unlocked:1;      // 0x07:1    0=LOCKED, 1=UNLOCKED
  uint8_t n_inverted:1;           // 0x07:2    0=inverted, 1=not inverted (device reset will set to 1!)
  uint8_t        :5;              // 0x0000
  uint8_t dummy;
};

struct hbw_config {
  uint8_t logging_time;     // 0x01
  uint32_t central_address;  // 0x02 - 0x05
  uint8_t direct_link_deactivate:1;   // 0x06:0
  uint8_t              :7;   // 0x06:1-7
  hbw_config_switch switchcfg[NUM_SW_CHANNELS]; // 0x07-0x1F (2 bytes each)
  hbw_config_analog_in ctcfg[NUM_AD_CHANNELS];    // 0x20-0x2C (2 bytes each)
} hbwconfig;


class HBWChanSw : public HBWChannel {
  public:
    HBWChanSw(uint8_t _relayPos, uint8_t _ledPos, ShiftRegister74HC595* _shiftRegister, hbw_config_switch* _config);
    virtual uint8_t get(uint8_t* data);
    virtual void loop(HBWDevice*, uint8_t channel);
    virtual void set(HBWDevice*, uint8_t length, uint8_t const * const data);
    virtual void afterReadConfig();
//    virtual void peeringEventTrigger(HBWDevice* device, uint8_t const * const data);

  private:
    uint8_t relayPos; // bit position for actual IO port
    uint8_t ledPos;
    ShiftRegister74HC595* shiftRegister;  // allow function calls to the correct shift register
    hbw_config_switch* config; // logging
    uint32_t lastFeedbackTime;  // when did we send the last feedback?
    uint16_t nextFeedbackDelay; // 0 -> no feedback pending
    
    bool operateRelay;
    unsigned long relayOperationTimeStart;
    
    // set from links/peering (implements state machine)
    void setOutput(uint8_t const * const data);
    uint8_t getNextState(uint8_t bitshift);
    inline uint32_t convertTime(uint8_t timeValue);
    uint8_t actiontype;
    uint8_t onDelayTime;
    uint8_t onTime;
    uint8_t offDelayTime;
    uint8_t offTime;
    union { uint16_t WORD;
      uint8_t jt_hi_low[2];
    } jumpTargets;
    boolean stateTimerRunning;
    uint8_t currentState;
    uint8_t nextState;
    unsigned long stateCangeWaitTime;
    unsigned long lastStateChangeTime;
    uint8_t lastKeyEvent;
};


HBWChannel* channels[NUM_CHANNELS];


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
    };
    virtual void afterReadConfig();
};

// device specific defaults
void HBSwDevice::afterReadConfig() {
  if(hbwconfig.logging_time == 0xFF) hbwconfig.logging_time = 20;
};


HBSwDevice* device = NULL;

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
  
  onTime = 0xFF;
  offTime = 0xFF;
  jumpTargets.WORD = 0;
  stateTimerRunning = false;
  stateCangeWaitTime = 0;
  lastStateChangeTime = 0;
};

// channel specific settings or defaults
void HBWChanSw::afterReadConfig() {
  
  //need intial reset (or set if inverterted) for all relays - bistable relays may have incorrect state!!!
  if (config->n_inverted) { // off - perform reset
    shiftRegister->set(relayPos, LOW);      // set coil
    shiftRegister->set(relayPos +1, HIGH);  // reset coil
  }
  else {  // on - perform set
    shiftRegister->set(relayPos +1, LOW);    // reset coil
    shiftRegister->set(relayPos, HIGH);  // set coil
  }
  //TODO: add delay? setting 12 relays at once would consume high current...

  shiftRegister->set(ledPos, LOW); // LED
  currentState = JT_OFF;
  nextState = currentState; // no action for state machine needed

  relayOperationTimeStart = millis();  // Relay coils must be set two low after some ms (bistable Relays!!)
  operateRelay = true;
}


//void HBWChanSw::peeringEventTrigger(HBWDevice* device, uint8_t const * const data) {
void HBWChanSw::set(HBWDevice* device, uint8_t length, uint8_t const * const data) {

  if (length >= 8) {  // got called with additional peering parameters -- test for correct NUM_PEER_PARAMS
    actiontype = *(data);
    uint8_t keyEvent = data[7];

#ifndef USE_HARDWARE_SERIAL
  hbwdebug(F("aV: "));
  hbwdebughex(keyEvent);
  hbwdebug(F("\n"));
#endif

    if ((actiontype & B00001111) >1) {   // TOGGLE_USE
      if (!stateTimerRunning && lastKeyEvent != keyEvent) {   // do not interrupt running timer, ignore LONG_MULTIEXECUTE
        byte level;
        if ((actiontype & B00001111) == 2)   // TOGGLE_TO_COUNTER
          level = ((keyEvent %2 == 0) ? 0 : 200);  //  (switch ON at odd numbers, OFF at even numbers)
        else if ((actiontype & B00001111) == 3)   // TOGGLE_INVERSE_TO_COUNTER
          level = ((keyEvent %2 == 0) ? 200 : 0);
        else   // TOGGLE
          level = 255;

#ifndef USE_HARDWARE_SERIAL
  hbwdebug(F("Tg to: "));
  hbwdebughex(level);
  hbwdebug(F("\n"));
#endif
//        this->set(device,2,&level);
        setOutput(&level);
        nextState = currentState; // avoid state machine to run
      }
    }
    else if (lastKeyEvent == keyEvent && !(bitRead(actiontype,5))) {
      // repeated key event, must be long press: LONG_MULTIEXECUTE not enabled
    }
    else {
      // assign values based on EEPROM layout
      onDelayTime = data[1];
      onTime = data[2];
      offDelayTime = data[3];
      offTime = data[4];
      jumpTargets.jt_hi_low[0] = data[5];
      jumpTargets.jt_hi_low[1] = data[6];

#ifndef USE_HARDWARE_SERIAL
  hbwdebug(F("onT: "));
  hbwdebughex(onTime);
  hbwdebug(F("\n"));
#endif

      nextState = FORCE_STATE_CHANGE; // force update
    }
    lastKeyEvent = keyEvent;  // store key press number, to identify repeated key events
  }
  else {  // set value - no peering event, overwrite any timer //TODO check: ok to ignore absolute on/off time running? how do original devices handle this?
    //if (!stateTimerRunning)??
    setOutput(data);
    stateTimerRunning = false;
    nextState = currentState; // avoid state machine to run
  }
};

// set value - no peering event, does not overwrite any timer //TODO check: keep on/off time running? how do original devices handle this?
//void HBWChanSw::set(HBWDevice* device, uint8_t length, uint8_t const * const data) {
void HBWChanSw::setOutput(uint8_t const * const data) {
  
  // TODO: handle peering set() call different? - e.g. do not allow on/off changes when timers are running? - or allow it, but don't trigger next jump
  // e.g. length > 1, nextState=currentState?
  
  if (config->output_unlocked) {  //0=LOCKED, 1=UNLOCKED
    byte level = *(data);

    if (level > 200) // toggle
      level = !shiftRegister->get(ledPos); // get current state and negate
    else if (level) {   // set to 0 or 1
      level = (LOW ^ config->n_inverted);
      shiftRegister->set(ledPos, HIGH); // set LEDs (register used for actual state!)
      currentState = JT_ON;   // update for state machine
    }
    else {
      level = (HIGH ^ config->n_inverted);
      shiftRegister->set(ledPos, LOW); // set LEDs (register used for actual state!)
      currentState = JT_OFF;   // update for state machine
    }
// TODO:  zero crossing function?. Just set portStatus[]? + add portStatusDesired[]?

    if (level) { // on - perform set
      shiftRegister->set(relayPos +1, LOW);    // reset coil
      shiftRegister->set(relayPos, HIGH);  // set coil
      
    }
    else {  // off - perform reset
      shiftRegister->set(relayPos, LOW);      // set coil
      shiftRegister->set(relayPos +1, HIGH);  // reset coil
      
    }
    
    relayOperationTimeStart = millis();  // Relay coils must be set two low after some ms (bistable Relays!!)
    operateRelay = true;

//    if (length == 1) {
//      stateTimerRunning = false;
//      nextState = currentState; // avoid state machine to run TODO; clear timer?
//    }
  }
  // Logging
  // (logging is considered for locked channels)
  if(!nextFeedbackDelay && config->logging) {
    lastFeedbackTime = millis();
    nextFeedbackDelay = device->getLoggingTime() * 100;
  }
};


uint8_t HBWChanSw::get(uint8_t* data) {
// read current state from shift register array
  //if (shiftRegister->get(ledPos) ^ config->n_inverted)
  if (shiftRegister->get(ledPos))
    (*data) = 200;
  else
    (*data) = 0;
  return 1;
};


void HBWChanSw::loop(HBWDevice* device, uint8_t channel) {
  
  unsigned long now = millis();

  /* important to remove power from latching relay after some milliseconds!! */
  if (((now - relayOperationTimeStart) >= RELAY_PULSE_DUARTION) && operateRelay == true) {
  // time to remove power from both coils?
    shiftRegister->setNoUpdate(relayPos +1, LOW);    // reset coil
    shiftRegister->setNoUpdate(relayPos, LOW);  // set coil
    shiftRegister->updateRegisters();
    
    operateRelay = false;
  }
  
//*** state machine begin ***//
  bool setNewLevel = false;

  if (((now - lastStateChangeTime > stateCangeWaitTime) && stateTimerRunning) || currentState != nextState) {

    if (currentState == nextState)  // no change to state, so must be time triggered
      stateTimerRunning = false;

#ifndef USE_HARDWARE_SERIAL
  hbwdebug(F("chan:"));
  hbwdebughex(channel);
  hbwdebug(F(" cs:"));
  hbwdebughex(currentState);
#endif
    
    // check next jump from current state
    switch (currentState) {
      case JT_ONDELAY:      // jump from on delay state
        nextState = getNextState(0);
        break;
      case JT_ON:       // jump from on state
        nextState = getNextState(3);
        break;
      case JT_OFFDELAY:    // jump from off delay state
        nextState = getNextState(6);
        break;
      case JT_OFF:      // jump from off state
        nextState = getNextState(9);
        break;
    }

#ifndef USE_HARDWARE_SERIAL
  hbwdebug(F(" ns:"));
  hbwdebughex(nextState);
  hbwdebug(F("\n"));
#endif

    uint8_t newLevel = 0;   // default value. Will only be set if setNewLevel was also set 'true'
    uint8_t currentLevel;
    get(&currentLevel);
 
    if (nextState != JT_NO_JUMP_IGNORE_COMMAND) {
      
      switch (nextState) {
        case JT_ONDELAY:
          stateCangeWaitTime = convertTime(onDelayTime);
          lastStateChangeTime = now;
          stateTimerRunning = true;
          currentState = JT_ONDELAY;
          break;
          
        case JT_ON:
          newLevel = 200;
          setNewLevel = true;
          stateTimerRunning = false;
          break;
          
        case JT_OFFDELAY:
          stateCangeWaitTime = convertTime(offDelayTime);
          lastStateChangeTime = now;
          stateTimerRunning = true;
          currentState = JT_OFFDELAY;
          break;
          
        case JT_OFF:
          //newLevel = 0; // 0 is default
          setNewLevel = true;
          stateTimerRunning = false;
          break;
          
        case ON_TIME_ABSOLUTE:
          newLevel = 200;
          setNewLevel = true;
          stateCangeWaitTime = convertTime(onTime);
          lastStateChangeTime = now;
          stateTimerRunning = true;
          nextState = JT_ON;
          break;
          
        case OFF_TIME_ABSOLUTE:
          //newLevel = 0; // 0 is default
          setNewLevel = true;
          stateCangeWaitTime = convertTime(offTime);
          lastStateChangeTime = now;
          stateTimerRunning = true;
          nextState = JT_OFF;
          break;
          
        case ON_TIME_MINIMAL:
          newLevel = 200;
          setNewLevel = true;
          if (now - lastStateChangeTime < convertTime(onTime)) {
            stateCangeWaitTime = convertTime(onTime);
            lastStateChangeTime = now;
            stateTimerRunning = true;
          }
          nextState = JT_ON;
          break;
          
        case OFF_TIME_MINIMAL:
          //newLevel = 0; // 0 is default
          setNewLevel = true;
          if (now - lastStateChangeTime < convertTime(offTime)) {
            stateCangeWaitTime = convertTime(offTime);
            lastStateChangeTime = now;
            stateTimerRunning = true;
          }
          nextState = JT_OFF;
          break;
      }
    }
    else {  // NO_JUMP_IGNORE_COMMAND
      currentState = (currentLevel ? JT_ON : JT_OFF );    // get current level and update state, TODO: actually needed? or keep for robustness?
      nextState = currentState;   // avoid to run into a loop
    }
    if (currentLevel != newLevel && setNewLevel) {   // check for current level. don't set same level again
//      this->set(device,2,&newLevel);
      setOutput(&newLevel);
      setNewLevel = false;
    }
  }
  //*** state machine end ***//
  
	
  if(!nextFeedbackDelay)  // feedback trigger set?
    return;
  now = millis(); // current timestamp needed!
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
};


// read jump target entry - set by peering (used for state machine)
uint8_t HBWChanSw::getNextState(uint8_t bitshift) {
  
  uint8_t nextJump = ((jumpTargets.WORD >>bitshift) & B00000111);
  
  if (nextJump == JT_ON) {
    if (onTime != 0xFF)      // not used is 0xFF
      nextJump = (actiontype & B10000000) ? ON_TIME_ABSOLUTE : ON_TIME_MINIMAL;  // on time ABSOLUTE or MINIMAL?
  }
  else if (nextJump == JT_OFF) {
    if (offTime != 0xFF)      // not used is 0xFF
      nextJump = (actiontype & B01000000) ? OFF_TIME_ABSOLUTE : OFF_TIME_MINIMAL;  // off time ABSOLUTE or MINIMAL?
  }
  
  if (stateTimerRunning && nextState == FORCE_STATE_CHANGE) { // timer still running, but update forced
    if (currentState == JT_ON)
      nextJump = (actiontype & B10000000) ? ON_TIME_ABSOLUTE : ON_TIME_MINIMAL;
    else if (currentState == JT_OFF)
      nextJump = (actiontype & B01000000) ? OFF_TIME_ABSOLUTE : OFF_TIME_MINIMAL;
  }
  return nextJump;
};


// convert time value stored in EEPROM to milliseconds (used for state machine)
uint32_t HBWChanSw::convertTime(uint8_t timeValue) {
  uint8_t factor = timeValue & 0xC0;    // mask out factor (higest two bits)
  timeValue &= 0x3F;    // keep time value only

  // factors: 1,60,1000,6000 (last one is not used)
  switch (factor) {
    case 0:         // x1
      return (uint32_t)timeValue *1000;
      break;
    case 64:         // x60
      return (uint32_t)timeValue *60000;
      break;
    case 128:        // x1000
      return (uint32_t)timeValue *1000000;
      break;
//    case 192:        // not used value
//      return 0; // TODO: check how to handle this properly, what does on/off time == 0 mean? always on/off??
//      break;
  }
  return 0;
};



void setup()
{
   // assing switches (relay) pins
   uint8_t LEDBitPos[6] = {0, 1, 2, 3, 4, 5};    // shift register 1: 6 LEDs // not only used for the LEDs, but also to keep track of the output state!
   uint8_t RelayBitPos[6] = {8, 10, 12,          // shift register 2: 3 relays (with 2 coils each)
                             16, 18, 20};        // shift register 3: 3 relays (with 2 coils each)
   uint8_t currentTransformerPins[6] = {CT_PIN1, CT_PIN2, CT_PIN3, CT_PIN4, CT_PIN5, CT_PIN6};
   
  // create channels
  for(uint8_t i = 0; i < NUM_SW_CHANNELS; i++){
    if (i < 6) {
      channels[i] = new HBWChanSw(RelayBitPos[i], LEDBitPos[i], &myShReg_one, &(hbwconfig.switchcfg[i]));
      channels[i+NUM_SW_CHANNELS] = new HBWAnalogIn(currentTransformerPins[i], &(hbwconfig.ctcfg[i]));
    }
    else
      channels[i] = new HBWChanSw(RelayBitPos[i %6], LEDBitPos[i %6], &myShReg_two, &(hbwconfig.switchcfg[i]));
  };

  #ifdef USE_HARDWARE_SERIAL  // RS485 via UART Serial, no debug (_debugstream is NULL)
    Serial.begin(19200, SERIAL_8E1);
    
    device = new HBSwDevice(HMW_DEVICETYPE, HARDWARE_VERSION, FIRMWARE_VERSION,
                           &Serial, RS485_TXEN, sizeof(hbwconfig), &hbwconfig,
                           NUM_CHANNELS,(HBWChannel**)channels,
                           NULL,
                           NULL, new HBWLinkSwitchAdvanced(NUM_LINKS,LINKADDRESSSTART));
    
    device->setConfigPins(BUTTON, LED);  // use analog input for 'BUTTON'
    
  #else
    Serial.begin(19200);
    rs485.begin();    // RS485 via SoftwareSerial
    
    device = new HBSwDevice(HMW_DEVICETYPE, HARDWARE_VERSION, FIRMWARE_VERSION,
                           &rs485, RS485_TXEN, sizeof(hbwconfig), &hbwconfig,
                           NUM_CHANNELS, (HBWChannel**)channels,
                           &Serial,
                           NULL, new HBWLinkSwitchAdvanced(NUM_LINKS,LINKADDRESSSTART));
     
    device->setConfigPins(BUTTON, LED);  // 8 (button) and 13 (led) is the default
   
    hbwdebug(F("B: 2A "));
    hbwdebug(freeRam());
    hbwdebug(F("\n"));
  #endif
}


void loop()
{
  device->loop();
};

