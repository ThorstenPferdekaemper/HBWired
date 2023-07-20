/* 
** HBWBlind
**
** Rolladenaktor mit Richtungs und Aktiv Relais pro Kanal
** 
** Infos: http://loetmeister.de/Elektronik/homematic/index.htm#modules
** Vorlage: https://github.com/loetmeister/HM485-Lib/tree/markus/HBW-LC-Bl4
**
*/

#ifndef HBWBlind_h
#define HBWBlind_h

#include "HBWired.h"

//#define DEBUG  // enable per channel class

/********************************/
/* Config der Rollo-Steuerung:  */
/********************************/

#define ON HIGH					// Möglichkeit für invertierte Logik
#define OFF LOW
#define UP HIGH					// "Richtungs-Relais"
#define DOWN LOW
#define BLIND_WAIT_TIME 100		// Wartezeit [ms] zwischen Ansteuerung des "Richtungs-Relais" und des "Ein-/Aus-Relais" (Bei Richtungswechsel 8-fache Wartezeit)
#define BLIND_OFFSET_TIME 1000	// Zeit [ms], die beim Anfahren der Endlagen auf die berechnete Zeit addiert wird, um die Endlagen wirklich sicher zu erreichen



/* here comes the code... */
#define BL_STATE_STOP 0
#define BL_STATE_RELAIS_OFF 1
#define BL_STATE_WAIT 2
#define BL_STATE_TURN_AROUND 3
#define BL_STATE_MOVE 4
#define BL_STATE_SWITCH_DIRECTION 5


// config of blind channel, address step 7
struct hbw_config_blind {
  byte logging:1;                         // 0x07:0    0x0E  0x15  0x1C
  // byte        :7;                         // 0x07:1-7  0x0E
  byte blindMotorDelay:7;                 // Anlaufzeit (Verzögerung bis zur Bewegung, nachdem der Motor Spannung erhalten hat)
  unsigned char blindTimeChangeOver;      // 0x08  0x0F  0x16  0x1D - Zulässige Laufzeit ohne die Position zu ändern (erlaubt Stellwinkel von Lamellen zu ändern)
  unsigned char blindReferenceRunCounter; // 0x09  0x10  0x17  0x1E
  unsigned int blindTimeBottomTop;        // 0x0A  0x11  0x18  0x1F
  unsigned int blindTimeTopBottom;        // 0x0C  0x13  0x1A  0x21
};


// new class for blinds
class HBWChanBl : public HBWChannel {
  public:
    HBWChanBl(uint8_t _blindDir, uint8_t _blindAct, hbw_config_blind* _config);
    virtual uint8_t get(uint8_t* data);   
    virtual void loop(HBWDevice*, uint8_t channel);
    virtual void set(HBWDevice*, uint8_t length, uint8_t const * const data);
    virtual void afterReadConfig();
	
  private:
    void getCurrentPosition();
    uint8_t blindDir;   // direction relay
    uint8_t blindAct;   // activity relay
    hbw_config_blind* config;
   
    byte blindNextState;
    byte blindCurrentState;
    byte blindPositionRequested;
    byte blindPositionRequestedSave;
    byte blindPositionActual;
    byte blindPositionLast;
    byte blindAngleActual;
    byte blindRunCounter;
    bool blindDirection;
    bool blindForceNextState;
    bool blindPositionKnown;
    bool blindSearchingForRefPosition;
    unsigned int blindNextStateDelayTime;
    unsigned long blindTimeStart;
    unsigned long blindTimeLastAction;
    
    uint8_t lastKeyNum;
};

#endif

