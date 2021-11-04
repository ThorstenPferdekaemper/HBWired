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


template<uint8_t numLinks, uint16_t eepromStart>
class HBWLinkBlindSimple : public HBWLinkReceiver {
  public:
      HBWLinkBlindSimple();
	  void receiveKeyEvent(HBWDevice* device, uint32_t senderAddress, uint8_t senderChannel, 
                           uint8_t targetChannel, uint8_t keyPressNum, boolean longPress);					   
  private:
      uint32_t lastSenderAddress;
      uint8_t lastSenderChannel;

      static const uint8_t EEPROM_SIZE = 9;   // "address_step" in XML
      static const uint8_t NUM_PEER_PARAMS = 1;   // number of bytes for long and short peering action each (without address and channel)
};

#include "HBWLinkBlindSimple.hpp"
#endif
