/*
 * HBWired.cpp
 *
 *  Created on: 19.11.2016
 *      Author: Thorsten Pferdekaemper thorsten@pferdekaemper.com
 *      Nach einer Vorlage von Dirk Hoffmann (dirk@hfma.de) 2012
 *
 *  HomeBrew-Wired RS485-Protokoll 
 */

#include "HBWired.h"

#include "Arduino.h"
#include <EEPROM.h>


// bus must be idle 210 + rand(0..100) ms
#define DIFS_CONSTANT 210
#define DIFS_RANDOM 100
// we wait max 200ms for an ACK
#define ACKWAITTIME 200
// while waiting for an ACK, bus might not be idle
// bus has to be idle at least this time for a retry
#define RETRYIDLETIME 150

// Defines
#define FRAME_STARTBYTE 0xFD
#define CRC16_POLYNOM 0x1002
#define ESCAPE_CHAR 0xFC

// Empfangs-Status
#define FRAME_START 1    // Startzeichen gefunden
#define FRAME_ESCAPE 2   // Escape-Zeichen gefunden
#define FRAME_SENTACKWAIT 8  // Nachricht wurde gesendet, warte auf ACK

// Methods

// eigene Adresse setzen und damit auch random seed
void HBWDevice::setOwnAddress(uint32_t address) {
  ownAddress = address;
  randomSeed(ownAddress);
  minIdleTime = random(DIFS_CONSTANT, DIFS_CONSTANT+DIFS_RANDOM);
}


uint32_t HBWDevice::getOwnAddress() {
	return ownAddress;
}


boolean HBWDevice::parseFrame () { // returns true, if event needs to be processed by the module
// Wird aufgerufen, wenn eine komplette Nachricht empfangen wurde

  byte seqNumReceived;
  byte seqNumSent;

//### START ACK verarbeiten ###
  if(frameStatus & FRAME_SENTACKWAIT) {   // wir warten auf ACK?
    // Empfangene Sendefolgenummer muss stimmen
    seqNumReceived = (frameControlByte >> 5) & 0x03;
    seqNumSent     = (txFrameControlByte >> 1) & 0x03;
    // Absenderadresse mus stimmen
    if(seqNumReceived == seqNumSent && senderAddress == txTargetAddress) {
      // "ackwait" zuruecksetzen (ansonsten wird auf den Timeout vertraut)
      hbwdebug(F("R: ACK\n"));
      frameStatus &= ~FRAME_SENTACKWAIT;
    }
  }
// ### END ACK verarbeiten ###
// Muss das Modul was dazu sagen?
// Nein, wenn es nur ein ACK ist oder die Laenge der Daten 0 ist

// Nur ein ACK
  if((frameControlByte & 0x03) == 1) return false;
// Leere Message und kein Broadcast? -> Ein ACK senden koennen wir selbst
  if(frameDataLength == 0 && targetAddress != 0xFFFFFFFF) {
    txTargetAddress = senderAddress;
    sendAck();
    return false;
  };
// ja, das Modul sollte seine Meinung dazu sagen
  return true;
} // parseFrame


// send Frame, wait for ACK and maybe repeat
// onlyIfIdle: If this is set, then the bus must have been idle since 210+rand(0..100) ms
// sendFrame returns...
//   0 -> ok
//   1 -> bus not idle (only if onlyIfIdle)
//   2 -> three times no ACK (cannot occur for broadcasts or ACKs)
byte HBWDevice::sendFrame(boolean onlyIfIdle){
// TODO: non-blocking
// TODO: Wenn als Antwort kein reines ACK kommt, dann geht die Antwort verloren
//       D.h. sie wird nicht interpretiert. Die Gegenstelle sollte es dann nochmal
//       senden, aber das ist haesslich (andererseits scheint das nicht zu passieren)

// carrier sense
   if(onlyIfIdle) {
	 if(millis() - lastReceivedTime < minIdleTime)
		 return 1;
	 // set new idle time
	 minIdleTime = random(DIFS_CONSTANT, DIFS_CONSTANT+DIFS_RANDOM);
   }

// simple send for ACKs and Broadcasts
  if(txTargetAddress == 0xFFFFFFFF || ((txFrameControlByte & 0x03) == 1)) {
	sendFrameSingle();
	// TODO: nicht besonders schoen, zuerst ackwait zu setzen und dann wieder zu loeschen
	frameStatus &= ~FRAME_SENTACKWAIT; // we do not really wait
	return 0;  // we do not wait for an ACK, i.e. always ok
  };

  uint32_t lastTry = 0;

  for(byte i = 0; i < 3; i++) {  // maximal 3 Versuche
    sendFrameSingle();
    lastTry = millis();
// wait for ACK
    // TODO: Die Wartezeit bis die Uebertragung wiederholt wird sollte einen Zufallsanteil haben
    while(millis() - lastTry < ACKWAITTIME) {   // normally 200ms
// Daten empfangen (tut nichts, wenn keine Daten vorhanden)
      receive();
// Check
// TODO: Was tun, wenn inzwischen ein Broadcast kommt, auf den wir reagieren muessten?
//       (Pruefen, welche das sein koennten.)
      if(frameComplete) {
        if(targetAddress == ownAddress){
          frameComplete = 0;
          parseFrame();
          if(!(frameStatus & FRAME_SENTACKWAIT))  // ACK empfangen
            return 0;  // we have an ACK, i.e. ok
        };
      };
    };
    // We have not received an ACK. However, there might be
    // other stuff on the bus and we might better keep quiet
    // TODO: random part?
    if(onlyIfIdle && millis() - lastTry < RETRYIDLETIME)
        return 1;	// bus is not really free
  };
  return 2;  // three times without ACK
}


