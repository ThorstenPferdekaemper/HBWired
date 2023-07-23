/* 
** HBWBlind
**
** Rolladenaktor mit Richtungs und Aktiv Relais pro Kanal
** 
** Infos: http://loetmeister.de/Elektronik/homematic/index.htm#modules
** Vorlage: https://github.com/loetmeister/HM485-Lib/tree/markus/HBW-LC-Bl4
**
** Last updated: 09.07.2023
*/

#include "HBWBlind.h"
#include "HBWLinkBlindSimple.h"


// constructor
HBWChanBl::HBWChanBl(uint8_t _blindDir, uint8_t _blindAct, hbw_config_blind* _config) {
  blindDir = _blindDir;
  blindAct = _blindAct;
  config = _config;  
  clearFeedback();
  digitalWrite(blindAct, OFF);
  digitalWrite(blindDir, OFF);
  pinMode(blindAct, OUTPUT);
  pinMode(blindDir, OUTPUT);
  blindNextState = BL_STATE_RELAIS_OFF;
  blindCurrentState = BL_STATE_RELAIS_OFF;
  blindForceNextState = false;
  blindPositionKnown = false;
  blindPositionActual = 0;
  blindAngleActual = 0;
  blindDirection = UP;
  blindSearchingForRefPosition = false;
  blindPositionRequested = 0;
  blindRunCounter = 0;
  lastKeyNum = 255;
};

// channel specific settings or defaults
void HBWChanBl::afterReadConfig() {
    if (config->blindTimeTopBottom == 0xFFFF) config->blindTimeTopBottom = 500;
    if (config->blindTimeBottomTop == 0xFFFF) config->blindTimeBottomTop = 500;
    if (config->blindTimeChangeOver == 0xFF) config->blindTimeChangeOver = 5;
    if (config->blindMotorDelay == 0x7F) config->blindMotorDelay = 0;
}

/* standard public function - set a channel, directly or via peering event. Data array contains new value or all peering details */
void HBWChanBl::set(HBWDevice* device, uint8_t length, uint8_t const * const data) {
  
  if (length == 3)
  {
    uint8_t currentKeyNum = data[1];
    bool sameLastSender = data[2];

    if (lastKeyNum == currentKeyNum && sameLastSender)  // ignore repeated key press from the same sender
      return;
    else
      lastKeyNum = currentKeyNum;
  }
  
  // blind control
  if((*data) == SET_BLIND_TOGGLE) {
  #ifdef DEBUG
    hbwdebug(F("Toggle\n"));
  #endif
    if (blindCurrentState == BL_STATE_TURN_AROUND)
      blindNextState = BL_STATE_STOP;

    blindForceNextState = true;

    if ((blindCurrentState == BL_STATE_STOP) || (blindCurrentState == BL_STATE_RELAIS_OFF)) {
      
      blindPositionRequested = (blindDirection == UP) ? 100 : 0;
      blindNextState = BL_STATE_WAIT;

      // if current blind position is not known (e.g. due to a reset), actual position is set to the limit, to ensure that the moving time is long enough to reach the end position
      if (!blindPositionKnown) {
      #ifdef DEBUG
        hbwdebug(F("Position unknown\n"));
      #endif

        blindPositionActual = (blindDirection == UP) ? 0 : 100;
      }
    }
  }
  else if ((*data) == SET_BLIND_STOP) {
  #ifdef DEBUG
    hbwdebug(F("Stop\n"));
  #endif
    blindNextState = BL_STATE_STOP;
    blindForceNextState = true;
  }
  else { // set level
  
    blindPositionRequested = (*data) / 2;

    if (blindPositionRequested > 100)
      blindPositionRequested = 100;
  #ifdef DEBUG
    hbwdebug(F("Requested Position: ")); hbwdebug(blindPositionRequested); hbwdebug(F("\n"));
  #endif

    if (!blindPositionKnown) {
    #ifdef DEBUG
      hbwdebug(F("Position unknown. Moving to reference position.\n"));
    #endif

      if (blindPositionRequested == 0) {
      blindPositionActual = 100;
      }
      else if (blindPositionRequested == 100) {
      blindPositionActual = 0;
      }
      else {  // Target position >0 and <100
      // for requested position 50% or lower, set target to 0% to move in right direction - set blindPositionActual (unknown anyway) as opposite (always 0 or 100)
      blindPositionActual = blindPositionRequested <= 50 ? 100 : 0;
      blindPositionRequestedSave = blindPositionRequested;
      blindPositionRequested = blindPositionRequested <= 50 ? 0 : 100;
      blindSearchingForRefPosition = true;
      }
    }

    if ((blindCurrentState == BL_STATE_TURN_AROUND) || (blindCurrentState == BL_STATE_MOVE)) { // aktuelle Position ist nicht bekannt

      getCurrentPosition();

      if (((blindDirection == UP) && (blindPositionRequested > blindPositionActual)) || ((blindDirection == DOWN) && (blindPositionRequested < blindPositionActual))) {
        // Rollo muss die Richtung umkehren:
        blindNextState = BL_STATE_SWITCH_DIRECTION;
        blindForceNextState = true;
      }
      else {
        // Rollo fährt schon in die richtige Richtung
        if (blindCurrentState == BL_STATE_MOVE) { // Zeit muss neu berechnet werden // current state == BL_STATE_TURN_AROUND, nicht zu tun, Zeit wird ohnehin beim ersten Aufruf von BL_STATE_MOVE berechnet
          blindNextState = BL_STATE_MOVE; // Im Zustand BL_STATE_MOVE wird die neue Zeit berechnet
          blindForceNextState = true;
        }
      }
    }
    else { // aktueller State != BL_STATE_MOVE und != BL_STATE_TURN_AROUND, d.h. Rollo steht gerade
      // set next state only if a new target value is requested
      if (blindPositionRequested != blindPositionActual) {
        blindNextState = BL_STATE_WAIT;
        blindForceNextState = true;
      }
    }
  }
};


