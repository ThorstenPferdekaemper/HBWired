/*
 * HBWSwitchSerialAdvanced.cpp
 * 
 * see "HBWSwitchSerialAdvanced.h" for details
 * 
 * Updated: 29.12.2024
 *  
 * http://loetmeister.de/Elektronik/homematic/index.htm#modules
 * 
 */

#include "HBWSwitchSerialAdvanced.h"


// Class HBWSwitchSerialAdvanced, derived from HBWSwitchAdvanced
// TODO: remove unused code from base class, like private setOutput() which is not used at all.. add another class? HBW(lib)SwitchAdvanced -> HBWSwitch[Serial]Advanced?
HBWSwitchSerialAdvanced::HBWSwitchSerialAdvanced(uint8_t _relayPos, uint8_t _ledPos, SHIFT_REGISTER_CLASS* _shiftRegister, hbw_config_switch* _config)
  : HBWSwitchAdvanced(NOT_A_PIN, _config)
  {
  relayPos = _relayPos;
  ledPos = _ledPos;
  // config = _config;// - taken from parent class
  shiftRegister = _shiftRegister;
  relayOperationPending = false;
  relayLevel = OFF;
  
  // init(); - HBWSwitchAdvanced initializer called it already
  currentState = JT_OFF;  // init() will set state to UNKNOWN_STATE - we don't want that here
  // clearFeedback();
};


// channel specific settings or defaults
// (This function is called after device read config from EEPROM)
void HBWSwitchSerialAdvanced::afterReadConfig()
{
  // perform intial reset (or refresh) for all relays - bistable relays may have random state from previous power loss!
  // relay state (relayLevel) is OFF on device reset, but maintained on EEPROM re-read
  operateRelay(relayLevel ? (LOW ^ config->n_inverted) : (HIGH ^ config->n_inverted));
}

/* standard public function - set a channel, directly or via peering event. Data array contains new value or all peering details */
// using parent void HBWSwitchAdvanced::set()...


/* standard public function - returns length of data array. Data array contains current channel reading */
uint8_t HBWSwitchSerialAdvanced::get(uint8_t* data)
{
  (*data++) = relayLevel == ON ? 200 : 0;
  *data = stateTimerRunning ? 64 : 0;  // state flag 'working'
  
  return 2;
};


// set actual outputs
bool HBWSwitchSerialAdvanced::setOutput(HBWDevice* device, uint8_t newstate)
{
  bool result = false;

  if (config->output_unlocked && relayOperationPending == false)  // not LOCKED and no relay operation pending
  {
    if (newstate == JT_ON) {hbwdebug(F("!ON!"));
      // level = (LOW ^ config->n_inverted);
      shiftRegister->set(ledPos, HIGH); // set LEDs
      operateRelay(LOW ^ config->n_inverted);
    }
    else if (newstate == JT_OFF) {hbwdebug(F("!OFF!"));
      // level = (HIGH ^ config->n_inverted);
      shiftRegister->set(ledPos, LOW); // set LEDs
      operateRelay(HIGH ^ config->n_inverted);
    }
    // hbwdebug(F("!b!"));hbwdebug(relayOperationPending);hbwdebug(F("!"));
    result = true;  // return success for unlocked channels, to accept any new state
  }
  // Logging
  // setFeedback(device, config->logging);
  return result;
};


void HBWSwitchSerialAdvanced::operateRelay(bool _newLevel)
{
  /* set new state, only if no relay operation is pending */
  if (_newLevel) { // on - perform set
    shiftRegister->set(relayPos +1, LOW);    // reset coil (remove power first - for safety)
    shiftRegister->set(relayPos, HIGH);      // power set coil
    relayLevel = ON;
  }
  else {  // off - perform reset
    shiftRegister->set(relayPos, LOW);      // set coil (remove power first - for safety)
    shiftRegister->set(relayPos +1, HIGH);  // power reset coil
    relayLevel = OFF;
  }
  
  relayOperationTimeStart = millis();  // relay coil power must be removed after some ms (bistable relays!!)
  relayOperationPending = true;
// hbwdebug(F("re-S!"));
};


void HBWSwitchSerialAdvanced::loop(HBWDevice* device, uint8_t channel)
{
  unsigned long now = millis();
  
  /* important to remove power from latching relay after some milliseconds!! */
  if ((now - relayOperationTimeStart) > RELAY_PULSE_DUARTION && relayOperationPending)
  {
  // time to remove power from all coils // TODO: implement as timer interrupt routine? as it's important function... or rely on watchdog reset on major failure?
    shiftRegister->setNoUpdate(relayPos +1, LOW);    // reset coil
    shiftRegister->setNoUpdate(relayPos, LOW);       // set coil
    shiftRegister->updateRegisters();
    relayOperationPending = false;
// hbwdebug(F("re-C!"));
  }

  HBWSwitchAdvanced::loop(device, channel);  // parent function handles the rest...

  //*** state machine begin ***//
  // if (((now - lastStateChangeTime > stateChangeWaitTime) && stateTimerRunning))
  // {
  //   stopStateChangeTime();  // time was up, so don't come back into here - new valid timer should be set below
  //  #ifdef DEBUG_OUTPUT
  //    hbwdebug(F("chan:"));
  //    hbwdebughex(channel);
  //  #endif
  //   // check next jump from current state
  //   uint8_t nextState = getNextState();
  //   uint32_t delay = getDelayForState(nextState, stateParamList);

  //   // try to set new state and start new timer, if needed
  //   setState(device, nextState, delay, stateParamList);
  // }
  // //*** state machine end ***//

  // // feedback trigger set?
  // checkFeedback(device, channel);
};
