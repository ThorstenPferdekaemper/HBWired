/* 
** HBWLinkBlindSimple
**
** Einfache direkte Verknuepfung (Peering), zu Schaltausgaengen
** Ein Link-Objekt steht immer fuer alle (direkt aufeinander folgenden) Verknuepfungen
** des entsprechenden Typs.
**
*/

#include "HBWLinkBlindSimple.h"

#define EEPROM_SIZE 7


HBWLinkBlindSimple::HBWLinkBlindSimple(uint8_t _numLinks, uint16_t _eepromStart) {
	numLinks = _numLinks;
	eepromStart = _eepromStart;
}
 
// processKeyEvent wird aufgerufen, wenn ein Tastendruck empfangen wurde
// TODO: von wem aufgerufen? Direkt von der Tasten-Implementierung oder vom Device? 
//       wahrscheinlich besser vom Device ueber sendKeyEvent
// TODO: Der Beginn aller Verknuepfungen ist gleich. Eigentlich koennte man 
//       das meiste in einer gemeinsamen Basisklasse abhandeln
void HBWLinkBlindSimple::receiveKeyEvent(HBWDevice* device, uint32_t senderAddress, uint8_t senderChannel, 
                                          uint8_t targetChannel, boolean longPress) {

  uint32_t sndAddrEEPROM;
  uint8_t channelEEPROM;
  uint8_t actionType;
  // read what to do from EEPROM
  for(byte i = 0; i < numLinks; i++) {
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
      // read actionType
   	  device->readEEPROM(&actionType, eepromStart + EEPROM_SIZE * i + 6, 1);
	  // differs for short and long
   	  if (longPress)
		  actionType = (actionType >> 3) & B00000111;
	  else
		  actionType &= B00000111;
	  uint8_t data;
	  // we can have
	  switch(actionType) {
	  // 0 -> OPEN
	    case 0: data = 200;
		        break;
	  // 1 -> STOP
	    case 1: data = 201;
		        break;
	  // 2 -> CLOSE
	    case 2: data = 0;
				break;
	  // 3 -> TOGGLE (default)
	    case 3: data = 255;
                break;
	  // 4 -> INACTIVE
	    case 4: continue;
      };
	  device->set(targetChannel,1,&data);    // channel, data length, data
  }
}
 

