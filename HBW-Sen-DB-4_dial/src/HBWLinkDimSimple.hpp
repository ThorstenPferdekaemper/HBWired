/* 
** HBWLinkDimSimple
**
** Einfache direkte Verknuepfung (Peering), zu Schaltausgaengen
** Ein Link-Objekt steht immer fuer alle (direkt aufeinander folgenden) Verknuepfungen
** des entsprechenden Typs.
**
*/

// TODO: add keyPressNum & sameLastSender ?

template<uint8_t numLinks, uint16_t eepromStart>
HBWLinkDimSimple<numLinks, eepromStart>::HBWLinkDimSimple() {
}


// processKeyEvent wird aufgerufen, wenn ein Tastendruck empfangen wurde
// TODO: von wem aufgerufen? Direkt von der Tasten-Implementierung oder vom Device? 
//       wahrscheinlich besser vom Device ueber sendKeyEvent
// TODO: Der Beginn aller Verknuepfungen ist gleich. Eigentlich koennte man 
//       das meiste in einer gemeinsamen Basisklasse abhandeln
template<uint8_t numLinks, uint16_t eepromStart>
void HBWLinkDimSimple<numLinks, eepromStart>::receiveKeyEvent(HBWDevice* device, uint32_t senderAddress, uint8_t senderChannel, 
                                          uint8_t targetChannel, uint8_t keyPressNum, boolean longPress) {

  uint32_t sndAddrEEPROM;
  uint8_t channelEEPROM;
  uint8_t actionType;
  uint8_t data[3];
  data[1] = keyPressNum;
  data[2] = false;
  
  if (senderAddress == lastSenderAddress && senderChannel == lastSenderChannel) {
    data[2] = true;  // true, as this was the same sender (source device & channel) - sameLastSender
  }
  lastSenderAddress = senderAddress;
  lastSenderChannel = senderChannel;

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
	  if (longPress) actionType >>= 4;
	  actionType &= 0b00001111;  // action type stored in address +6 and +6.4, size 0.4
	  // we can have
	  switch(actionType) {
	  // 0 -> OFF
	    case 0: //data[0] = 0;
		        // break;
	  // 1-12 -> dim level (step width 8% - last step 12%)
	    case 1:
	    case 2:
	    case 3:
	    case 4:
	    case 5:
	    case 6:
	    case 7:
	    case 8:
	    case 9:
	    case 10:
	    case 11: data[0] = (8* actionType)*2;
		        break;
	  // 12 -> ON 100% (default)
	    case 12: data[0] = 200;
		        break;
	  // 13 -> OLD_LEVEL // last on level
	    case 13: data[0] = 201;
		        break;
	  // 14 -> INACTIVE
	    case 14: continue;
	  // 15 -> TOGGLE
	    case 15: data[0] = 255;
                // break;
      };
	  device->set(targetChannel, sizeof(data), data);    // channel, data length, data
  }
}
