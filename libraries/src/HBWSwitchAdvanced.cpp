/* 
* HBWSwitchAdvanced.cpp
*
* Als Alternative zu HBWSwitch & HBWLinkSwitchSimple sind mit
* HBWSwitchAdvanced & HBWLinkSwitchAdvanced folgende Funktionen möglich:
* Peering mit TOGGLE, TOGGLE_TO_COUNTER, TOGGLE_INVERSE_TO_COUNTER, onTime,
* offTime (Ein-/Ausschaltdauer), onDelayTime, offDelayTime (Ein-/Ausschaltverzögerung).
*
* http://loetmeister.de/Elektronik/homematic/
*
*/

#include "HBWSwitchAdvanced.h"

// Switches class
// HBWSwitchAdvanced::HBWSwitchAdvanced(uint8_t _pin, hbw_config_switch* _config) : stateParamList (NULL) {
HBWSwitchAdvanced::HBWSwitchAdvanced(uint8_t _pin, hbw_config_switch* _config) {
  pin = _pin;
  config = _config;
  clearFeedback();
  init();
};


// channel specific settings or defaults
void HBWSwitchAdvanced::afterReadConfig()
{
  if (pin == NOT_A_PIN)  return;
  
  if (currentState == UNKNOWN_STATE) {
  // All off on init, but consider inverted setting
    digitalWrite(pin, config->n_inverted ? LOW : HIGH);    // 0=inverted, 1=not inverted (device reset will set to 1!)
    pinMode(pin, OUTPUT);
    currentState = JT_OFF;
  }
  else {
  // Do not reset outputs on config change (EEPROM re-reads), but update its state
    if (currentState == JT_ON)                          // TODO add || JT_OFFDELAY ?
      digitalWrite(pin, LOW ^ config->n_inverted);
    else if (currentState == JT_OFF)                    // TODO add || JT_ONDELAY ?
      digitalWrite(pin, HIGH ^ config->n_inverted);
  // TODO: check off dealy state - right now the output will not be updated...
  }
};