// Send a whole data frame.
// The following attributes must be set
//
// hmwTxTargetAdress(4)                   the target adress
// hmwTxFrameControllByte                 the controll byte
// hmwTxSenderAdress(4)                   the sender adress
// hmwTxFrameDataLength                   the length of data to send
// hmwTxFrameData(MAX_RX_FRAME_LENGTH)    the data array to send
void HBWDevice::sendFrameSingle() {

      byte tmpByte;
      uint16_t crc16checksum = 0xFFFF;

 // TODO: Das Folgende nimmt an, dass das ACK zur letzten empfangenen Sendung gehoert
 //       Wahrscheinlich stimmt das immer oder ist egal, da die Gegenseite nicht auf
 //       ein ACK wartet. (Da man nicht kein ACK senden kann, ist jeder Wert gleich
 //       gut, wenn keins gesendet werden muss.)
      if(txTargetAddress != 0xFFFFFFFF){
        byte txSeqNum = (frameControlByte >> 1) & 0x03;
        txFrameControlByte &= 0x9F;
        txFrameControlByte |= (txSeqNum << 5);
      };

      hbwdebug(F("T: "));
      digitalWrite(txEnablePin, HIGH);
	  hbwdebughex(FRAME_STARTBYTE);
      serial->write(FRAME_STARTBYTE);  // send startbyte
      crc16Shift(FRAME_STARTBYTE , &crc16checksum);

      byte i;
      uint32_t address = txTargetAddress;
      for( i = 0; i < 4; i++){      // send target address
    	 tmpByte = address >> 24;
         sendFrameByte( tmpByte, &crc16checksum);
         address = address << 8;
      };

      sendFrameByte(txFrameControlByte, &crc16checksum);

      if(bitRead(txFrameControlByte,3)){                                      // check if message has sender
    	  address = ownAddress;
    	  for( i = 0; i < 4; i++){                                           // send sender address
    	    	 tmpByte = address >> 24;
    	         sendFrameByte( tmpByte, &crc16checksum);
    	         address = address << 8;
    	  }
      };
      tmpByte = txFrameDataLength + 2;                              // send data length
      sendFrameByte(tmpByte, &crc16checksum);

    	for(i = 0; i < txFrameDataLength; i++){            // send data, falls was zu senden
            sendFrameByte(txFrameData[i], &crc16checksum);
    	}
      crc16Shift(0 , &crc16checksum);
      crc16Shift(0 , &crc16checksum);

      // CRC schicken
      sendFrameByte(crc16checksum / 0x100);
      sendFrameByte(crc16checksum & 0xFF);

      serial->flush();                  // othwerwise, enable pin will go low too soon
      digitalWrite(txEnablePin, LOW);

      frameStatus |= FRAME_SENTACKWAIT;
	  
	  hbwdebug(F("\n"));
} // sendFrameSingle


// Send a data byte.
// Before sending check byte for special chars. Special chars are escaped before sending
// TX-Pin needs to be HIGH before calling this
void HBWDevice::sendFrameByte(byte sendByte, uint16_t* checksum) {
	// Debug
	hbwdebug(":");
	hbwdebughex(sendByte);
	// calculate checksum, if needed
	if(checksum)
		crc16Shift(sendByte, checksum);
    // Add escape character, if needed
    if(sendByte == FRAME_STARTBYTE || sendByte == 0xFE || sendByte == ESCAPE_CHAR) {
       serial->write(ESCAPE_CHAR);
       sendByte &= 0x7F;
    };
    serial->write(sendByte);
};

