/* 
** HBWLinkSwitch
**
** Direkte Verknuepfung (Peering), zu Schaltausgaengen
** Ein Link-Objekt steht immer fuer alle (direkt aufeinander folgenden) Verknuepfungen
** des entsprechenden Typs.
**
*/

#ifndef HBWLinkSwitch_h
#define HBWLinkSwitch_h

#include "HBWired.h"

class HBWLinkSwitch : public HBWLinkReceiver {
  public:
      HBWLinkSwitch(uint8_t _numLinks, uint16_t _eepromStart);
	  void receiveKeyEvent(HBWDevice* device, uint32_t senderAddress, uint8_t senderChannel, 
                           uint8_t targetChannel, boolean longPress);					   
  private:
      uint8_t numLinks;         // number of links of this type  
      uint16_t eepromStart;     //size sollte konstant sein -> als define in .cpp
};

#endif
