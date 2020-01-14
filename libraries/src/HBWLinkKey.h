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


template<uint8_t numLinks, uint16_t eepromStart>
class HBWLinkKey : public HBWLinkSender {
  public:
      HBWLinkKey();
	  void sendKeyEvent(HBWDevice* device, uint8_t srcChan, uint8_t keyPressNum, boolean longPress);
  private:
      static const uint8_t EEPROM_SIZE = 6;   // "address_step" in XML
};

#include "HBWLinkKey.hpp"
#endif