// Sendet eine ACK-Nachricht
// Folgende Attribute MUESSEN vorher gesetzt sein:
// txTargetAdress
// txSenderAdress
void HBWDevice::sendAck() {
      txFrameControlByte = 0x19;
      txFrameDataLength = 0;
      sendFrame();
};


// calculate crc16 checksum for each byte
void HBWDevice::crc16Shift(uint8_t newByte , uint16_t* crc) {

  byte stat;

  for (byte i = 0; i < 8; i++) {
    if (*crc & 0x8000) {stat = 1;}
    else               {stat = 0;}
    *crc = (*crc << 1);
    if (newByte & 0x80) {
      *crc = (*crc | 1);
    }
    if (stat) {
      *crc = (*crc ^ CRC16_POLYNOM);
    }
    newByte = newByte << 1;
  }
}  // crc16Shift




// RS485 empfangen
// Muss zyklisch aufgerufen werden
void HBWDevice::receive(){

  // Frame Control Byte might be used before a message is completely received
  static byte rxFrameControlByte;

#define ADDRESSLENGTH 4
#define ADDRESSLENGTHLONG 9
  static byte framePointer;
  static byte addressPointer;
  static uint16_t crc16checksum;

// TODO: Kann sich hier zu viel "anstauen", so dass das while vielleicht
//       nach ein paar Millisekunden unterbrochen werden sollte?

  while(serial->available()) {
//  carrier sense
	lastReceivedTime = millis();

    byte rxByte = serial->read();    // von Serial oder SoftSerial

    // Debug
    if( rxByte == FRAME_STARTBYTE ){
		hbwdebug(F("R: "));
	}else{
		hbwdebug(F(":"));
	}	
    hbwdebughex(rxByte);

   if(rxByte == ESCAPE_CHAR && !(frameStatus & FRAME_ESCAPE)){
// TODO: Wenn frameEscape gesetzt ist, dann sind das zwei Escapes hintereinander
//       Das ist eigentlich ein Fehler -> Fehlerbehandlung
     frameStatus |= FRAME_ESCAPE;
   }else{
      if(rxByte == FRAME_STARTBYTE){  // Startzeichen empfangen
         frameStatus |= FRAME_START;
         frameStatus &= ~FRAME_ESCAPE;
         framePointer = 0;
         addressPointer = 0;
         senderAddress = 0;
         targetAddress = 0;
		 crc16checksum = 0xFFFF;
         crc16Shift(rxByte , &crc16checksum);
      }else if(frameStatus & FRAME_START) {  // Startbyte wurde gefunden und Frame wird nicht ignoriert
         if(frameStatus & FRAME_ESCAPE) {
            rxByte |= 0x80;
            frameStatus &= ~FRAME_ESCAPE;
         };
         crc16Shift(rxByte , &crc16checksum);
         if(addressPointer < ADDRESSLENGTH){  // Adressbyte Zieladresse empfangen
            targetAddress <<= 8;
            targetAddress |= rxByte;
            addressPointer++;
         }else if(addressPointer == ADDRESSLENGTH){   // Controlbyte empfangen
            addressPointer++;
            rxFrameControlByte = rxByte;
         }else if( bitRead(rxFrameControlByte,3) && addressPointer < ADDRESSLENGTHLONG) {
        	// Adressbyte Sender empfangen wenn CTRL_HAS_SENDER und FRAME_START_LONG
        	senderAddress <<= 8;
        	senderAddress |= rxByte;
            addressPointer++;
         }else if(addressPointer != 0xFF) { // Datenlänge empfangen
            addressPointer = 0xFF;
            frameDataLength = rxByte;
            if(frameDataLength > MAX_RX_FRAME_LENGTH) // Maximale Puffergöße checken.
            {
                frameStatus &= ~FRAME_START;
                hbwdebug(F("E: MsgTooLong\n"));
            }
         }else{                   // Daten empfangen
            frameData[framePointer] = rxByte;   // Daten in Puffer speichern
            framePointer++;
            if(framePointer == frameDataLength) {  // Daten komplett
               if(crc16checksum == 0) {    //
            	  frameStatus &= ~FRAME_START;
                  // Framedaten für die spätere Verarbeitung speichern
                  // TODO: Braucht man das wirklich?
                  frameControlByte = rxFrameControlByte;
                  frameDataLength -= 2;
                  // es liegt eine neue Nachricht vor
                  frameComplete = 1;
				  hbwdebug(F("\n"));
                  // auch wenn noch Daten im Puffer sind, muessen wir erst einmal
                  // die gerade gefundene Nachricht verarbeiten
                  return;
               }else{
            	  hbwdebug(F("E: CRC\n"));
               }
            }
         }
      }
    }
  }
} // receive



