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


template<uint8_t numLinks, uint16_t eepromStart>
class HBWLinkInfoEventSensor : public HBWLinkSender {
  public:
      HBWLinkInfoEventSensor();
      void sendInfoEvent(HBWDevice* device, uint8_t srcChan, uint8_t length, uint8_t const * const data);
  private:
      static const uint8_t EEPROM_SIZE = 6;   // "address_step" in XML
};

#include "HBWLinkInfoEventSensor.hpp"
#endif
