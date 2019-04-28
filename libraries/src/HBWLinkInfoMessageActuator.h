/* 
** HBWLinkInfoMessageActuator
**
** Einfache direkte Verknuepfung (Peering), zu Schaltausgaengen
** Ein Link-Objekt steht immer fuer alle (direkt aufeinander folgenden) Verknuepfungen
** des entsprechenden Typs.
**
*/

#ifndef HBWLinkInfoMessageActuator_h
#define HBWLinkInfoMessageActuator_h

#include "HBWired.h"

class HBWLinkInfoMessageActuator : public HBWLinkReceiver {
  public:
      HBWLinkInfoMessageActuator(uint8_t _numLinks, uint16_t _eepromStart);
      void receiveInfoEvent(HBWDevice* device, uint32_t senderAddress, uint8_t senderChannel, 
                           uint8_t targetChannel, uint8_t length, uint8_t const * const data);
  private:
      uint8_t numLinks;         // number of links of this type  
      uint16_t eepromStart;     //size sollte konstant sein -> als define in .cpp
};

#endif
