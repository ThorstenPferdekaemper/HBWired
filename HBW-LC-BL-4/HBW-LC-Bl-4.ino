//*******************************************************************
//
// HBW-LC-Bl-4
//
// Homematic Wired Hombrew Hardware
// Arduino NANO als Homematic-Device
// 4 Kanal Rollosteuerung
// - 

// TODO: Add direct peering (HBWLink...) & update XML
//
//*******************************************************************


#define HARDWARE_VERSION 0x01
#define FIRMWARE_VERSION 0x0100

#define NUMBER_OF_BLINDS 4
//#define NUM_LINKS 36
//#define LINKADDRESSSTART 0x40

#define HMW_DEVICETYPE 0x82 //BL4 device (make sure to import hbw_lc_bl4.xml into FHEM)


#include "HBWSoftwareSerial.h"
#include "FreeRam.h"    


// HB Wired protocol and module
#include "HBWired.h"


#define RS485_RXD 4
#define RS485_TXD 2
#define RS485_TXEN 3  // Transmit-Enable

// HBWSoftwareSerial can only do 19200 baud
HBWSoftwareSerial rs485(RS485_RXD, RS485_TXD); // RX, TX


// Pins
#define BUTTON 8  // Button fuer Factory-Reset etc.
#define LED LED_BUILTIN        // Signal-LED

#define BLIND1_ACT 5		// "Ein-/Aus-Relais"
#define BLIND1_DIR 6		// "Richungs-Relais"
#define BLIND2_ACT A0
#define BLIND2_DIR A1
#define BLIND3_ACT A2
#define BLIND3_DIR A3
#define BLIND4_ACT A4
#define BLIND4_DIR A5


/********************************/
/* Config der Rollo-Steuerung:  */
/********************************/

#define ON LOW					// Möglichkeit für invertierte Logik
#define OFF HIGH
#define UP LOW					// "Richtungs-Relais"
#define DOWN HIGH
#define BLIND_WAIT_TIME 100		// Wartezeit [ms] zwischen Ansteuerung des "Richtungs-Relais" und des "Ein-/Aus-Relais"
#define BLIND_OFFSET_TIME 1000	// Zeit [ms], die beim Anfahren der Endlagen auf die berechnete Zeit addiert wird, um die Endlagen wirklich sicher zu erreichen



/* here comes the code... */
#define STOP 0
#define WAIT 1
#define TURN_AROUND 2
#define MOVE 3
#define RELAIS_OFF 4
#define SWITCH_DIRECTION 5


struct hbw_config_blind {
	byte logging:1;    			  		    // 0x07:0   0x0E	0x15	0x1C
	byte        :7;   			            // 0x07:1-7	0x0E
	unsigned char blindTimeChangeOver;      // 0x08		0x0F	0x16	0x1D
	unsigned char blindReferenceRunCounter; // 0x09		0x10	0x17	0x1E
	unsigned int blindTimeBottomTop;   		// 0x0A 	0x11	0x18	0x1F
	unsigned int blindTimeTopBottom;  		// 0x0C;  0x13  0x1A  0x21
};

struct hbw_config {
  uint8_t logging_time;     // 0x01
  uint32_t central_address;  // 0x02 - 0x05
  uint8_t direct_link_deactivate:1;   // 0x06:0
  uint8_t              :7;   // 0x06:1-7
  hbw_config_blind blindscfg[NUMBER_OF_BLINDS]; // 0x07-0x... ? (step 7)
} hbwconfig;



// new class for blinds
class HBWChanBl : public HBWChannel {
  public:
    HBWChanBl(uint8_t _blindDir, uint8_t _blindAct, hbw_config_blind* _config);
    virtual uint8_t get(uint8_t* data);   
    virtual void loop(HBWDevice*, uint8_t channel);
    virtual void set(HBWDevice*, uint8_t length, uint8_t const * const data);
    void getCurrentPosition();
    unsigned long now;
  private:
    uint8_t blindDir;
    uint8_t blindAct;
    hbw_config_blind* config; // logging
    uint32_t lastFeedbackTime;  // when did we send the last feedback?
    uint16_t nextFeedbackDelay; // 0 -> no feedback pending

