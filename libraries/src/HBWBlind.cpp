/* 
** HBWBlind
**
** Rolladenaktor mit Richtungs und Aktiv Relais pro Kanal
** 
** Infos: http://loetmeister.de/Elektronik/homematic/index.htm#modules
** Vorlage: https://github.com/loetmeister/HM485-Lib/tree/markus/HBW-LC-Bl4
**
** Last updated: 08.02.2019
*/

#include "HBWBlind.h"


// constructor
HBWChanBl::HBWChanBl(uint8_t _blindDir, uint8_t _blindAct, hbw_config_blind* _config) {
  blindDir = _blindDir;
  blindAct = _blindAct;
  config = _config;  
  clearFeedback();
  pinMode(blindAct,OUTPUT);
  digitalWrite(blindAct,OFF);
  pinMode(blindDir,OUTPUT);
  digitalWrite(blindDir,OFF);
  blindNextState = RELAIS_OFF;
  blindCurrentState = RELAIS_OFF;
  blindForceNextState = false;
  blindPositionKnown = false;
  blindPositionActual = 0;
  blindAngleActual = 0;
  blindDirection = UP;
  blindSearchingForRefPosition = false;
  blindPositionRequested = 0;
  lastKeyNum = 0;
};

// channel specific settings or defaults
void HBWChanBl::afterReadConfig() {
    if (config->blindTimeTopBottom == 0xFFFF) config->blindTimeTopBottom = 200;
    if (config->blindTimeBottomTop == 0xFFFF) config->blindTimeBottomTop = 200;
    if (config->blindTimeChangeOver == 0xFF) config->blindTimeChangeOver = 20;
}

void HBWChanBl::set(HBWDevice* device, uint8_t length, uint8_t const * const data) {
  
  if (lastKeyNum == data[1] && length == 2) return;  // irgnore same keyNumber, surpress repeated long press (peering)
  
  // blind control
  if((*data) == 0xFF) { // toggle
  #ifdef DEBUG
    hbwdebug(F("Toggle\n"));
  #endif
    if (blindCurrentState == TURN_AROUND)
      blindNextState = STOP;

    blindForceNextState = true;

    if ((blindCurrentState == STOP) || (blindCurrentState == RELAIS_OFF)) {
      if (blindDirection == UP) blindPositionRequested = 100;
      else blindPositionRequested = 0;

      blindNextState = WAIT;

      // if current blind position is not known (e.g. due to a reset), actual position is set to the limit, to ensure that the moving time is long enough to reach the end position
      if (!blindPositionKnown) {
      #ifdef DEBUG
        hbwdebug(F("Position unknown\n"));
      #endif
        if (blindDirection == UP)
          blindPositionActual = 0;
        else
          blindPositionActual = 100;
      }
    }
  }
  else if ((*data) == 0xC9) { // stop
  #ifdef DEBUG
    hbwdebug(F("Stop\n"));
  #endif
    blindNextState = STOP;
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
      blindPositionActual = 100;
      blindPositionRequestedSave = blindPositionRequested;
      blindPositionRequested = 0;
      blindSearchingForRefPosition = true;
      }
    }

    if ((blindCurrentState == TURN_AROUND) || (blindCurrentState == MOVE)) { // aktuelle Position ist nicht bekannt

      getCurrentPosition();

      if (((blindDirection == UP) && (blindPositionRequested > blindPositionActual)) || ((blindDirection == DOWN) && (blindPositionRequested < blindPositionActual))) {
        // Rollo muss die Richtung umkehren:
        blindNextState = SWITCH_DIRECTION;
        blindForceNextState = true;
      }
      else {
        // Rollo fährt schon in die richtige Richtung
        if (blindCurrentState == MOVE) { // Zeit muss neu berechnet werden // current state == TURN_AROUND, nicht zu tun, Zeit wird ohnehin beim ersten Aufruf von MOVE berechnet
          blindNextState = MOVE; // Im Zustand MOVE wird die neue Zeit berechnet
          blindForceNextState = true;
        }
      }
    }
    else { // aktueller State != MOVE und != TURN_AROUND, d.h. Rollo steht gerade
      // set next state only if a new target value is requested
      if (blindPositionRequested != blindPositionActual) {
        blindNextState = WAIT;
        blindForceNextState = true;
      }
    }
  }
  lastKeyNum = data[1];  // store keyNum for next call
};


