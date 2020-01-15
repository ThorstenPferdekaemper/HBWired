/* 
** HBWLinkSwitchSimple
**
** Einfache direkte Verknuepfung (Peering), zu Schaltausgaengen
** Ein Link-Objekt steht immer fuer alle (direkt aufeinander folgenden) Verknuepfungen
** des entsprechenden Typs.
**
*/

#ifndef HBWLinkSwitchSimple_h
#define HBWLinkSwitchSimple_h

#include "HBWired.h"


template<uint8_t numLinks, uint16_t eepromStart>
class HBWLinkSwitchSimple : public HBWLinkReceiver {
  public:
      HBWLinkSwitchSimple();
	  void receiveKeyEvent(HBWDevice* device, uint32_t senderAddress, uint8_t senderChannel, 
                           uint8_t targetChannel, uint8_t keyPressNum, boolean longPress);
  private:
      static const uint8_t EEPROM_SIZE = 7;   // "address_step" in XML
};

#include "HBWLinkSwitchSimple.hpp"
#endif
