/* 
** HBWLinkSwitchSimple
**
** Einfache direkte Verknuepfung (Peering), zu Schaltausgaengen
** Ein Link-Objekt steht immer fuer alle (direkt aufeinander folgenden) Verknuepfungen
** des entsprechenden Typs.
**
*/


template<uint8_t numLinks, uint16_t eepromStart>
HBWLinkSwitchSimple<numLinks, eepromStart>::HBWLinkSwitchSimple() {
}


// processKeyEvent wird aufgerufen, wenn ein Tastendruck empfangen wurde
// TODO: von wem aufgerufen? Direkt von der Tasten-Implementierung oder vom Device? 
//       wahrscheinlich besser vom Device ueber sendKeyEvent
// TODO: Der Beginn aller Verknuepfungen ist gleich. Eigentlich koennte man 
//       das meiste in einer gemeinsamen Basisklasse abhandeln
template<uint8_t numLinks, uint16_t eepromStart>
void HBWLinkSwitchSimple<numLinks, eepromStart>::receiveKeyEvent(HBWDevice* device, uint32_t senderAddress, uint8_t senderChannel, 
                                          uint8_t targetChannel, uint8_t keyPressNum, boolean longPress) {

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
	  if (longPress) actionType >>= 2;
	  actionType &= 0b00000011;
	  uint8_t data;
	  // we can have
	  switch(actionType) {
	  // 0 -> ON
	    case 0: data = 200;
		        break;
	  // 1 -> OFF
	    case 1: data = 0;
		        break;
	  // 2 -> INACTIVE
	    case 2: continue;
	  // 3 -> TOGGLE (default)
	    case 3: data = 255;
                // break;
      };
	  device->set(targetChannel,1,&data);    // channel, data length, data
  }
}
