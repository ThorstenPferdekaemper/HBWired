/* 
** HBWLinkKey
**
** Einfache direkte Verknuepfung (Peering), vom Tastereingang ausgehend
** Ein Link-Objekt steht immer fuer alle (direkt aufeinander folgenden) Verknuepfungen
** des entsprechenden Typs.
**
*/


template<uint8_t numLinks, uint16_t eepromStart>
HBWLinkKey<numLinks, eepromStart>::HBWLinkKey() {
}


// keyPressed wird aufgerufen, wenn ein Tastendruck erkannt wurde
// TODO: von wem? Direkt von der Tasten-Implementierung oder vom Device? 
//       wahrscheinlich besser vom Device ueber sendKeyEvent
// TODO: Der Beginn aller Verknuepfungen ist gleich. Eigentlich koennte man 
//       das meiste in einer gemeinsamen Basisklasse abhandeln
template<uint8_t numLinks, uint16_t eepromStart>
void HBWLinkKey<numLinks, eepromStart>::sendKeyEvent(HBWDevice* device, uint8_t srcChan, 
                                    uint8_t keyPressNum, boolean longPress) {
	uint8_t channelEEPROM;
	uint32_t addrEEPROM;
    // care for peerings
	for(int i = 0; i < numLinks; i++) {
		// get channel number
		device->readEEPROM(&channelEEPROM, eepromStart + EEPROM_SIZE * i, 1);
		// valid peering?
		// TODO: Really like that? This always goes through all possible peerings
		if(channelEEPROM == 0xFF) continue;
		// channel is key?
		if(channelEEPROM != srcChan) continue;
		// get address and channel of actuator
		device->readEEPROM(&addrEEPROM, eepromStart + EEPROM_SIZE * i + 1, 4, true);
		device->readEEPROM(&channelEEPROM, eepromStart + EEPROM_SIZE * i +5, 1);
		// own address? -> internal peering
		if(addrEEPROM == device->getOwnAddress()) {
			device->receiveKeyEvent(addrEEPROM, srcChan, channelEEPROM, keyPressNum, longPress);
		}else{
			// external peering
			// TODO: If bus busy, then try to repeat. ...aber zuerst feststellen, wie das die Original-Module machen (bzw. hier einfach so lassen)
			/* byte result = */ 
			device->sendKeyEvent(srcChan, keyPressNum, longPress, addrEEPROM, channelEEPROM, !NEED_IDLE_BUS, PEER_SEND_RETRIES);  // free/idle bus was checked before calling sendKeyEvent - by sending a broadcast message
		};
	}; 
}
