/* 
** HBWBlind
**
** Rolladenaktor mit Richtungs und Aktiv Relais pro Kanal
** 
** 
**
*/

#ifndef HBWBlind_h
#define HBWBlind_h

#include "HBWired.h"


/********************************/
/* Config der Rollo-Steuerung:  */
/********************************/

#define ON LOW					// Möglichkeit für invertierte Logik
#define OFF HIGH
#define UP LOW					// "Richtungs-Relais"
#define DOWN HIGH
#define BLIND_WAIT_TIME 100		// Wartezeit [ms] zwischen Ansteuerung des "Richtungs-Relais" und des "Ein-/Aus-Relais"
#define BLIND_OFFSET_TIME 1000	// Zeit [ms], die beim Anfahren der Endlagen auf die berechnete Zeit addiert wird, um die Endlagen wirklich sicher zu erreichen
#define BLIND_SWITCH_DIRECTION_WAIT_TIME 800	// + BLIND_WAIT_TIME; Zeit die beim direkten Richtungswechsel gewartet wird, in der der Motor zum Stillstand kommt
                                              //TODO: Add to device or channel config?


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
	unsigned int blindTimeTopBottom;  		// 0x0C   0x13  0x1A  0x21
};


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
    unsigned int blindNextStateDelayTime;
    unsigned long blindTimeStart;
    unsigned long blindTimeLastAction;
};

#endif