// Basisklasse fuer Channels, defaults
void HBWChannel::set(HBWDevice* device, uint8_t length, uint8_t const * const data) {};
uint8_t HBWChannel::get(uint8_t* data) { return 0; };   
void HBWChannel::loop(HBWDevice* device, uint8_t channel) {};    


// Processing of default events (related to all modules)
void HBWDevice::processEvent(byte const * const frameData, byte frameDataLength, boolean isBroadcast) {

      uint16_t adrStart;
      boolean onlyAck = true;

      // ACKs kommen hier nicht an, werden eine Schicht tiefer abgefangen

      // Wenn irgendwas von Broadcast kommt, dann gibt es darauf keine
      // Reaktion, ausser z und Z (und es kommt von der Zentrale)
      // TODO: Muessen wir pruefen, ob's wirklich von der Zentrale kommt?
      if(isBroadcast){
    	/* switch(frameData[0]){
    	  case 'Z':                                                               // End discovery mode
    	    // reset hmwModuleDiscovering
    	    // TODO: Discovery mode
    	    break;
    	  case 'z':                                                               // start discovery mode
    	    // set hmwModuleDiscovering
    	    // TODO: Discovery mode
    	    break;
    	} */
    	return;
      };

      txTargetAddress = senderAddress;
      // gibt es was zu verarbeiten -> Ja, die Kommunikationsschicht laesst nur Messages durch,
      // die auch Daten enthalten

      txFrameControlByte = 0x78;

      switch(frameData[0]){
         case '@':             // 0x40                 // HBW-specifics
           if(frameData[1] == 'a' && frameDataLength == 6) {  // 0x61 "a" -> address set
        	 // TODO: Avoid "central" addresses (like 0000...)
        	 for(byte i = 0; i < 4; i++)
        	   writeEEPROM(E2END - 3 + i, frameData[i + 2], true);
           };
           readAddressFromEEPROM();
           break;
         /* case '!':                                                             // reset the Module
            // reset the Module jump after the bootloader
        	// Nur wenn das zweite Zeichen auch ein "!" ist
        	// TODO: Wirklich machen, aber wie geht das?
            // if(frameData[1] == '!') { };   //  then goto 0
            break;  */
         case 'A':                                                             // Announce
        	txFrameData[0] = 'i';
			onlyAck = false;
            break;
         case 'C':                                                              // re read Config
        	readConfig();    // also calls back to device
            break;
         case 'E':                           // see separate docs
        	onlyAck = false;
        	processEmessage(frameData);
            break;
         case 'K':                           // 0x4B Key-Event
         case 0xCB:   // 'Ë':       // Key-Sim-Event TODO: Es gibt da einen theoretischen Unterschied
			receiveKeyEvent(senderAddress, frameData[1], frameData[2], frameData[3] & 0x01);
            break;
         case 'R':                                                              // Read EEPROM
        	// TODO: Check requested length...
            if(frameDataLength == 4) {                                // Length of incoming data must be 4
               onlyAck = false;
               hbwdebug(F("C: Read EEPROM\n"));
               adrStart = ((uint16_t)(frameData[1]) << 8) | frameData[2];  // start adress of eeprom
               for(byte i = 0; i < frameData[3]; i++) {
            	   txFrameData[i] = EEPROM.read(adrStart + i);
               };
               txFrameDataLength = frameData[3];
            };
            break;
         case 'S':                                                               // GET Level
            processEventGetLevel(frameData[1], frameData[0]);
            txFrameData[0] = 'i';
			onlyAck = false;
            break;
         case 'W':                                                               // Write EEPROM
            if(frameDataLength == frameData[3] + 4) {
            	hbwdebug(F("C: Write EEPROM\n"));
               adrStart = ((uint16_t)(frameData[1]) << 8) | frameData[2];  // start adress of eeprom
               for(byte i = 4; i < frameDataLength; i++){
            	 writeEEPROM(adrStart+i-4, frameData[i]);
               }
            };
            break;
         /* case 'c':                                                               // Zieladresse löschen?
            // TODO: ???
            break;  */

         case 'h':                                 // 0x68 get Module type and hardware version
        	hbwdebug(F("T: HWVer,Typ\n"));
            onlyAck = false;
            txFrameData[0] = deviceType;
            txFrameData[1] = hardware_version;
            txFrameDataLength = 2;
            break;
         /* case 'l':               // set Lock Does this really make sense?
            processEventSetLock();
            break;   */
         case 'n':                                       // Seriennummer
        	determineSerial(txFrameData);
        	txFrameDataLength = 10;
        	onlyAck = false;
        	break;
         /* case 'q':                                                               // Zieladresse hinzufügen?
            // TODO: ???
        	break; */
         /* case 'u':                                                              // Update (Bootloader starten)
            // Bootloader neu starten
            // Goto $7c00                                                          ' Adresse des Bootloaders
        	// TODO: Bootloader?
        	break; */
         case 'v':                                                               // get firmware version
            hbwdebug(F("T: FWVer\n"));
            onlyAck = false;
            txFrameData[0] = firmware_version / 0x100;
            txFrameData[1] = firmware_version & 0xFF;
            txFrameDataLength = 2;
            break;
         case 's':   // level set
         case 'x':   // Level set. In der Protokollbeschreibung steht hier was von "install test",
        	         // aber es sieht so aus, als ob 0x73 und 0x78 dasselbe tun
			set(frameData[1], frameDataLength-2, &(frameData[2]));
            // get what the hardware did and send it back
			// TODO: Is this really what the standard modules do?
			//       For slow modules, this might not be correct 
            processEventGetLevel(frameData[1], frameData[0]);  // for feedback
            txFrameData[0] = 'i';
            onlyAck = false; 
			break;
      }

      if(onlyAck){
    	  sendAck();
      }else{
    	  sendFrame();
      };
};


