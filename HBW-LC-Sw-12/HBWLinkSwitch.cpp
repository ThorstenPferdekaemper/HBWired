/* 
** HBWLinkSwitch
**
** Direkte Verknuepfung (Peering), zu Schaltausgaengen
** Ein Link-Objekt steht immer fuer alle (direkt aufeinander folgenden) Verknuepfungen
** des entsprechenden Typs.
**
*/

#include "HBWLinkSwitch.h"

#define EEPROM_SIZE 20


HBWLinkSwitch::HBWLinkSwitch(uint8_t _numLinks, uint16_t _eepromStart) {
	numLinks = _numLinks;
	eepromStart = _eepromStart;
}
 
// processKeyEvent wird aufgerufen, wenn ein Tastendruck empfangen wurde
// TODO: von wem aufgerufen? Direkt von der Tasten-Implementierung oder vom Device? 
//       wahrscheinlich besser vom Device ueber sendKeyEvent
// TODO: Der Beginn aller Verknuepfungen ist gleich. Eigentlich koennte man 
//       das meiste in einer gemeinsamen Basisklasse abhandeln
void HBWLinkSwitch::receiveKeyEvent(HBWDevice* device, uint32_t senderAddress, uint8_t senderChannel, 
                                          uint8_t targetChannel, boolean longPress) {

  uint32_t sndAddrEEPROM;
  uint8_t channelEEPROM;
  uint8_t actionType;

  uint8_t data[7];  // store all peer parameter
  uint8_t *pdata = data;
  
  // read what to do from EEPROM
  for(byte i = 0; i < numLinks; i++) {
   //action = 2; // TODO set default? - inactive
   
	  device->readEEPROM(&sndAddrEEPROM, eepromStart + EEPROM_SIZE * i, 4, true);
	  // TODO: is the following really ok?
	  //       it reads to all links, even if none is set 
	  if(sndAddrEEPROM == 0xFFFFFFFF) continue;
	  if(sndAddrEEPROM != senderAddress) continue;
	  // compare sender channel
	  device->readEEPROM(&channelEEPROM, eepromStart + EEPROM_SIZE * i + 4, 1);
	  if(channelEEPROM != senderChannel) continue;
	  // compare target channel
	  device->readEEPROM(&channelEEPROM, eepromStart + EEPROM_SIZE * i + 5, 1);
	  if(channelEEPROM != targetChannel) continue;
	  // ok, we have found a match
    if (!longPress) { // differs for short and long
      device->readEEPROM(&actionType, eepromStart + EEPROM_SIZE * i + 6, 1);      // read shortPress actionType
      if (actionType & B00000001) {   // SHORT_ACTION_TYPE, ACTIVE
        // read other values and call set()
        device->readEEPROM(&data, eepromStart + EEPROM_SIZE * i + 6, 7);     // read all parameters
//          device->readEEPROM(&data[1], eepromStart + EEPROM_SIZE * i + 7, 1);      // read SHORT_ONDELAY_TIME
//          device->readEEPROM(&data[2], eepromStart + EEPROM_SIZE * i + 8, 1);      // read SHORT_ON_TIME
//          device->readEEPROM(&data[3], eepromStart + EEPROM_SIZE * i + 9, 1);      // read SHORT_OFFDELAY_TIME
//          device->readEEPROM(&data[4], eepromStart + EEPROM_SIZE * i + 10, 1);      // read SHORT_OFF_TIME
//          device->readEEPROM(&jumpTable, eepromStart + EEPROM_SIZE * i + 11, 2);      // read SHORT_JT_* table
//          device->get(targetChannel, &channelValue);
        device->set(targetChannel,7,pdata);    // channel, data length, data
      }
    }
    // read specific long action eeprom section
    else {
      device->readEEPROM(&actionType, eepromStart + EEPROM_SIZE * i + 13, 1); // read longPress actionType
      if (actionType & B00000001) {  // LONG_ACTION_TYPE, ACTIVE
        // read other values and call set()
        device->readEEPROM(&data, eepromStart + EEPROM_SIZE * i + 13, 7);     // read all parameters
        device->set(targetChannel,7,pdata);    // channel, data length, data
      }
    }
  }
}
 

