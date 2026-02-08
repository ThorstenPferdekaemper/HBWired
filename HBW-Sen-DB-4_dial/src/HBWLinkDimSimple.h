/* 
** HBWLinkDimSimple
**
** Super simple dimmer peering. Using same address_step (7) as HBWLinkSwitchSimple
** - to allow to use them in the same device
**
** Einfache direkte Verknuepfung (Peering), zu Schaltausgaengen
** Ein Link-Objekt steht immer fuer alle (direkt aufeinander folgenden) Verknuepfungen
** des entsprechenden Typs.
**
*/

#ifndef HBWLinkDimSimple_h
#define HBWLinkDimSimple_h

#include "HBWired.h"


template<uint8_t numLinks, uint16_t eepromStart>
class HBWLinkDimSimple : public HBWLinkReceiver {
  public:
      HBWLinkDimSimple();
	  void receiveKeyEvent(HBWDevice* device, uint32_t senderAddress, uint8_t senderChannel, 
                           uint8_t targetChannel, uint8_t keyPressNum, boolean longPress);
  private:
      uint32_t lastSenderAddress;
      uint8_t lastSenderChannel;

      static const uint8_t EEPROM_SIZE = 7;   // "address_step" in XML
};

#include "HBWLinkDimSimple.hpp"
#endif