void HBWDevice::receiveKeyEvent(uint32_t senderAddress, uint8_t srcChan, 
	                             uint8_t dstChan, boolean longPress) {
    if(linkReceiver)								 
        linkReceiver->receiveKeyEvent(this, senderAddress, srcChan, dstChan, longPress);
};


   void HBWDevice::processEventGetLevel(byte channel, byte command){
	 // get value from the hardware and send it back
	 txFrameData[0] = 0x69;         // 'i'
	 txFrameData[1] = channel;      // Sensornummer
	 uint8_t length = get(channel, &(txFrameData[2]));
	 // hmwrs485->txFrameData[2] = info / 0x100;
	 // hmwrs485->txFrameData[3] = info & 0xFF;
	 txFrameDataLength = 0x02 + length;      // Length
   };


   void HBWDevice::processEmessage(uint8_t const * const frameData) {
	 // process E-Message

     uint8_t blocksize = frameData[3];
     uint8_t blocknum  = frameData[4];

     // length of response
     txFrameDataLength = 4 + blocknum / 8;
     // care for odd block numbers
     if(blocknum % 8) txFrameDataLength++;
     // we don't need to check the size as it can maximum
     // be 4 + 255 div 8 + 1 = 36
     // init to zero, mainly because we need it later
     memset(txFrameData,0,txFrameDataLength);
     // first byte "e" - answer on "E"
     txFrameData[0]  = 0x65;  //e
     // next 3 bytes are just repeated from request
     txFrameData[1]  = frameData[1];
     txFrameData[2]  = frameData[2];
     txFrameData[3]  = frameData[3];

     // determine whether blocks are used
     for(int block = 0; block <= blocknum; block++) {
       // check this memory block
       for(int byteIdx = 0; byteIdx < blocksize; byteIdx++) {
    	 if(EEPROM.read(block * blocksize + byteIdx) != 0xFF) {
    	   bitSet(txFrameData[4 + block / 8], block % 8);
    	   break;
    	 }
       }
     };
   };


   // "Announce-Message" ueber broadcast senden
   byte HBWDevice::broadcastAnnounce(byte channel) {
      txTargetAddress = 0xFFFFFFFF;  // broadcast
      txFrameControlByte = 0xF8;     // control byte
      txFrameDataLength = 16;      // Length
      txFrameData[0] = 0x41;         // 'i'
      txFrameData[1] = channel;      // Sensornummer
      txFrameData[2] = deviceType;
      txFrameData[3] = 0;          // TODO: should this be hardware version?
      txFrameData[4] = firmware_version / 0x100;
      txFrameData[5] = firmware_version & 0xFF;
      determineSerial(txFrameData + 6);
      return sendFrame(true);  // only if bus is free
   };


   // "i-Message" senden
   // this is only called from "outside" and not as a response
 uint8_t HBWDevice::sendInfoMessage(uint8_t channel, uint8_t length, uint8_t const * const data, uint32_t target_address) {
	   txTargetAddress = target_address;
       if(!txTargetAddress) txTargetAddress = getCentralAddress();	
       txFrameControlByte = 0xF8;     // control byte
  	   txFrameDataLength = 0x02 + length;      // Length
  	   txFrameData[0] = 0x69;         // 'i'
  	   txFrameData[1] = channel;      // Sensornummer
	   memcpy(&(txFrameData[2]), data, length);
  	   return sendFrame(true);  // only if bus is free
     };

	 
 // key-Event senden, inklusive peers etc.
