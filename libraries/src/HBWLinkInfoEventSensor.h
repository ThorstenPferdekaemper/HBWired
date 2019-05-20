/* 
** HBWLinkInfoEventSensor
**
** Einfache direkte Verknuepfung (Peering), vom Sensor ausgehend
** Ein Link-Objekt steht immer fuer alle (direkt aufeinander folgenden) Verknuepfungen
** des entsprechenden Typs.
**
*/


#ifndef HBWLinkInfoEventSensor_h
#define HBWLinkInfoEventSensor_h

#include "HBWired.h"

class HBWLinkInfoEventSensor : public HBWLinkSender {
  public:
      HBWLinkInfoEventSensor(uint8_t _numLinks, uint16_t _eepromStart);
      void sendInfoEvent(HBWDevice* device, uint8_t srcChan, uint8_t length, uint8_t const * const data);
  private:
      uint8_t numLinks;         // number of links of this type  
      uint16_t eepromStart;     //size sollte konstant sein -> als define in .cpp
};

#endif
