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


template<uint8_t numLinks, uint16_t eepromStart>
class HBWLinkDimmerAdvanced : public HBWLinkReceiver {
  public:
    HBWLinkDimmerAdvanced();
	  void receiveKeyEvent(HBWDevice* device, uint32_t senderAddress, uint8_t senderChannel, 
                           uint8_t targetChannel, uint8_t keyPressNum, boolean longPress);				

  private:
      static const uint8_t EEPROM_SIZE = 42;   // "address_step" in XML
      static const uint8_t NUM_PEER_PARAMS = 18;   // number of bytes for long and short peering action each (without address and channel)
};

#include "HBWLinkDimmerAdvanced.hpp"
#endif