uint8_t HBWDevice::sendKeyEvent(uint8_t srcChan, uint8_t keyPressNum, boolean longPress) {
    if(linkSender)
        linkSender->sendKeyEvent(this, srcChan, keyPressNum, longPress);
    return sendKeyEvent(srcChan, keyPressNum, longPress, 0xFFFFFFFF, 0);  // only if bus is free
};

	 
// key-Event senden an bestimmtes Target
 uint8_t HBWDevice::sendKeyEvent(uint8_t srcChan, uint8_t keyPressNum, boolean longPress, uint32_t targetAddr, uint8_t targetChan) {
 	   txTargetAddress = targetAddr;  // target address
 	   txFrameControlByte = 0xF8;     // control byte
 	   txFrameDataLength = 0x04;      // Length
 	   txFrameData[0] = 0x4B;         // 'K'
 	   txFrameData[1] = srcChan;      // Sensornummer
 	   txFrameData[2] = targetChan;   // Zielaktor
 	   txFrameData[3] = (longPress ? 3 : 2) + (keyPressNum << 2);
 	   return sendFrame(true);  // only if bus is free
 } ;


 // Key-Event senden mit Geraetespezifischen Daten (nur Broadcast)
 uint8_t HBWDevice::sendKeyEvent(uint8_t srcChan, uint8_t length, void* data) {
 	   txTargetAddress = 0xFFFFFFFF;  // target address
 	   txFrameControlByte = 0xF8;     // control byte
 	   txFrameDataLength = 3 + length;      // Length
 	   txFrameData[0] = 0x4B;         // 'K'
 	   txFrameData[1] = srcChan;      // Sensornummer
 	   txFrameData[2] = 0;   // Zielaktor
  	   memcpy(&(txFrameData[3]), data, length);
 	   return sendFrame(true);  // only if bus is free
 } ;

 
	 
     void HBWDevice::writeEEPROM(int16_t address, byte value, bool privileged ) {
       // save uppermost 4 bytes
       if(!privileged && (address > E2END - 4))
    	 return;
       // write if not anyway the same value
   	   if(value != EEPROM.read(address))
   		 EEPROM.write(address, value);
     };


     // read device address from EEPROM
     // TODO: Avoid "central" addresses (like 0000...)
     void HBWDevice::readAddressFromEEPROM(){
       uint32_t address = 0;

       for(byte i = 0; i < 4; i++){
    	   address <<= 8;
    	   address |= EEPROM.read(E2END - 3 + i);
       }
       if(address == 0xFFFFFFFF)
    	   address = 0x42FFFFFF;
       setOwnAddress(address);
     }


     void HBWDevice::determineSerial(byte* buf) {
       char numAsStr[20];
       sprintf(numAsStr, "%07lu", getOwnAddress() % 10000000L );
       buf[0] = 'H';
       buf[1] = 'B';
       buf[2] = 'W';
       memcpy(buf+3, numAsStr, 7);
     };

	 
void HBWDevice::readConfig() {         // read config from EEPROM	
    // read EEPROM
    readEEPROM(config, 0x01, configSize);
    // TODO: switch debugging
	//       should probably be done in main program
    /* if(config.nodebug){
      hbwdebugstream = NULL;
    }else{
        hbwdebugstream = &Serial;
    }; */
    // turn around central address
    uint32_t addr = *((uint32_t*)(config + 1));
    for(uint8_t i = 0; i < 4; i++)
      config[i+1] = ((uint8_t*)(&addr))[3-i];
    // set defaults if values not provided from EEPROM
	// or other device specific stuff
    //afterReadConfig();
	afterReadConfigPending = true; // tell main loop to run afterReadConfig() for device and channels //TODO test afterReadConfig
}