    byte blindNextState;
    byte blindCurrentState;
    byte blindPositionRequested;
    byte blindPositionRequestedSave;
    byte blindPositionActual;
    byte blindPositionLast;
    byte blindAngleActual;
    byte blindDirection;
    byte blindForceNextState;
    bool blindPositionKnown;
    bool blindSearchingForRefPosition;
    unsigned long blindTimeNextState;
    unsigned long blindTimeStart;
};


HBWChanBl* blinds[NUMBER_OF_BLINDS];

class HBBlDevice : public HBWDevice {
    public: 
    HBBlDevice(uint8_t _devicetype, uint8_t _hardware_version, uint16_t _firmware_version,
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
        for(uint8_t channel = 0; channel < NUMBER_OF_BLINDS; channel++) {
                    
          if (hbwconfig.blindscfg[channel].blindTimeTopBottom == 0xFFFF) hbwconfig.blindscfg[channel].blindTimeTopBottom = 200;
          if (hbwconfig.blindscfg[channel].blindTimeBottomTop == 0xFFFF) hbwconfig.blindscfg[channel].blindTimeBottomTop = 200;
          if (hbwconfig.blindscfg[channel].blindTimeChangeOver == 0xFF) hbwconfig.blindscfg[channel].blindTimeChangeOver = 20;
        };
    };
};


//HBSenDevice* device = NULL;
HBBlDevice* device = NULL;


HBWChanBl::HBWChanBl(uint8_t _blindDir, uint8_t _blindAct, hbw_config_blind* _config) {
  blindDir = _blindDir;
  blindAct = _blindAct;
  config = _config;  
  nextFeedbackDelay = 0;
  lastFeedbackTime = 0;
  
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
};




