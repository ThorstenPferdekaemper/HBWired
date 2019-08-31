/* 
** HBWLinkKey
**
** Einfache direkte Verknuepfung (Peering), vom Tastereingang ausgehend
** Ein Link-Objekt steht immer fuer alle (direkt aufeinander folgenden) Verknuepfungen
** des entsprechenden Typs.
**
*/


#ifndef HBWLinkKey_h
#define HBWLinkKey_h

#include "HBWired.h"

class HBWLinkKey : public HBWLinkSender {
  public:
      HBWLinkKey(uint8_t _numLinks, uint16_t _eepromStart);
	  void sendKeyEvent(HBWDevice* device, uint8_t srcChan, uint8_t keyPressNum, boolean longPress, boolean enqueue = true);
  private:
      uint8_t numLinks;         // number of links of this type  
      uint16_t eepromStart;     //size sollte konstant sein -> als define in .cpp
};

#endif