// get central address
uint32_t HBWDevice::getCentralAddress() {
	return *((uint32_t*)(config + 1));
}

	

// EEPROM lesen
void HBWDevice::readEEPROM(void* dst, uint16_t address, uint16_t length, 
                           boolean lowByteFirst) {
  byte* ptr = (byte*)(dst);
  for(uint16_t offset = 0; offset < length; offset++){
    *ptr = EEPROM.read(address + (lowByteFirst ? length - 1 - offset : offset));
    ptr++;
  };
};


// broadcast announce message once at the beginning
// this might need to "wait" until the bus is free
void HBWDevice::handleBroadcastAnnounce() {
  static boolean announced = false;
  // avoid sending broadcast in the first second
  // we don't care for the overflow as the announce
  // is sent after 40 days anyway (hopefully)
  if(millis() < 1000) return;
  if(announced) return;
  // send methods return 0 if everything is ok
  announced = (broadcastAnnounce(0) == 0);
}
	 

/*
********************************
** DEBUG
********************************
*/

Stream* hbwdebugstream = 0;


/*
************************************
** The HBWired-Device
************************************
*/

HBWDevice::HBWDevice(uint8_t _devicetype, uint8_t _hardware_version, uint16_t _firmware_version,
              Stream* _rs485, uint8_t _txen, 
	          uint8_t _configSize, void* _config, 
			  uint8_t _numChannels, HBWChannel** _channels,
			  Stream* _debugstream, HBWLinkSender* _linkSender, HBWLinkReceiver* _linkReceiver) {	
    configSize = _configSize;     // size of config object without peerings
	config = (uint8_t*)_config;        // pointer to config object 
	numChannels = _numChannels;    // number of channels
	channels = _channels;  // channels
	linkSender = _linkSender;
	linkReceiver = _linkReceiver;
	hardware_version = _hardware_version;
    firmware_version = _firmware_version;	
	// lower layer						 
	serial = _rs485;
	txEnablePin = _txen;
	pinMode(txEnablePin, OUTPUT);
	digitalWrite(txEnablePin, LOW);
	frameComplete = 0;
	lastReceivedTime = 0;
	minIdleTime = DIFS_CONSTANT;  // changes in setOwnAddress
	// upper layer
    deviceType = _devicetype;
    readAddressFromEEPROM();
	hbwdebugstream = _debugstream;    // debug stream, might be NULL
	configPin = 0xFF;  //inactive by default
	ledPin = 0xFF;     // inactive by default
	useAnalogConfigPin = false;		// use digital ConfigPin by default
	// read config
	readConfig();
	afterReadConfigPending = true; // force read config after startup //TODO test afterReadConfig
}
  

 void HBWDevice::setConfigPins(uint8_t _configPin, uint8_t _ledPin, boolean _useAnalogConfigPin) {
	configPin = _configPin;
	useAnalogConfigPin = _useAnalogConfigPin;
	if(configPin != 0xFF) {
		if (useAnalogConfigPin)	// no pullup for analog input
			pinMode(configPin,INPUT);
		else
			pinMode(configPin,INPUT_PULLUP);
	}
    ledPin = _ledPin;	
	if(ledPin != 0xFF) pinMode(ledPin,OUTPUT);
 };
  
  
void HBWDevice::set(uint8_t channel,uint8_t length,uint8_t const * const data){
	if(hbwdebugstream) {
	    hbwdebug(F("S: ")); hbwdebughex(channel); hbwdebug(F(" "));
		for(uint8_t i = 0; i < length; i++)
            hbwdebughex(data[i]); 
		hbwdebug("\n");
	};
	// to avoid crashes, do not try to set any channels, which do not exist
	if(channel < numChannels)
        channels[channel]->set(this, length, data);
}


uint8_t HBWDevice::get(uint8_t channel, uint8_t* data) {  // returns length
    // to avoid crashes, return 0 for channels, which do not exist
	if(channel >= numChannels) {
		data[0] = 0;
		return 1;
	}
    return channels[channel]->get(data);
}