void HBWChanBl::set(HBWDevice* device, uint8_t length, uint8_t const * const data) {
  
  // blind control
	if((*data) == 0xFF) { // toggle

		hbwdebug("Toggle\n");
		if (blindCurrentState == TURN_AROUND)
			blindNextState = STOP;

		// if (blindCurrentState[channel] == MOVE)  // muss nicht extra erwähnt werden, nextState ist ohnehin STOP
		//   blindNextState[channel] = STOP;

		blindForceNextState = true;

		if ((blindCurrentState == STOP) || (blindCurrentState == RELAIS_OFF)) {
			if (blindDirection == UP) blindPositionRequested = 100;
			else blindPositionRequested = 0;

			blindNextState = WAIT;

			// if current blind position is not known (e.g. due to a reset), actual position is set to the limit, to ensure that the moving time is long enough to reach the end position
			if (!blindPositionKnown) {
				hbwdebug("Position unknown\n");
				if (blindDirection == UP)
					blindPositionActual = 0;
				else
					blindPositionActual = 100;
			}
		}
	}
	else if ((*data) == 0xC9) { // stop
		hbwdebug("Stop\n");
		blindNextState = STOP;
		blindForceNextState = true;
	}
	else { // set level
	  
	  blindPositionRequested = (*data) / 2;

		if (blindPositionRequested > 100)
		  blindPositionRequested = 100;
		hbwdebug("Requested Position: "); hbwdebug(blindPositionRequested); hbwdebug("\n");

    if (!blindPositionKnown) {
      hbwdebug("Position unknown. Moving to reference position.\n");

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
  
  // Logging
  if(!nextFeedbackDelay && config->logging) {
    lastFeedbackTime = millis();
    nextFeedbackDelay = device->getLoggingTime() * 100;
  }
};


uint8_t HBWChanBl::get(uint8_t* data) {
  
  if (blindNextState == STOP) {      // wenn Rollo gestopppt wird,
    getCurrentPosition();
    (*data) = blindPositionActual *2;    // dann aktuelle Position ausgeben,
   }
   else {
    (*data) = blindPositionRequested *2; // ansonsten die angeforderte Zielposition
   }
  return 1;
};


void HBWChanBl::loop(HBWDevice* device, uint8_t channel) {
  //handle blinds
  
  uint8_t data;
  now = millis();

  if ((blindForceNextState == true) || (now >= blindTimeNextState)) {

    switch(blindNextState) {
      case RELAIS_OFF:
        // Switch off the "direction" relay
        digitalWrite(blindDir, OFF);

        // debug message
//        debugStateChange(blindNextState, channel);

        blindCurrentState = RELAIS_OFF;
        blindTimeNextState = now + 20000;  // time is increased to avoid cyclic call
        break;


      case WAIT:
        if (blindPositionRequested > blindPositionActual)  // blind needs to move down
          blindDirection = DOWN;
        else  // blind needs to move up
          blindDirection = UP;

        // switch on the "direction" relay
        digitalWrite(blindDir, blindDirection);

        // debug message
//        debugStateChange(blindNextState, channel);

        // Set next state & delay time
        blindCurrentState = WAIT;
        blindNextState = TURN_AROUND;
        blindForceNextState = false;
        blindTimeNextState = now + BLIND_WAIT_TIME;
        break;


      case MOVE:
        // switch on the "active" relay
        digitalWrite(blindAct, ON);

        // debug message
//        debugStateChange(blindNextState, channel);

        blindTimeStart = now;
        blindPositionLast = blindPositionActual;

        // Set next state & delay time
        blindCurrentState = MOVE;
        blindNextState = STOP;
        if (blindDirection == UP) {
//          blindTimeNextState = now + (blindPositionActual - blindPositionRequested) * blindTimeBottomTop;
          blindTimeNextState = now + (blindPositionActual - blindPositionRequested) * config->blindTimeBottomTop;
        }
        else {
//          blindTimeNextState = now + (blindPositionRequested - blindPositionActual) * blindTimeTopBottom;
          blindTimeNextState = now + (blindPositionRequested - blindPositionActual) * config->blindTimeTopBottom;
        }

        // add offset time if final positions are requested to ensure that final position is really reached
        if ((blindPositionRequested == 0) || (blindPositionRequested == 100))
          blindTimeNextState += BLIND_OFFSET_TIME;

        if (blindForceNextState == true)
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
            hbwdebug("Reference position reached. Position is known.\n");
            blindPositionKnown = true;
          }
        }

        // send info message with current position
        //hmwmodule->sendInfoMessage(channel, blindPositionActual[channel], 0xFFFFFFFF);
        //device->sendInfoMessage(channel, 512 * blindPositionActual, 0xFFFFFFFF);
        data = (blindPositionActual * 2);
        device->sendInfoMessage(channel, 1, &data);   //TODO: add retry (e.g. replace by logging process)


        // switch off the "active" relay
        digitalWrite(blindAct, OFF);

        // debug message
//        debugStateChange(blindNextState, channel);

        // Set next state & delay time
        blindCurrentState = STOP;
        blindNextState = RELAIS_OFF;
        blindTimeNextState = now + BLIND_WAIT_TIME;


        if (blindSearchingForRefPosition == true) {
          hbwdebug("Reference position reached. Moving to target position.\n");
          //hmwdevice.setLevel(channel, blindPositionRequestedSave[channel] * 2);
          data = (blindPositionRequestedSave * 2);
          device->set(channel, 1, &data);
          blindSearchingForRefPosition = false;
        }
        break;


      case TURN_AROUND:
        // switch on the "active" relay
        digitalWrite(blindAct, ON);
        blindTimeStart = now;

        // debug message
//        debugStateChange(blindNextState, channel);

        blindCurrentState = TURN_AROUND;
        blindNextState = MOVE;
        if (blindDirection == UP)
          blindTimeNextState = now + blindAngleActual * config->blindTimeChangeOver;
        else
          blindTimeNextState = now + (100 - blindAngleActual) * config->blindTimeChangeOver;
        break;
        

      case SWITCH_DIRECTION:

        // switch off the "active" relay
        digitalWrite(blindAct, OFF);

        // debug message
//        debugStateChange(blindNextState, channel);

        // Set current state, next state & delay time
        blindCurrentState = SWITCH_DIRECTION;
        blindNextState = WAIT;
        blindForceNextState = false;
        blindTimeNextState = now + BLIND_WAIT_TIME;
        break;
      }
    }
  
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
    }
    else {
      nextFeedbackDelay = 0;
    }
}


