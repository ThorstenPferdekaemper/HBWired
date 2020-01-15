/* 
** HBWLinkInfoEventSensor
**
** Einfache direkte Verknuepfung (Peering), vom Sensor ausgehend
** Ein Link-Objekt steht immer fuer alle (direkt aufeinander folgenden) Verknuepfungen
** des entsprechenden Typs.
**
*/


#ifdef Support_HBWLink_InfoEvent
template<uint8_t numLinks, uint16_t eepromStart>
HBWLinkInfoEventSensor<numLinks, eepromStart>::HBWLinkInfoEventSensor() {
}


// TODO: Der Beginn aller Verknuepfungen ist gleich. Eigentlich koennte man 
//       das meiste in einer gemeinsamen Basisklasse abhandeln
template<uint8_t numLinks, uint16_t eepromStart>
void HBWLinkInfoEventSensor<numLinks, eepromStart>::sendInfoEvent(HBWDevice* device, uint8_t srcChan, 
                                    uint8_t length, uint8_t const * const data) {
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
			device->receiveInfoEvent(addrEEPROM, srcChan, channelEEPROM, length, data);
		}else{
			// external peering
			// TODO: If bus busy, then try to repeat. ...aber zuerst feststellen, wie das die Original-Module machen (bzw. hier einfach so lassen)
			/* byte result = */ 
			device->sendInfoEvent(srcChan, length, data, addrEEPROM, channelEEPROM, !NEED_IDLE_BUS, 1);  // free/idle bus was checked before calling sendInfoEvent. Send peer message only once
		};
	}; 
}
#endif
