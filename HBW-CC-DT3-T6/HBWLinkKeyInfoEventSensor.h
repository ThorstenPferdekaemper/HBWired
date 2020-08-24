/* 
** HBWLinkKeyInfoEventSensor
**
** Einfache direkte Verknuepfung (Peering), vom Sensor ausgehend
** Ein Link-Objekt steht immer fuer alle (direkt aufeinander folgenden) Verknuepfungen
** des entsprechenden Typs.
**
*/

// TODO: remove these files and add option to the lib, allowing to combine different LinkSender

#ifndef HBWLinkKeyInfoEventSensor_h
#define HBWLinkKeyInfoEventSensor_h

#include "HBWired.h"


template<uint8_t numLinks, uint16_t eepromStart>
class HBWLinkKeyInfoEventSensor : public HBWLinkSender {
  public:
      HBWLinkKeyInfoEventSensor();
      void sendInfoEvent(HBWDevice* device, uint8_t srcChan, uint8_t length, uint8_t const * const data);
	  void sendKeyEvent(HBWDevice* device, uint8_t srcChan, uint8_t keyPressNum, boolean longPress);
  private:
      static const uint8_t EEPROM_SIZE = 6;   // "address_step" in XML
};

#include "HBWLinkKeyInfoEventSensor.hpp"
#endif