uint8_t HBWChanBl::get(uint8_t* data)
{
  uint8_t newData;
  uint8_t stateFlag = 0;  // working "off", direction "none"
  
  if (blindNextState == BL_STATE_STOP) {     // wenn Rollo gestopppt wird und keine Referenzfahrt läuft,
    getCurrentPosition();
    newData = blindPositionActual;  // dann aktuelle Position ausgeben,
  }
  else {                           // ansonsten die angeforderte Zielposition
    if (blindSearchingForRefPosition)
      newData = blindPositionRequestedSave;
    else
      newData = blindPositionRequested;
  }
  
  if (blindCurrentState > BL_STATE_RELAIS_OFF || blindNextState > BL_STATE_RELAIS_OFF) {  // set "direction" flag (turns "working" to "on")
    if (blindPositionActual < blindPositionRequested)
      stateFlag |= B00010000;
    else
      stateFlag |= B00100000;
  }
  
  (*data++) = newData *2;
  *data = stateFlag;
  return 2;
};


/* standard public function - called by device main loop for every channel in sequential order */
void HBWChanBl::loop(HBWDevice* device, uint8_t channel)
{
  unsigned long now = millis();

  if ((blindForceNextState == true) || (now - blindTimeLastAction >= blindNextStateDelayTime)) {

    switch(blindNextState) {
      case BL_STATE_RELAIS_OFF:
        digitalWrite(blindAct, OFF);   // switch off the "active" relay (should be OFF already)
        digitalWrite(blindDir, OFF);   // Switch off the "direction" relay
        blindCurrentState = BL_STATE_RELAIS_OFF;
        blindTimeLastAction = now;
        blindNextStateDelayTime = 20000;  // time is increased to avoid cyclic call
        break;

      case BL_STATE_WAIT:
        blindDirection = (blindPositionRequested > blindPositionActual) ? DOWN : UP;
        digitalWrite(blindDir, blindDirection);   // switch on the "direction" relay

        // Set next state & delay time
        blindCurrentState = BL_STATE_WAIT;
        blindNextState = BL_STATE_TURN_AROUND;
        blindForceNextState = false;
        blindTimeLastAction = now;
        blindNextStateDelayTime = BLIND_WAIT_TIME;
        break;

      case BL_STATE_MOVE:
        digitalWrite(blindAct, ON);   // switch on the "active" relay
        blindTimeStart = now;
        blindTimeLastAction = now;
        blindPositionLast = blindPositionActual;

        // Set next state & delay time
        blindCurrentState = BL_STATE_MOVE;
        blindNextState = BL_STATE_STOP;
        if (blindDirection == UP) {
          blindNextStateDelayTime = (blindPositionActual - blindPositionRequested) * config->blindTimeBottomTop;
        }
        else {
          blindNextStateDelayTime = (blindPositionRequested - blindPositionActual) * config->blindTimeTopBottom;
        }
        blindNextStateDelayTime += config->blindMotorDelay *10;   // Motor lief nicht, eingestellte Anlaufzeit addieren
		blindRunCounter++;

        // add offset time if final positions are requested to ensure that final position is really reached
        if ((blindPositionRequested == 0) || (blindPositionRequested == 100)) {
          blindNextStateDelayTime += BLIND_OFFSET_TIME;
        }

        blindForceNextState = false;
        break;

      case BL_STATE_STOP:
        // calculate current position if next state was forced by toggle/stop
        if (blindForceNextState == true) {
          blindForceNextState = false;
          getCurrentPosition();
          blindPositionRequested = blindPositionActual;
        }
        // or take over requested position if blind has moved for the desired time
        else {
          blindPositionActual = blindPositionRequested;
          
          if (blindDirection == UP)
            blindAngleActual = 0;
          else
            blindAngleActual = 100;

          // if current position was not known (e.g. due to a reset) and end position are reached, then the current position is known again
          if ((!blindPositionKnown) && ((blindPositionRequested == 0) || (blindPositionRequested == 100))) {
          #ifdef DEBUG
            hbwdebug(F("Reference position reached. Position is known.\n"));
          #endif
            blindPositionKnown = true;
          }
        }

        if ((blindPositionActual == 0) || (blindPositionActual == 100)) {
          blindRunCounter = 0;  // stopped at min/max position, clear run counter
        }

        // prepare to send info message with current position
        if(!blindSearchingForRefPosition)  // Logging only for final state (BL_STATE_STOP state), don't send when searching ref. pos.
          setFeedback(device, config->logging);

        digitalWrite(blindAct, OFF);   // switch off the "active" relay

        // Set next state & delay time
        blindCurrentState = BL_STATE_STOP;
        blindNextState = BL_STATE_RELAIS_OFF;
        blindTimeLastAction = now;
        blindNextStateDelayTime = BLIND_WAIT_TIME;

        if (blindSearchingForRefPosition == true) {
        #ifdef DEBUG
          hbwdebug(F("Reference position reached. Moving to target position.\n"));
        #endif
          uint8_t data = (blindPositionRequestedSave * 2);
          device->set(channel, 1, &data);
          blindSearchingForRefPosition = false;
          blindRunCounter = 0;
        }
        if (blindRunCounter >= config->blindReferenceRunCounter && config->blindReferenceRunCounter != 0) {
          blindPositionKnown = false;  // next level set will trigger drive to reference position
        }
        break;

      case BL_STATE_TURN_AROUND:
        digitalWrite(blindAct, ON);   // switch on the "active" relay
        blindTimeStart = now;
        blindTimeLastAction = now;
        blindCurrentState = BL_STATE_TURN_AROUND;
        blindNextState = BL_STATE_MOVE;
        if (blindDirection == UP) {  // Zulässige Laufzeit ohne die Position zu ändern (erlaubt Stellwinkel von Lamellen zu ändern)
          blindNextStateDelayTime = blindAngleActual * config->blindTimeChangeOver;
        }
        else {
          blindNextStateDelayTime = (100 - blindAngleActual) * config->blindTimeChangeOver;
        }
        break;

      case BL_STATE_SWITCH_DIRECTION:
        digitalWrite(blindAct, OFF);   // switch off the "active" relay
        blindTimeLastAction = now;

        // Set current state, next state & delay time
        blindCurrentState = BL_STATE_SWITCH_DIRECTION;
        blindNextState = BL_STATE_WAIT;
        blindForceNextState = false;
        blindNextStateDelayTime = BLIND_WAIT_TIME *8;
        break;
    }
  }
  
  // handle feedback (sendInfoMessage)
  checkFeedback(device, channel);
};


void HBWChanBl::getCurrentPosition()
{
  unsigned long now = millis() + config->blindMotorDelay *10;  // motor start time does not count for motion...
  
  if (blindCurrentState == BL_STATE_MOVE) {
    if (blindDirection == UP) {
      blindPositionActual = blindPositionLast - (now - blindTimeStart) / config->blindTimeBottomTop;
      if (blindPositionActual > 100)
        blindPositionActual = 0; // robustness
      blindAngleActual = 0;
    }
    else {
      blindPositionActual = blindPositionLast + (now - blindTimeStart) / config->blindTimeTopBottom;
      if (blindPositionActual > 100)
        blindPositionActual = 100; // robustness
      blindAngleActual = 100;
    }
  }
  else {  // => current state == BL_STATE_TURN_AROUND
    // blindPosition unchanged
    if (blindDirection == UP) {
      blindAngleActual -= (now - blindTimeStart) / config->blindTimeChangeOver;
      if (blindAngleActual > 100)
        blindAngleActual = 0; // robustness
    }
    else {
      blindAngleActual += (now - blindTimeStart) / config->blindTimeChangeOver;
      if (blindAngleActual > 100)
        blindAngleActual = 100; // robustness
    }
  }
};