// The loop function is called in an endless loop
void HBWDevice::loop()
{
  // read device and channel config, on init and if triggered by ReadConfig() // TODO test afterReadConfig
   if (afterReadConfigPending) {
		afterReadConfig();
		for(uint8_t i = 0; i < numChannels; i++) {
			if (afterReadConfigPending)
				channels[i]->afterReadConfig();  // TODO test afterReadConfig
		}
		afterReadConfigPending = false;
	}
// Daten empfangen und alles, was zur Kommunikationsschicht gehört
// processEvent vom Modul wird als Callback aufgerufen
// Daten empfangen (tut nichts, wenn keine Daten vorhanden)
  receive();
  // Check
  if(frameComplete) {
	frameComplete = 0;   // only once
    if(targetAddress == ownAddress || targetAddress == 0xFFFFFFFF){
	  if(parseFrame()) {
	    processEvent(frameData, frameDataLength, (targetAddress == 0xFFFFFFFF));
	  };	
	}
  };
  // send announce message, if not done yet
  handleBroadcastAnnounce();
// feedback from switches and handle keys
   for(uint8_t i = 0; i < numChannels; i++) {
        channels[i]->loop(this,i);
   }
// config Button
   handleConfigButton();
};


// get logging time	
uint8_t HBWDevice::getLoggingTime() {
	return config[0];
}


void HBWDevice::afterReadConfig() {};



void HBWDevice::factoryReset() {
  // writes FF into EEPROM, without upper 4 Bytes
  for(uint16_t i = 0; i <= E2END - 4; i++) {
    writeEEPROM(i, 0xFF, false);
  };
  // re-read config. This includes setting defaults etc.
  readConfig();
}


void HBWDevice::handleConfigButton() {
  // langer Tastendruck (5s) -> LED blinkt hektisch
  // dann innerhalb 10s langer Tastendruck (3s) -> LED geht aus, EEPROM-Reset

  // do we have a config-pin?
  if(configPin == 0xFF) return;
  
  static long lastTime = 0;
  static uint8_t status = 0;  // 0: normal, 1: Taste erstes mal gedrueckt, 2: erster langer Druck erkannt
                           // 3: Warte auf zweiten Tastendruck, 4: Taste zweites Mal gedrueckt
                           // 5: zweiter langer Druck erkannt

  long now = millis();
  boolean buttonState;

  if (useAnalogConfigPin) {
	buttonState = false;
	if (analogRead(configPin) > 800) { // Button press @ 10k / 100k voltage divider (@5V == 4.5V and @3.3V == 3V --> 930 AD value)
		buttonState = true;
	}
  }
  else
    buttonState = !digitalRead(configPin);


  switch(status) {
    case 0:
      if(buttonState) status = 1;
      lastTime = now;
      break;
    case 1:
      if(buttonState) {   // immer noch gedrueckt
          if(now - lastTime > 5000) status = 2;
      }else{              // nicht mehr gedrueckt
          if(now - lastTime > 100)   // announce on short press
              broadcastAnnounce(0);
        status = 0;
      };
      break;
    case 2:
      if(!buttonState) {  // losgelassen
          status = 3;
          lastTime = now;
      };
      break;
    case 3:
      // wait at least 100ms
      if(now - lastTime < 100)
          break;
      if(buttonState) {   // zweiter Tastendruck
          status = 4;
          lastTime = now;
      }else{              // noch nicht gedrueckt
          if(now - lastTime > 10000) status = 0;   // give up
      };
      break;
    case 4:
      if(now - lastTime < 100) // entprellen
            break;
      if(buttonState) {   // immer noch gedrueckt
          if(now - lastTime > 3000) status = 5;
      }else{              // nicht mehr gedrueckt
          status = 0;
      };
      break;
    case 5:   // zweiter Druck erkannt
      if(!buttonState) {    //erst wenn losgelassen
          // Factory-Reset          
          factoryReset();
          status = 0;
      }
      break;
  }

  // control LED, if set  
  if(ledPin == 0xFF) return;
  static long lastLEDtime = 0;
  switch(status) {
    case 0:
      digitalWrite(ledPin, LOW);
      break;
    case 1:
      digitalWrite(ledPin, HIGH);
      break;
    case 2:
    case 3:
    case 4:
      if(now - lastLEDtime > 100) {  // schnelles Blinken
          digitalWrite(ledPin,!digitalRead(ledPin));
          lastLEDtime = now;
      };
      break;
    case 5:
      if(now - lastLEDtime > 750) {  // langsames Blinken
          digitalWrite(ledPin,!digitalRead(ledPin));
          lastLEDtime = now;
      };
  }
};


/****************************************
** DEBUG
****************************************/
void hbwdebughex(uint8_t b) {
   if(!hbwdebugstream) return;
   hbwdebugstream->print(b >> 4, HEX);
   hbwdebugstream->print(b & 15, HEX);
};










