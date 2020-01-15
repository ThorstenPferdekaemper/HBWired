/* 
** HBWLinkInfoEventActuator
**
** Einfache direkte Verknuepfung (Peering), zu Schaltausgaengen
** Ein Link-Objekt steht immer fuer alle (direkt aufeinander folgenden) Verknuepfungen
** des entsprechenden Typs.
**
*/


#ifdef Support_HBWLink_InfoEvent
template<uint8_t numLinks, uint16_t eepromStart>
HBWLinkInfoEventActuator<numLinks, eepromStart>::HBWLinkInfoEventActuator() {
}


// TODO: Der Beginn aller Verknuepfungen ist gleich. Eigentlich koennte man 
//       das meiste in einer gemeinsamen Basisklasse abhandeln
template<uint8_t numLinks, uint16_t eepromStart>
void HBWLinkInfoEventActuator<numLinks, eepromStart>::receiveInfoEvent(HBWDevice* device, uint32_t senderAddress, uint8_t senderChannel, 
                                          uint8_t targetChannel, uint8_t length, uint8_t const * const data) {

  uint32_t sndAddrEEPROM;
  uint8_t channelEEPROM;
  
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

	  
	  device->setInfo(targetChannel, length, data);    // channel, data length, data
  }
}
#endif
