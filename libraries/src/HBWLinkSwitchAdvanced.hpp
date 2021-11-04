/* 
** HBWLinkSwitchAdvanced
**
** Direkte Verknuepfung (Peering), zu Schaltausgaengen
** Ein Link-Objekt steht immer fuer alle (direkt aufeinander folgenden) Verknuepfungen
** des entsprechenden Typs.
**
** http://loetmeister.de/Elektronik/homematic/
*/


template<uint8_t numLinks, uint16_t eepromStart>
HBWLinkSwitchAdvanced<numLinks, eepromStart>::HBWLinkSwitchAdvanced() {
}


// receiveKeyEvent wird aufgerufen, wenn ein Tastendruck empfangen wurde
// TODO: Der Beginn aller Verknuepfungen ist gleich. Eigentlich koennte man 
//       das meiste in einer gemeinsamen Basisklasse abhandeln
template<uint8_t numLinks, uint16_t eepromStart>
void HBWLinkSwitchAdvanced<numLinks, eepromStart>::receiveKeyEvent(HBWDevice* device, uint32_t senderAddress, uint8_t senderChannel, 
                                          uint8_t targetChannel, uint8_t keyPressNum, boolean longPress) {
  
  uint32_t sndAddrEEPROM;
  uint8_t channelEEPROM;
  uint8_t actionType;
  uint8_t data[NUM_PEER_PARAMS +2];  // store all peer parameter (use extra element for keyPressNum & sameLastSender)
  
  data[NUM_PEER_PARAMS] = keyPressNum;
  data[NUM_PEER_PARAMS +1] = false;
  
  if (senderAddress == lastSenderAddress && senderChannel == lastSenderChannel) {
    data[NUM_PEER_PARAMS +1] = true;  // true, as this was the same sender (source device & channel) - sameLastSender
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
    if (!longPress) { // differs for short and long
      device->readEEPROM(&actionType, eepromStart + EEPROM_SIZE * i + 6, 1);      // read shortPress actionType
      if (actionType & B00001111) {   // SHORT_ACTION_TYPE, ACTIVE
        // when active, read all other values and call channel set()
        device->readEEPROM(&data, eepromStart + EEPROM_SIZE * i + 6, NUM_PEER_PARAMS);     // read all parameters (must be consecutive)
      //           + 6        //  SHORT_ACTION_TYPE
      //           + 7        //  SHORT_ONDELAY_TIME
      //           + 8        //  SHORT_ON_TIME
      //           + 9        //  SHORT_OFFDELAY_TIME
      //           + 10       //  SHORT_OFF_TIME
      //           + 11, 12   //  SHORT_JT_* table
        device->set(targetChannel, sizeof(data), data);    // channel, data length, data
      }
    }
    // read specific long action eeprom section
    else {
      device->readEEPROM(&actionType, eepromStart + EEPROM_SIZE * i + 6 + NUM_PEER_PARAMS, 1); // read longPress actionType
      if (actionType & B00001111) {  // LONG_ACTION_TYPE, ACTIVE
        // when active, read all other values and call channel set()
        device->readEEPROM(&data, eepromStart + EEPROM_SIZE * i + 6 + NUM_PEER_PARAMS, NUM_PEER_PARAMS);     // read all parameters (must be consecutive)
        device->set(targetChannel, sizeof(data), data);    // channel, data length, data
      }
    }
  }
}
