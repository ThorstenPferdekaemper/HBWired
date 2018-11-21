/* 
** HBWLinkDimmerAdvanced
**
** Direkte Verknuepfung (Peering), zu Schaltausgaengen
** Ein Link-Objekt steht immer fuer alle (direkt aufeinander folgenden) Verknuepfungen
** des entsprechenden Typs.
**
** http://loetmeister.de/Elektronik/homematic/
*/

#ifndef HBWLinkDimmerAdvanced_h
#define HBWLinkDimmerAdvanced_h

#include "HBWired.h"

class HBWLinkDimmerAdvanced : public HBWLinkReceiver {
  public:
    HBWLinkDimmerAdvanced(uint8_t _numLinks, uint16_t _eepromStart);
	  void receiveKeyEvent(HBWDevice* device, uint32_t senderAddress, uint8_t senderChannel, 
                           uint8_t targetChannel, uint8_t keyPressNum, boolean longPress);				

  private:
      uint8_t numLinks;         // number of links of this type  
      uint16_t eepromStart;     //size sollte konstant sein -> als define in .cpp
};

#endif
