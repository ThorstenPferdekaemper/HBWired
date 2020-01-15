/* 
** HBWLinkInfoEventActuator
**
** Einfache direkte Verknuepfung (Peering), zu Schaltausgaengen
** Ein Link-Objekt steht immer fuer alle (direkt aufeinander folgenden) Verknuepfungen
** des entsprechenden Typs.
**
*/

#ifndef HBWLinkInfoEventActuator_h
#define HBWLinkInfoEventActuator_h

#include "HBWired.h"


template<uint8_t numLinks, uint16_t eepromStart>
class HBWLinkInfoEventActuator : public HBWLinkReceiver {
  public:
      HBWLinkInfoEventActuator();
      void receiveInfoEvent(HBWDevice* device, uint32_t senderAddress, uint8_t senderChannel, 
                           uint8_t targetChannel, uint8_t length, uint8_t const * const data);
  private:
      static const uint8_t EEPROM_SIZE = 7;   // "address_step" in XML
};

#include "HBWLinkInfoEventActuator.hpp"
#endif
