/* 
** HBWLinkBlindSimple
**
** Einfache direkte Verknuepfung (Peering), zu Schaltausgaengen
** Ein Link-Objekt steht immer fuer alle (direkt aufeinander folgenden) Verknuepfungen
** des entsprechenden Typs.
**
*/


template<uint8_t numLinks, uint16_t eepromStart>
HBWLinkBlindSimple<numLinks, eepromStart>::HBWLinkBlindSimple() {
}


// processKeyEvent wird aufgerufen, wenn ein Tastendruck empfangen wurde
// TODO: von wem aufgerufen? Direkt von der Tasten-Implementierung oder vom Device? 
//       wahrscheinlich besser vom Device ueber sendKeyEvent
// TODO: Der Beginn aller Verknuepfungen ist gleich. Eigentlich koennte man 
//       das meiste in einer gemeinsamen Basisklasse abhandeln
template<uint8_t numLinks, uint16_t eepromStart>
void HBWLinkBlindSimple<numLinks, eepromStart>::receiveKeyEvent(HBWDevice* device, uint32_t senderAddress, uint8_t senderChannel, 
                                          uint8_t targetChannel, uint8_t keyPressNum, boolean longPress) {

  uint32_t sndAddrEEPROM;
  uint8_t channelEEPROM;
  uint8_t actionType;
  uint8_t data[NUM_PEER_PARAMS +1];  // store all peer parameter (use extra element for keyPressNum)
  data[NUM_PEER_PARAMS] = keyPressNum;
  
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
   	  if (longPress) {
		  actionType = (actionType >> 3) & B00000111;
		  device->readEEPROM(&data, eepromStart + EEPROM_SIZE * i + 8, 1);
	  }
	  else {
		  actionType &= B00000111;
		  device->readEEPROM(&data, eepromStart + EEPROM_SIZE * i + 7, 1);
	  }
	  // we can have
	  switch(actionType) {
	  // 0 -> OPEN
	    case 0: data[0] = 200;
		        break;
	  // 1 -> STOP
	    case 1: data[0] = 201;
		        break;
	  // 2 -> CLOSE
	    case 2: data[0] = 0;
				break;
	  // 3 -> TOGGLE (default)
	    case 3: data[0] = 255;
                break;
	  // 4 -> LEVEL
	    case 4: { asm ("nop"); } // target level already stored in "data" - nothing to do...
                break;
	  // 5 -> INACTIVE
	    case 5: continue;
      };
	  device->set(targetChannel, 2, data);    // channel, data length, data
  }
}
