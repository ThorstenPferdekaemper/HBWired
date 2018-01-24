/* 
** HBWLinkBlindSimple
**
** Einfache direkte Verknuepfung (Peering), zu Rolladenaktor
** Ein Link-Objekt steht immer fuer alle (direkt aufeinander folgenden) Verknuepfungen
** des entsprechenden Typs.
**
*/

#ifndef HBWLinkBlindSimple_h
#define HBWLinkBlindSimple_h

#include "HBWired.h"

class HBWLinkBlindSimple : public HBWLinkReceiver {
  public:
      HBWLinkBlindSimple(uint8_t _numLinks, uint16_t _eepromStart);
	  void receiveKeyEvent(HBWDevice* device, uint32_t senderAddress, uint8_t senderChannel, 
                           uint8_t targetChannel, boolean longPress);					   
  private:
      uint8_t numLinks;         // number of links of this type  
      uint16_t eepromStart;     //size sollte konstant sein -> als define in .cpp
};

#endif
