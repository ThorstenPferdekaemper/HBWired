/* 
** HBWLinkKey
**
** Einfache direkte Verknuepfung (Peering), vom Tastereingang ausgehend
** Ein Link-Objekt steht immer fuer alle (direkt aufeinander folgenden) Verknuepfungen
** des entsprechenden Typs.
**
*/


#ifndef HBWLinkInfoMessageSensor_h
#define HBWLinkInfoMessageSensor_h

#include "HBWired.h"

class HBWLinkInfoMessageSensor : public HBWLinkSender {
  public:
      HBWLinkInfoMessageSensor(uint8_t _numLinks, uint16_t _eepromStart);
      void sendIMEvent(HBWDevice* device, uint8_t srcChan, uint8_t length, uint8_t const * const data);
      void sendKeyEvent(HBWDevice* device, uint8_t srcChan, uint8_t keyPressNum, boolean longPress);
  private:
      uint8_t numLinks;         // number of links of this type  
      uint16_t eepromStart;     //size sollte konstant sein -> als define in .cpp
};

#endif