void HBWChanBl::getCurrentPosition() {
  
  if (blindCurrentState == MOVE) {
    if (blindDirection == UP) {
//      blindPositionActual = blindPositionLast - (now - blindTimeStart) / blindTimeBottomTop;
      blindPositionActual = blindPositionLast - (now - blindTimeStart) / config->blindTimeBottomTop;
      if (blindPositionActual > 100)
        blindPositionActual = 0; // robustness
      blindAngleActual = 0;
    }
    else {
//      blindPositionActual = blindPositionLast + (now - blindTimeStart) / blindTimeTopBottom;
      blindPositionActual = blindPositionLast + (now - blindTimeStart) / config->blindTimeTopBottom;
      if (blindPositionActual > 100)
        blindPositionActual = 100; // robustness
      blindAngleActual = 100;
    }
  }
  else {	// => current state == TURN_AROUND
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


//void debugStateChange(byte state, byte channel) {
//  hbwdebug("State: ");
//  switch(state) {
//    case RELAIS_OFF: hbwdebug("RELAY_OFF"); break;
//    case WAIT: hbwdebug("WAIT"); break;
//    case MOVE: hbwdebug("MOVE"); break;
//    case STOP: hbwdebug("STOP"); break;
//    case TURN_AROUND: hbwdebug("TURN_AROUND"); break;
//    case SWITCH_DIRECTION: hbwdebug("SWITCH_DIRECTION"); break;
//  }
//  hbwdebug(" - Time: ");
//  hbwdebug(now);
//  hbwdebug(" - Target: ");
//  hbwdebug(blindPositionRequested[channel]);
//  hbwdebug(" - Actual: ");
//  hbwdebug(blindPositionActual[channel]);
//  hbwdebug("\n");
//}



void setup()
{
  pinMode(BUTTON, INPUT_PULLUP);
  pinMode(LED, OUTPUT);

  Serial.begin(19200);
  rs485.begin();    // RS485 via SoftwareSerial

  // create channels
  byte blindDir[4] = {BLIND1_DIR, BLIND2_DIR, BLIND3_DIR, BLIND4_DIR};
  byte blindAct[4] = {BLIND1_ACT, BLIND2_ACT, BLIND3_ACT, BLIND4_ACT};
  // assing relay pins
  for(uint8_t i = 0; i < NUMBER_OF_BLINDS; i++){
    blinds[i] = new HBWChanBl(blindDir[i], blindAct[i], &(hbwconfig.blindscfg[i]));
   };

  device = new HBBlDevice(HMW_DEVICETYPE, HARDWARE_VERSION, FIRMWARE_VERSION,
                         &rs485,RS485_TXEN,sizeof(hbwconfig),&hbwconfig,
                         NUMBER_OF_BLINDS,(HBWChannel**)blinds,
                         &Serial,
                         NULL, NULL);
   
  device->setConfigPins();  // 8 and 13 is the default
 
  hbwdebug(F("B: 2A "));
  hbwdebug(freeRam());
  hbwdebug(F("\n"));

//hbwdebug("TimeTopBottom: "); hbwdebug(hbwconfig.blindscfg[2].blindTimeTopBottom, DEC); hbwdebug("\n");
//hbwdebug("TimeBottomTop: "); hbwdebug(hbwconfig.blindscfg[2].blindTimeBottomTop, DEC); hbwdebug("\n");
//hbwdebug("TimeTurnAround: "); hbwdebug(hbwconfig.blindscfg[2].blindTimeChangeOver, DEC); hbwdebug("\n");
}


void loop()
{
  device->loop();
};