uint8_t HBWChanBl::get(uint8_t* data) {

  uint8_t newData;
  uint8_t stateFlag = 0;  // working "off", direction "none"
  
  if (blindNextState == STOP) {     // wenn Rollo gestopppt wird und keine Referenzfahrt läuft,
    getCurrentPosition();
    newData = blindPositionActual;  // dann aktuelle Position ausgeben,
  }
  else {                           // ansonsten die angeforderte Zielposition
    if (blindSearchingForRefPosition)
      newData = blindPositionRequestedSave;
    else
      newData = blindPositionRequested;
  }
  
  if (blindCurrentState > RELAIS_OFF || blindNextState > RELAIS_OFF) {  // set "direction" flag (turns "working" to "on")
    if (blindPositionActual < blindPositionRequested)
      stateFlag |= B00010000;
    else
      stateFlag |= B00100000;
  }
  
  (*data++) = newData *2;
  *data = stateFlag;
  return 2;
};


void HBWChanBl::loop(HBWDevice* device, uint8_t channel) {
  
  //handle blinds
  
  uint8_t data;
  unsigned long now = millis();

  if ((blindForceNextState == true) || (now - blindTimeLastAction >= blindNextStateDelayTime)) {

    switch(blindNextState) {
      case RELAIS_OFF:
        // Switch off the "direction" relay
        digitalWrite(blindDir, OFF);
        blindCurrentState = RELAIS_OFF;
        blindTimeLastAction = now;
        blindNextStateDelayTime = 20000;  // time is increased to avoid cyclic call
        break;


      case WAIT:
        if (blindPositionRequested > blindPositionActual)  // blind needs to move down
          blindDirection = DOWN;
        else  // blind needs to move up
          blindDirection = UP;

        // switch on the "direction" relay
        digitalWrite(blindDir, blindDirection);

        // Set next state & delay time
        blindCurrentState = WAIT;
        blindNextState = TURN_AROUND;
        blindForceNextState = false;
        blindTimeLastAction = now;
        blindNextStateDelayTime = BLIND_WAIT_TIME;
        break;


      case MOVE:
        // switch on the "active" relay
        digitalWrite(blindAct, ON);
        blindTimeStart = now;
        blindTimeLastAction = now;
        blindPositionLast = blindPositionActual;

        // Set next state & delay time
        blindCurrentState = MOVE;
        blindNextState = STOP;
        if (blindDirection == UP) {
          blindNextStateDelayTime = (blindPositionActual - blindPositionRequested) * config->blindTimeBottomTop;
        }
        else {
          blindNextStateDelayTime = (blindPositionRequested - blindPositionActual) * config->blindTimeTopBottom;
        }

        // add offset time if final positions are requested to ensure that final position is really reached
        if ((blindPositionRequested == 0) || (blindPositionRequested == 100))
          blindNextStateDelayTime += BLIND_OFFSET_TIME;

        blindForceNextState = false;
        break;


      case STOP:
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

        // prepare to send info message with current position
        if(!blindSearchingForRefPosition)  // Logging only for final state (STOP state), don't send when searching ref. pos.
          setFeedback(device, config->logging);

        // switch off the "active" relay
        digitalWrite(blindAct, OFF);

        // Set next state & delay time
        blindCurrentState = STOP;
        blindNextState = RELAIS_OFF;
        blindTimeLastAction = now;
        blindNextStateDelayTime = BLIND_WAIT_TIME;

        if (blindSearchingForRefPosition == true) {
        #ifdef DEBUG
          hbwdebug(F("Reference position reached. Moving to target position.\n"));
        #endif
          data = (blindPositionRequestedSave * 2);
          device->set(channel, 1, &data);
          blindSearchingForRefPosition = false;
        }
        break;


      case TURN_AROUND:
        // switch on the "active" relay
        digitalWrite(blindAct, ON);
        blindTimeStart = now;
        blindTimeLastAction = now;
        blindCurrentState = TURN_AROUND;
        blindNextState = MOVE;
        if (blindDirection == UP)  // Zulässige Laufzeit ohne die Position zu ändern (erlaubt Stellwinkel von Lamellen zu ändern)
          blindNextStateDelayTime = blindAngleActual * config->blindTimeChangeOver;
        else
          blindNextStateDelayTime = (100 - blindAngleActual) * config->blindTimeChangeOver;
        break;
        

      case SWITCH_DIRECTION:
        // switch off the "active" relay
        digitalWrite(blindAct, OFF);
        blindTimeLastAction = now;

        // Set current state, next state & delay time
        blindCurrentState = SWITCH_DIRECTION;
        blindNextState = WAIT;
        blindForceNextState = false;
        blindNextStateDelayTime = BLIND_WAIT_TIME *4;
        break;
      }
    }
  
  // handle feedback (sendInfoMessage)
  checkFeedback(device, channel);
}


void HBWChanBl::getCurrentPosition() {
  unsigned long now = millis();
  
  if (blindCurrentState == MOVE) {
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
  else {  // => current state == TURN_AROUND
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
}