/* standard public function - set a channel, directly or via peering event. Data array contains new value or all peering details */
void HBWSwitchAdvanced::set(HBWDevice* device, uint8_t length, uint8_t const * const data)
{
  hbwdebug(F("set: "));
  if (length == 9) {  // got called with additional peering parameters -- test for correct NUM_PEER_PARAMS
    
    s_peering_list* peeringList;
    peeringList = (s_peering_list*)data;
    uint8_t currentKeyNum = data[7]; // TODO: add to struct? jtPeeringList? .. seem to use more prog memory...
    bool sameLastSender = data[8];
    
    // s_jt_peering_list* jtPeeringList;
    // jtPeeringList = (s_jt_peering_list*)&data[5];

    // if ((StateMachine.lastKeyNum == currentKeyNum && sameLastSender) && !peeringList->LongMultiexecute)
    if ((lastKeyNum == currentKeyNum && sameLastSender) && !peeringList->LongMultiexecute)
    // if ((lastKeyNum == jtPeeringList->currentKeyNum && jtPeeringList->sameLastSender) && !peeringList->LongMultiexecute)
      return;  // repeated long press reveived, but LONG_MULTIEXECUTE not enabled
    // TODO: check behaviour for LONG_MULTIEXECUTE & TOGGLE... (long toggle?)
    
	// hbwdebug(F("\nonDelayTime "));hbwdebug(peeringList->onDelayTime);hbwdebug(F("\n"));
	// hbwdebug(F("onTime "));hbwdebug(peeringList->onTime);hbwdebug(F("\n"));
	// hbwdebug(F("onTimeMin "));hbwdebug(peeringList->isOnTimeMinimal());hbwdebug(F("\n"));
	// hbwdebug(F("offDelayTime "));hbwdebug(peeringList->offDelayTime);hbwdebug(F("\n"));
	// hbwdebug(F("offTime "));hbwdebug(peeringList->offTime);hbwdebug(F("\n"));
	// hbwdebug(F("offTimeMin "));hbwdebug(peeringList->isOffTimeMinimal());hbwdebug(F("\n"));
	// // hbwdebug(F("jtOnDelay "));hbwdebug(peeringList->jtOnDelay);hbwdebug(F("\n"));
	// // hbwdebug(F("jtOn "));hbwdebug(peeringList->jtOn);hbwdebug(F("\n"));
	// // hbwdebug(F("jtOffDelay "));hbwdebug(peeringList->jtOffDelay);hbwdebug(F("\n"));
	// // hbwdebug(F("jtOff "));hbwdebug(peeringList->jtOff);hbwdebug(F("\n"));
	// hbwdebug(F("jtOnDelay "));hbwdebug(jtPeeringList->jtOnDelay);hbwdebug(F("\n"));
	// hbwdebug(F("jtOn "));hbwdebug(jtPeeringList->jtOn);hbwdebug(F("\n"));
	// hbwdebug(F("jtOffDelay "));hbwdebug(jtPeeringList->jtOffDelay);hbwdebug(F("\n"));
	// hbwdebug(F("jtOff "));hbwdebug(jtPeeringList->jtOff);hbwdebug(F("\n"));
    
    if (peeringList->actionType == JUMP_TO_TARGET) {
      hbwdebug(F("jumpToTarget\n"));
      s_jt_peering_list* jtPeeringList;
      jtPeeringList = (s_jt_peering_list*)&data[5];
      jumpToTarget(device, peeringList, jtPeeringList);
    }
    else if (peeringList->actionType >= TOGGLE_TO_COUNTER && peeringList->actionType <= TOGGLE)
    {
      uint8_t nextState;
      switch (peeringList->actionType) {
      // case JUMP_TO_TARGET:
        // hbwdebug(F("jumpToTarget\n"));
        // jumpToTarget(device, peeringList, jtPeeringList);
        // break;
      case TOGGLE_TO_COUNTER:
        hbwdebug(F("TOGGLE_TO_C\n"));  // switch OFF at odd numbers, ON at even numbers
        // HMW-LC-Sw2 behaviour: toggle actions also use delay and on/off time
        // setState(device, (currentKeyNum & 0x01) == 0x01 ? JT_ON : JT_OFF, DELAY_INFINITE);
        nextState = (currentKeyNum & 0x01) == 0x01 ? JT_ON : JT_OFF;
        break;
      case TOGGLE_INVERS_TO_COUNTER:
      hbwdebug(F("TOGGLE_INV_TO_C\n"));  // switch ON at odd numbers, OFF at even numbers
        // setState(device, (currentKeyNum & 0x01) == 0x00 ? JT_ON : JT_OFF, DELAY_INFINITE);
        nextState = (currentKeyNum & 0x01) == 0x00 ? JT_ON : JT_OFF;
        break;
      case TOGGLE: {
        hbwdebug(F("TOGGLE\n"));
        // setState(device, currentState == JT_ON ? JT_OFF : JT_ON, DELAY_INFINITE);
        // uint8_t reading[2];
        // get(reading);
        // nextState = (reading[0] == 0) ? JT_ON : JT_OFF;  // check actual output state. 0 is OFF, anything other ON
        // set OFF in onDelay and ON in offDelay state?
        nextState = (currentState == JT_OFF || currentState == JT_ONDELAY) ? JT_ON : JT_OFF;  // go to ON or OFF
		}
        break;
      default: break;
      }
      uint32_t delay = getDelayForState(nextState, peeringList);  // get on/off time also for toggle actions
      setState(device, nextState, delay, peeringList);
    }
    
    lastKeyNum = currentKeyNum;  // store key press number, to identify repeated key events
  }
  else {
    // set value - no peering event, overwrite any active timer. Peering actions can overwrite INFINITE delay by absolute time_mode only
    hbwdebug(F("value\n"));
    if (*(data) > 200) {   // toggle
      // setState(device, (currentState == JT_ON ? JT_OFF : JT_ON), DELAY_INFINITE);
      // uint8_t reading[2];
      // get(reading);
      // setState(device, (reading[0] == 0 ? JT_ON : JT_OFF), DELAY_INFINITE);;  // check actual output state. 0 is OFF, anything other ON
      setState(device, ((currentState == JT_OFF || currentState == JT_ONDELAY) ? JT_ON : JT_OFF), DELAY_INFINITE);
    }
    else {
      setState(device, *(data) == 0 ? JT_OFF : JT_ON, DELAY_INFINITE);  // set on or off
    }
  }
};


/* standard public function - returns length of data array. Data array contains current channel reading */
uint8_t HBWSwitchAdvanced::get(uint8_t* data)
{
  (*data++) = (digitalRead(pin) ^ config->n_inverted) ? 0 : 200;
  *data = stateTimerRunning ? 64 : 0;  // state flag 'working'
  
  return 2;
};


// separate function to set the actual outputs. Would be usually different in derived class
bool HBWSwitchAdvanced::setOutput(HBWDevice* device, uint8_t newstate)
{
  bool result = false;
  
  if (config->output_unlocked)  //0=LOCKED, 1=UNLOCKED
  {
    if (newstate == JT_ON) { // TODO: JT_OFFDELAY would turn output ON, if not yet ON
      digitalWrite(pin, LOW ^ config->n_inverted);
    }
    else if (newstate == JT_OFF) { // TODO: JT_ONDELAY would turn output OFF, if not yet OFF
      digitalWrite(pin, HIGH ^ config->n_inverted);
    }
    result = true;  // return success for unlocked channels, to accept any new state
  }
  // Logging
  // setFeedback(device, config->logging);
  return result;
};


void HBWSwitchAdvanced::loop(HBWDevice* device, uint8_t channel)
{
  unsigned long now = millis();

 //*** state machine begin ***//
  if (((now - lastStateChangeTime > stateChangeWaitTime) && stateTimerRunning))
  {
    stopStateChangeTime();  // time was up, so don't come back into here - new valid timer should be set below
    
   #ifdef DEBUG_OUTPUT
     hbwdebug(F("chan:"));
     hbwdebughex(channel);
   #endif
    // check next jump from current state
    uint8_t nextState = getNextState();
    uint32_t delay = getDelayForState(nextState, stateParamList);

    // try to set new state and start new timer, if needed
    setState(device, nextState, delay, stateParamList);
  }
  //*** state machine end ***//
  
  // feedback trigger set?
  checkFeedback(device, channel);
};

