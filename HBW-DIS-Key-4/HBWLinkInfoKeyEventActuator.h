/* 
** HBWLinkInfoKeyEventActuator
**
** Einfache direkte Verknuepfung (Peering), zu Schaltausgaengen
** Ein Link-Objekt steht immer fuer alle (direkt aufeinander folgenden) Verknuepfungen
** des entsprechenden Typs.
**
*/

// TODO: put this back into the normal HBWLinkReceiver - find a good way to combine different classes (without wasting space...)

#ifndef HBWLinkInfoKeyEventActuator_h
#define HBWLinkInfoKeyEventActuator_h

#include "HBWired.h"


template<uint8_t numLinks, uint16_t eepromStart>
class HBWLinkInfoKeyEventActuator : public HBWLinkReceiver {
  public:
      HBWLinkInfoKeyEventActuator();
      void receiveInfoEvent(HBWDevice* device, uint32_t senderAddress, uint8_t senderChannel, 
                           uint8_t targetChannel, uint8_t length, uint8_t const * const data);
      void receiveKeyEvent(HBWDevice* device, uint32_t senderAddress, uint8_t senderChannel, 
                           uint8_t targetChannel, uint8_t keyPressNum, boolean longPress);
  private:
      static const uint8_t EEPROM_SIZE = 7;   // "address_step" in XML
};

#include "HBWLinkInfoKeyEventActuator.hpp"
#endif
