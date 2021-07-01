/*
 * HBWired.cpp
 *
 *  Created on: 19.11.2016
 *      Author: Thorsten Pferdekaemper thorsten@pferdekaemper.com
 *      Nach einer Vorlage von Dirk Hoffmann (dirk@hfma.de) 2012
 *
 *  HomeBrew-Wired RS485-Protokoll 
 *
 * Last updated: 21.01.2019
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
#define FRAME_STARTBYTE_SHORT 0xFE
#define CRC16_POLYNOM 0x1002
#define ESCAPE_CHAR 0xFC

// Empfangs-Status
#define FRAME_START 1    // Startzeichen gefunden
#define FRAME_ESCAPE 2   // Escape-Zeichen gefunden
#define FRAME_SENTACKWAIT 8  // Nachricht wurde gesendet, warte auf ACK

// Statics
HBWDevice::s_PendingActions HBWDevice::pendingActions;
uint8_t HBWDevice::configButtonStatus;

// Methods

// eigene Adresse setzen und damit auch random seed
void HBWDevice::setOwnAddress(uint32_t address) {
  ownAddress = address;
  randomSeed(ownAddress);
  minIdleTime = random(DIFS_CONSTANT, DIFS_CONSTANT+DIFS_RANDOM);
  pendingActions.announced = false;	// (re)send broadcast announce message
}


uint32_t HBWDevice::getOwnAddress() {
	return ownAddress;
}


boolean HBWDevice::parseFrame () { // returns true, if event needs to be processed by the module
// Wird aufgerufen, wenn eine komplette Nachricht empfangen wurde

  byte seqNumReceived;
  byte seqNumSent;
  
  rxLEDStatus = true;

//### START ACK verarbeiten ###
  if(frameStatus & FRAME_SENTACKWAIT) {   // wir warten auf ACK?
    // Empfangene Sendefolgenummer muss stimmen
    seqNumReceived = (frameControlByte >> 5) & 0x03;
    seqNumSent     = (txFrame.controlByte >> 1) & 0x03;
    // Absenderadresse mus stimmen
    if(seqNumReceived == seqNumSent && senderAddress == txFrame.targetAddress) {
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
    txFrame.targetAddress = senderAddress;
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
byte HBWDevice::sendFrame(boolean onlyIfIdle, uint8_t retries){
// TODO: non-blocking
// TODO: Wenn als Antwort kein reines ACK kommt, dann geht die Antwort verloren
//       D.h. sie wird nicht interpretiert. Die Gegenstelle sollte es dann nochmal
//       senden, aber das ist haesslich (andererseits scheint das nicht zu passieren)

// carrier sense
   if(onlyIfIdle) {
	 if(!busIsIdle())
		 return BUS_BUSY;
	 // set new idle time
	 minIdleTime = random(DIFS_CONSTANT, DIFS_CONSTANT+DIFS_RANDOM);
   }
   
   txLEDStatus = true;

// simple send for ACKs and Broadcasts
  if(txFrame.targetAddress == 0xFFFFFFFF || ((txFrame.controlByte & 0x03) == 1)) {
	sendFrameSingle();
	// TODO: nicht besonders schoen, zuerst ackwait zu setzen und dann wieder zu loeschen
	frameStatus &= ~FRAME_SENTACKWAIT; // we do not really wait
	return SUCCESS;  // we do not wait for an ACK, i.e. always ok
  };

  uint32_t lastTry = 0;

  for(byte i = 0; i < retries; i++) {  // default, maximal 3 Versuche
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
          frameComplete = false;
          parseFrame();
          if(!(frameStatus & FRAME_SENTACKWAIT))  // ACK empfangen
            return SUCCESS;  // we have an ACK, i.e. ok
        };
// TODO: (loetmeister) add "else", in case a frame was received (broadcast or other?), but not for us - stop sending and return BUS_BUSY? (use as collision detection in linkSender)
      };
    };
    // We have not received an ACK. However, there might be
    // other stuff on the bus and we might better keep quiet
    // TODO: random part?
    if(onlyIfIdle && millis() - lastTry < RETRYIDLETIME)
        return BUS_BUSY;	// bus is not really free
  };
  return NO_ACK;  // three times (i.e. value of 'retries') without ACK
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
      if(txFrame.targetAddress != 0xFFFFFFFF){
        byte txSeqNum = (frameControlByte >> 1) & 0x03;
        txFrame.controlByte &= 0x9F;
        txFrame.controlByte |= (txSeqNum << 5);
      };

      hbwdebug(F("T: ")); hbwdebughex(FRAME_STARTBYTE);
      digitalWrite(txEnablePin, HIGH);
      
      serial->write(FRAME_STARTBYTE);  // send startbyte
      crc16Shift(FRAME_STARTBYTE , &crc16checksum);

      byte i;
      uint32_t address = txFrame.targetAddress;
      for( i = 0; i < 4; i++){      // send target address
    	 tmpByte = address >> 24;
         sendFrameByte( tmpByte, &crc16checksum);
         address = address << 8;
      };

      sendFrameByte(txFrame.controlByte, &crc16checksum);

      if(bitRead(txFrame.controlByte,3)){                                      // check if message has sender
    	  address = ownAddress;
    	  for( i = 0; i < 4; i++){                                           // send sender address
    	    	 tmpByte = address >> 24;
    	         sendFrameByte( tmpByte, &crc16checksum);
    	         address = address << 8;
    	  }
      };
      tmpByte = txFrame.dataLength + 2;                              // send data length
      sendFrameByte(tmpByte, &crc16checksum);

      for(i = 0; i < txFrame.dataLength; i++){            // send data, falls was zu senden
          sendFrameByte(txFrame.data[i], &crc16checksum);
      }
      crc16Shift(0 , &crc16checksum);
      crc16Shift(0 , &crc16checksum);

      // CRC schicken
      sendFrameByte(crc16checksum / 0x100);
      sendFrameByte(crc16checksum & 0xFF);

      serial->flush();                  // otherwise, enable pin will go low too soon
      digitalWrite(txEnablePin, LOW);

      frameStatus |= FRAME_SENTACKWAIT;
      hbwdebug(F("\n"));
} // sendFrameSingle


// Send a data byte.
// Before sending check byte for special chars. Special chars are escaped before sending
// TX-Pin needs to be HIGH before calling this
void HBWDevice::sendFrameByte(byte sendByte, uint16_t* checksum) {
  // Debug
  hbwdebug(":"); hbwdebughex(sendByte);
  // calculate checksum, if needed
  if(checksum)
  crc16Shift(sendByte, checksum);
    // Add escape character, if needed
    if(sendByte == FRAME_STARTBYTE || sendByte == FRAME_STARTBYTE_SHORT || sendByte == ESCAPE_CHAR) {
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
      txFrame.controlByte = 0x19;
      txFrame.dataLength = 0;
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
   #ifdef HBW_DEBUG
    if( rxByte == FRAME_STARTBYTE ){
		hbwdebug(F("R: "));
	}else{
		hbwdebug(F(":"));
	}	
    hbwdebughex(rxByte);
   #endif

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
         }else if(addressPointer != 0xFF) { // Datenl�nge empfangen
            addressPointer = 0xFF;
            frameDataLength = rxByte;
            if(frameDataLength > MAX_RX_FRAME_LENGTH) // Maximale Pufferg��e checken.
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
                  // Framedaten f�r die sp�tere Verarbeitung speichern
                  // TODO: Braucht man das wirklich?
                  frameControlByte = rxFrameControlByte;
                  frameDataLength -= 2;
                  // es liegt eine neue Nachricht vor
                  frameComplete = true;
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
#ifdef Support_HBWLink_InfoEvent
void HBWChannel::setInfo(HBWDevice* device, uint8_t length, uint8_t const * const data) {};
#endif
void HBWChannel::set(HBWDevice* device, uint8_t length, uint8_t const * const data) {};
uint8_t HBWChannel::get(uint8_t* data) { return 0; };   
void HBWChannel::setLock(boolean inhibit) { inhibitActive = inhibit; };
boolean HBWChannel::getLock() { return inhibitActive; };
void HBWChannel::loop(HBWDevice* device, uint8_t channel) {};    
void HBWChannel::afterReadConfig() {};

// default logging/feedback functions
void HBWChannel::setFeedback(HBWDevice* device, boolean loggingEnabled) {
  if (!nextFeedbackDelay && loggingEnabled) {
    lastFeedbackTime = millis();
    nextFeedbackDelay = device->getLoggingTime() * 100;
  }
};
void HBWChannel::checkFeedback(HBWDevice* device, uint8_t channel) {
  if(!nextFeedbackDelay)  // feedback trigger set?
    return;
  if (millis() - lastFeedbackTime < nextFeedbackDelay)
    return;
  lastFeedbackTime = millis();  // at least last time of trying
  uint8_t data[7];  // TODO: set meaningfull value for data lenght (usual values are 2 bytes. 16 bit value or 8 bit value + 8 bit state_flags)
  uint8_t data_len = get(data);
  // sendInfoMessage returns 0 on success, 1 if bus busy, 2 if failed
  uint8_t resultcode = device->sendInfoMessage(channel, data_len, data);   
  if (resultcode == HBWDevice::BUS_BUSY)  // bus busy
  // try again later, but insert a small delay
    nextFeedbackDelay = 250;
  else
    nextFeedbackDelay = 0;
};
void HBWChannel::clearFeedback() { nextFeedbackDelay = 0; };



void HBWLinkSender::sendKeyEvent(HBWDevice* device, uint8_t srcChan, uint8_t keyPressNum, boolean longPress) {};
void HBWLinkReceiver::receiveKeyEvent(HBWDevice* device, uint32_t senderAddress, uint8_t senderChannel, 
                                      uint8_t targetChannel, uint8_t keyPressNum, boolean longPress) {};
#ifdef Support_HBWLink_InfoEvent
void HBWLinkSender::sendInfoEvent(HBWDevice* device, uint8_t srcChan, uint8_t length, uint8_t const * const data) {};
void HBWLinkReceiver::receiveInfoEvent(HBWDevice* device, uint32_t senderAddress, uint8_t senderChannel,
                                       uint8_t targetChannel, uint8_t length, uint8_t const * const data) {};
#endif


// Processing of default events (related to all modules)
void HBWDevice::processEvent(byte const * const frameData, byte frameDataLength, boolean isBroadcast) {

      uint16_t adrStart;
      boolean onlyAck = true;

      // ACKs kommen hier nicht an, werden eine Schicht tiefer abgefangen

      // Wenn irgendwas von Broadcast kommt, dann gibt es darauf keine
      // Reaktion, ausser z und Z (und es kommt von der Zentrale)
      // TODO: Muessen wir pruefen, ob's wirklich von der Zentrale kommt?
      if(isBroadcast) {
         switch(frameData[0]) {
          case 'Z':                                            // end discovery mode
            pendingActions.zeroCommunicationActive = false;
            break;
          case 'z':                                              // start discovery mode
            pendingActions.zeroCommunicationActive = true;
            break;
          // case 'K':  // 0x4B Key-Event
            // broadcast key events sind f�r long_press interressant
            //  if (frameDataLength == 4) {...}
			// if (frameData[3] & 0x01) {    // long press broadcast only
              // receiveKeyEvent(senderAddress, frameData[1], frameData[2], frameData[3] >>2, true);
			// }
            // break;
        }
        return;
      };

      if (pendingActions.zeroCommunicationActive) {				// block any messages in this state, except:
      #if defined(_HAS_BOOTLOADER_) && defined(BOOTSTART)
         switch(frameData[0]){
            case 'u':                                                              // Update (Bootloader starten)
               goto *bootloader_start;			// Adresse des Bootloaders
               break;
         }
      #endif
         return;
      };

      txFrame.targetAddress = senderAddress;
      // gibt es was zu verarbeiten -> Ja, die Kommunikationsschicht laesst nur Messages durch,
      // die auch Daten enthalten

      txFrame.controlByte = 0x78;

      switch(frameData[0]){
         case '@':             // 0x40                 // HBW-specifics
           if(frameData[1] == 'a' && frameDataLength == 6) {  // 0x61 "a" -> address set
        	 hbwdebug(F("@: Set Address\n"));
        	 // Avoid "central" addresses (like 0000... 000FF)
        	 if (frameData[2] == 0 && frameData[3] == 0 && frameData[4] == 0) {
        	 	 hbwdebug(F("@: rejected\n"));
        	 	 break;
        	 }
        	 for(byte i = 0; i < 4; i++)
        	   writeEEPROM(E2END - 3 + i, frameData[i + 2], true);
           };
           readAddressFromEEPROM();
           break;
         #if defined (Support_ModuleReset)
         case '!':                                                             // reset the Module
            // reset the Module jump after the bootloader
        	// Nur wenn das zweite Zeichen auch ein "!" ist
            if(frameData[1] == '!') { pendingActions.resetSystem = true; };  // don't reset immediately, send ACK first
            break;
         #endif
         case 'A':                                                             // Announce
        	txFrame.data[0] = 'i';
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
         case 0xCB:   // '�':       // Key-Sim-Event TODO: Es gibt da einen theoretischen Unterschied
            receiveKeyEvent(senderAddress, frameData[1], frameData[2], frameData[3] >>2, frameData[3] & 0x01);
            break;
         case 'R':                                                              // Read EEPROM
        	// TODO: Check requested length...
            if(frameDataLength == 4) {                                // Length of incoming data must be 4
               onlyAck = false;
               hbwdebug(F("C: Read EEPROM\n"));
               adrStart = ((uint16_t)(frameData[1]) << 8) | frameData[2];  // start adress of eeprom
               for(byte i = 0; i < frameData[3]; i++) {
            	   txFrame.data[i] = EEPROM.read(adrStart + i);
               };
               txFrame.dataLength = frameData[3];
            };
            break;
         case 'S':                                                               // GET Level
            processEventGetLevel(frameData[1], frameData[0]);
            txFrame.data[0] = 'i';
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
         /* case 'c':                                                               // Zieladresse l�schen?
            // TODO: ???
            break;  */
       #ifdef Support_HBWLink_InfoEvent
         case 0xB4:                                                               // received 'Info Event' (peering with data) - custom HomeBrew
            if (frameDataLength > 3)
              receiveInfoEvent(senderAddress, frameData[1], frameData[2], frameDataLength-3, &(frameData[3]));
            break;
       #endif
         case 'h':                                 // 0x68 get Module type and hardware version
            hbwdebug(F("T: HWVer,Typ\n"));
            onlyAck = false;
            txFrame.data[0] = deviceType;
            txFrame.data[1] = hardware_version;
            txFrame.dataLength = 2;
            break;
         case 'l':
            processEventSetLock(frameData[2], frameData[3] & 0x01);
            break;
         case 'n':                                       // Seriennummer
        	determineSerial(txFrame.data, getOwnAddress());
        	txFrame.dataLength = 10;
        	onlyAck = false;
        	break;
         /* case 'q':                                                               // Zieladresse hinzuf�gen?
            // TODO: ???
        	break; */
         case 'v':                                                               // get firmware version
            hbwdebug(F("T: FWVer\n")); 
            onlyAck = false;
            txFrame.data[0] = firmware_version / 0x100;
            txFrame.data[1] = firmware_version & 0xFF;
            txFrame.dataLength = 2;
            break;
         case 's':   // level set
         case 'x':   // Level set. In der Protokollbeschreibung steht hier was von "install test",
                     // aber es sieht so aus, als ob 0x73 und 0x78 dasselbe tun
            set(frameData[1], frameDataLength-2, &(frameData[2]));
            // get what the hardware did and send it back
			// TODO: Is this really what the standard modules do?
			//       For slow modules, this might not be correct 
            processEventGetLevel(frameData[1], frameData[0]);  // for feedback
            txFrame.data[0] = 'i';
            onlyAck = false; 
            break;
      }

      if(onlyAck) {
         sendAck();
      }
      else {
         sendFrame();
      };
};


#ifdef Support_HBWLink_InfoEvent	
// custom (info) message as linkReceiver
void HBWDevice::receiveInfoEvent(uint32_t senderAddress, uint8_t srcChan, 
                                uint8_t dstChan, uint8_t length, uint8_t const * const data) {
    if(linkReceiver)
        linkReceiver->receiveInfoEvent(this, senderAddress, srcChan, dstChan, length, data);
};
#endif


// Enhanced peering, needs more data
void HBWDevice::receiveKeyEvent(uint32_t senderAddress, uint8_t srcChan, 
                                uint8_t dstChan, uint8_t keyPressNum, boolean longPress) {
    if(dstChan >= numChannels)  return;  // don't inhibit/lock or waste time search peerings for non exiting (target) channels
    if(channels[dstChan]->getLock())  return;
    if(linkReceiver)
        linkReceiver->receiveKeyEvent(this, senderAddress, srcChan, dstChan, keyPressNum, longPress);
};


void HBWDevice::processEventGetLevel(byte channel, byte command){
   // get value from the hardware and send it back
   txFrame.data[0] = 0x69;         // 'i'
   txFrame.data[1] = channel;      // Sensornummer
   uint8_t length = get(channel, &(txFrame.data[2]));
   txFrame.dataLength = 0x02 + length;      // Length
};


void HBWDevice::processEventSetLock(uint8_t channel, boolean inhibit){
   // lock (inihibit) a channel (disables all peerings to that channel)
   // to avoid crashes, do not try to set any channels, which do not exist
   if(channel < numChannels) {
       channels[channel]->setLock(inhibit);
       #ifdef HBW_DEBUG
         hbwdebug(F("ch: "));hbwdebug(channel); hbwdebug(F("set inhibit: ")); inhibit ? hbwdebug(F("TRUE\n")) : hbwdebug(F("FALSE\n"));
       #endif
   }
};


void HBWDevice::processEmessage(uint8_t const * const frameData) {
   // process E-Message
   
   uint8_t blocksize = frameData[3];
   uint8_t blocknum  = frameData[4];
   
   // length of response
   txFrame.dataLength = 4 + blocknum / 8;
   // care for odd block numbers
   if(blocknum % 8) txFrame.dataLength++;
   // we don't need to check the size as it can maximum
   // be 4 + 255 div 8 + 1 = 36
   // init to zero, mainly because we need it later
   memset(txFrame.data,0,txFrame.dataLength);
   // first byte "e" - answer on "E"
   txFrame.data[0]  = 0x65;  //e
   // next 3 bytes are just repeated from request
   txFrame.data[1]  = frameData[1];
   txFrame.data[2]  = frameData[2];
   txFrame.data[3]  = frameData[3];
   
   // determine whether blocks are used
   for(int block = 0; block <= blocknum; block++) {
      // check this memory block
      for(int byteIdx = 0; byteIdx < blocksize; byteIdx++) {
         if(EEPROM.read(block * blocksize + byteIdx) != 0xFF) {
            bitSet(txFrame.data[4 + block / 8], block % 8);
            break;
         }
      }
   };
};


// "Announce-Message" ueber broadcast senden
byte HBWDevice::broadcastAnnounce(byte channel) {
   txFrame.targetAddress = 0xFFFFFFFF;  // broadcast
   txFrame.controlByte = 0xF8;     // control byte
   txFrame.dataLength = 16;      // Length
   txFrame.data[0] = 0x41;         // 'A'
   txFrame.data[1] = channel;      // Sensornummer
   txFrame.data[2] = deviceType;
   txFrame.data[3] = hardware_version;
   txFrame.data[4] = firmware_version / 0x100;
   txFrame.data[5] = firmware_version & 0xFF;
   determineSerial(txFrame.data + 6, getOwnAddress());
   // only send, if bus is free. Don't send in zeroCommunication mode, return with "bus busy" instead
   return (pendingActions.zeroCommunicationActive ? BUS_BUSY : sendFrame(true));
};


// "i-Message" senden
// this is only called from "outside" and not as a response
uint8_t HBWDevice::sendInfoMessage(uint8_t channel, uint8_t length, uint8_t const * const data, uint32_t target_address) {
   if (pendingActions.zeroCommunicationActive) return BUS_BUSY;	// don't send in zeroCommunication mode, return with "bus busy" instead
   txFrame.targetAddress = target_address;
   if(!txFrame.targetAddress) txFrame.targetAddress = getCentralAddress();	
   txFrame.controlByte = 0xF8;     // control byte
   txFrame.dataLength = 0x02 + length;      // Length
   txFrame.data[0] = 0x69;         // 'i'
   txFrame.data[1] = channel;      // Sensornummer
   memcpy(&(txFrame.data[2]), data, length);
   return sendFrame(true);  // only if bus is free
};


#ifdef Support_HBWLink_InfoEvent
// link "InfoEvent Message" senden
uint8_t HBWDevice::sendInfoEvent(uint8_t channel, uint8_t length, uint8_t const * const data, uint32_t target_address, uint8_t target_channel, boolean busState, uint8_t retries) {
   if (pendingActions.zeroCommunicationActive) return BUS_BUSY;	// don't send in zeroCommunication mode, return with "bus busy" instead
   txFrame.targetAddress = target_address;
   txFrame.controlByte = 0xF8;          // control byte
   txFrame.dataLength = 0x03 + length;  // Length
   txFrame.data[0] = 0xB4;              // custom frame/command (mix of 'i' and 'K')
   txFrame.data[1] = channel;           // Sensornummer
   txFrame.data[2] = target_channel;    // Zielaktor
   memcpy(&(txFrame.data[3]), data, length);
   return sendFrame(busState, retries);  // only if bus is free
};

// InfoEvent senden, inklusive peers etc.
uint8_t HBWDevice::sendInfoEvent(uint8_t srcChan, uint8_t length, uint8_t const * const data, boolean busState) {
  if(linkSender) {
    if ((busIsIdle() && busState == NEED_IDLE_BUS ) || busState != NEED_IDLE_BUS) {
      linkSender->sendInfoEvent(this, srcChan, length, data);
    }
    else {
      return BUS_BUSY;
    }
  }
  return SUCCESS;   //TODO: maybe linkSender->sendInfoEvent() should not be void...? if it would return sendFrame status, "no ACK" result must be ignored? (dead peering)
};
#endif


// key-Event senden, inklusive peers etc.
uint8_t HBWDevice::sendKeyEvent(uint8_t srcChan, uint8_t keyPressNum, boolean longPress) {
  uint8_t result = sendKeyEvent(srcChan, keyPressNum, longPress, 0xFFFFFFFF, 0, NEED_IDLE_BUS);  // only if bus is free
  if(linkSender && result == SUCCESS)
    // send all peer events, don't retry, don't wait for idle bus (initial broadcast was send only on idle/free bus)
    // TODO: if linkSender->sendKeyEvent would return sendFrame status, "no ACK" result must be ignored? (dead peering)
    linkSender->sendKeyEvent(this, srcChan, keyPressNum, longPress);
  return result;
};


// key-Event senden an bestimmtes Target
uint8_t HBWDevice::sendKeyEvent(uint8_t srcChan, uint8_t keyPressNum, boolean longPress, uint32_t targetAddr, uint8_t targetChan, boolean busState, uint8_t retries) {
   if (pendingActions.zeroCommunicationActive) return BUS_BUSY;	// don't send in zeroCommunication mode, return with "bus busy" instead
   txFrame.targetAddress = targetAddr;  // target address
   txFrame.controlByte = 0xF8;     // control byte
   txFrame.dataLength = 0x04;      // Length
   txFrame.data[0] = 0x4B;         // 'K'
   txFrame.data[1] = srcChan;      // Sensornummer
   txFrame.data[2] = targetChan;   // Zielaktor
   txFrame.data[3] = (longPress ? 3 : 2) + (keyPressNum << 2);
   return sendFrame(busState, retries);  // only if bus is free, default true. Retries, default 3
};


// Key-Event senden mit Geraetespezifischen Daten (nur Broadcast)
uint8_t HBWDevice::sendKeyEvent(uint8_t srcChan, uint8_t length, void* data) {
   if (pendingActions.zeroCommunicationActive) return 1;	// don't send in zeroCommunication mode, return with "bus busy" instead
   txFrame.targetAddress = 0xFFFFFFFF;  // target address
   txFrame.controlByte = 0xF8;     // control byte
   txFrame.dataLength = 3 + length;      // Length
   txFrame.data[0] = 0x4B;         // 'K'
   txFrame.data[1] = srcChan;      // Sensornummer
   txFrame.data[2] = 0;   // Zielaktor
   memcpy(&(txFrame.data[3]), data, length);
   return sendFrame(true);  // only if bus is free
};


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


void HBWDevice::determineSerial(uint8_t* buf, uint32_t address) {

   buf[0] = 'H';
   buf[1] = 'B';
   buf[2] = 'W';
   
   // append last 7 digits of own address
   uint8_t* pEnd = &buf[9];
   while (pEnd != &buf[2])
   {
      *pEnd-- = '0' + (address % 10);  // store as string
      if (address)
      {
         address /= 10;
      }
   }
};


void HBWDevice::readConfig() {         // read config from EEPROM	
   // read EEPROM
   readEEPROM(config, 0x01, configSize);
   // turn around central address
   uint32_t addr = *((uint32_t*)(config + 1));
   for(uint8_t i = 0; i < 4; i++)
     config[i+1] = ((uint8_t*)(&addr))[3-i];
   // set defaults if values not provided from EEPROM or other device specific stuff
   pendingActions.afterReadConfig = true; // tell main loop to run afterReadConfig() for device and channels
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
   if(pendingActions.announced) return;
   // avoid sending broadcast in the first second
   if(millis() < 1000) return;
   // send methods return 0 if everything is ok
   pendingActions.announced = (broadcastAnnounce(0) == SUCCESS);
}


// return true, if bus was idle for minIdleTime (means nothing was received on the device)
boolean HBWDevice::busIsIdle()
{
  return (millis() - lastReceivedTime > minIdleTime);
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
   frameComplete = false;
   lastReceivedTime = 0;
   minIdleTime = DIFS_CONSTANT;  // changes in setOwnAddress
   ledPin = NOT_A_PIN;     // inactive by default
   txLedPin = NOT_A_PIN;     // inactive by default
   rxLedPin = NOT_A_PIN;     // inactive by default
   // upper layer
   deviceType = _devicetype;
   readAddressFromEEPROM();
   hbwdebugstream = _debugstream;    // debug stream, might be NULL
   configPin = NOT_A_PIN;  //inactive by default
   configButtonStatus = 0;
   readConfig();	// read config
   pendingActions.zeroCommunicationActive = false;	// will be activated by START_ZERO_COMMUNICATION = 'z' command
   #ifdef Support_ModuleReset
   pendingActions.resetSystem = false;
   #endif
   #ifdef Support_WDT
   wdt_enable(WDTO_1S);
   #endif
}
  

void HBWDevice::setConfigPins(uint8_t _configPin, uint8_t _ledPin) {
	configPin = _configPin;
	if(configPin != NOT_A_PIN) {
	#if defined(__AVR_ATmega328P__) || defined(__AVR_ATmega328__)
		if (configPin == A6 || configPin == A7)
			pinMode(configPin,INPUT);	// no pullup for analog input
		else
	#endif
		pinMode(configPin,INPUT_PULLUP);
	}
	ledPin = _ledPin;	
	if(ledPin != NOT_A_PIN) pinMode(ledPin,OUTPUT);
};


void HBWDevice::setStatusLEDPins(uint8_t _txLedPin, uint8_t _rxLedPin) {
	txLedPin = _txLedPin;
	rxLedPin = _rxLedPin;
	if(txLedPin != NOT_A_PIN) pinMode(txLedPin, OUTPUT);
	if(rxLedPin != NOT_A_PIN) pinMode(rxLedPin, OUTPUT);
};

#ifdef Support_HBWLink_InfoEvent
void HBWDevice::setInfo(uint8_t channel, uint8_t length, uint8_t const * const data) {
	#ifdef HBW_DEBUG
	    hbwdebug(F("Si: ")); hbwdebughex(channel); hbwdebug(F(" "));
		for(uint8_t i = 0; i < length; i++)
			hbwdebughex(data[i]); 
		hbwdebug("\n");
	#endif
	// to avoid crashes, do not try to set any channels, which do not exist
	if(channel < numChannels)
        channels[channel]->setInfo(this, length, data);
}
#endif

void HBWDevice::set(uint8_t channel, uint8_t length, uint8_t const * const data) {
	#ifdef HBW_DEBUG
	    hbwdebug(F("S: ")); hbwdebughex(channel); hbwdebug(F(" "));
		for(uint8_t i = 0; i < length; i++)
			hbwdebughex(data[i]); 
		hbwdebug("\n");
	#endif
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
  if (pendingActions.resetSystem) {
   #if defined (Support_ModuleReset)
    #if defined (Support_WDT)
    // wdt_wdt_enable(WDTO_15MS);
    while(1){}  // if watchdog is used & active, just run into infinite loop to force reset
    #else
    resetSoftware();  // otherwise jump to reset vector
    #endif
   #endif
  }
  // read device and channel config, on init and if triggered by ReadConfig()
  if (pendingActions.afterReadConfig) {
    afterReadConfig();
    for(uint8_t i = 0; i < numChannels; i++) {
      channels[i]->afterReadConfig();
    }
    pendingActions.afterReadConfig = false;
  }
// Daten empfangen und alles, was zur Kommunikationsschicht geh�rt
// processEvent vom Modul wird als Callback aufgerufen
// Daten empfangen (tut nichts, wenn keine Daten vorhanden)
  receive();
  // Check
  if(frameComplete) {
	frameComplete = false;   // only once
	if(targetAddress == ownAddress || targetAddress == 0xFFFFFFFF){
	  if(parseFrame()) {
	    processEvent(frameData, frameDataLength, (targetAddress == 0xFFFFFFFF));
	  };	
	}
  };
  // send announce message, if not done yet
  handleBroadcastAnnounce();
// feedback from individual channels (like switches and keys)
   static uint8_t loopCurrentChannel = 0;
   channels[loopCurrentChannel]->loop(this, loopCurrentChannel);
   loopCurrentChannel++;
   if (loopCurrentChannel >= numChannels) loopCurrentChannel = 0;
// config Button
   handleConfigButton();
// Rx & Tx LEDs
   handleStatusLEDs();
   #ifdef Support_WDT
   wdt_reset();
   #endif
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
  if(configPin == NOT_A_PIN) return;
  
  static long lastTime = 0;
  long now = millis();
  boolean buttonState;

#if defined(__AVR_ATmega328P__) || defined(__AVR_ATmega328__)
  if (configPin == A6 || configPin == A7) {
    buttonState = false;
    if (analogRead(configPin) < 250) // Button to ground with ~100k pullup
      buttonState = true;
  }
  else
#endif
    buttonState = !digitalRead(configPin);
	
  // configButtonStatus
  // 0: normal, 1: Taste erstes mal gedrueckt, 2: erster langer Druck erkannt
  // 3: Warte auf zweiten Tastendruck, 4: Taste zweites Mal gedrueckt
  // 5: zweiter langer Druck erkannt
  switch(configButtonStatus) {
    case 0:
      if(buttonState) configButtonStatus = 1;
      lastTime = now;
      break;
    case 1:
      if(buttonState) {   // immer noch gedrueckt
          if(now - lastTime > 5000) configButtonStatus = 2;
      }else{              // nicht mehr gedrueckt
          if(now - lastTime > 100)   // announce on short press
              pendingActions.announced = false;
        configButtonStatus = 0;
      };
      break;
    case 2:
      if(!buttonState) {  // losgelassen
          configButtonStatus = 3;
          lastTime = now;
      };
      break;
    case 3:
      // wait at least 100ms
      if(now - lastTime < 100)
          break;
      if(buttonState) {   // zweiter Tastendruck
          configButtonStatus = 4;
          lastTime = now;
      }else{              // noch nicht gedrueckt
          if(now - lastTime > 10000) configButtonStatus = 0;   // give up
      };
      break;
    case 4:
      if(now - lastTime < 100) // entprellen
            break;
      if(buttonState) {   // immer noch gedrueckt
          if(now - lastTime > 3000) configButtonStatus = 5;
      }else{              // nicht mehr gedrueckt
          configButtonStatus = 0;
      };
      break;
    case 5:   // zweiter Druck erkannt
      if(!buttonState) {    //erst wenn losgelassen
          // Factory-Reset          
          factoryReset();
          configButtonStatus = 0;
      };
      break;
  }

  // control LED, if set  
  if(ledPin == NOT_A_PIN) return;
  static long lastLEDtime = 0;
  if(now - lastLEDtime > 100) {  // update intervall & schnelles Blinken
	  switch(configButtonStatus) {
		case 0:
		  digitalWrite(ledPin, LOW);
		  lastLEDtime = now;
		  break;
		case 1:
		  digitalWrite(ledPin, HIGH);
		  lastLEDtime = now;
		  break;
		case 2:
		case 3:
		case 4:
		  digitalWrite(ledPin,!digitalRead(ledPin));
		  lastLEDtime = now;
		  break;
		case 5:
		  if(now - lastLEDtime > 750) {  // langsames Blinken
			  digitalWrite(ledPin,!digitalRead(ledPin));
			  lastLEDtime = now;
		  };
	  }
  }
};


void HBWDevice::handleStatusLEDs() {
	// turn on or off Tx, Rx LEDs - allow use of "config LED" for Tx, Rx combined
	// don't operate LED when configButton was pressed and "config LED" is used for Tx or Rx
	
	static long lastStatusLEDsTime = 0;
	
	if (millis() - lastStatusLEDsTime > 60) {	// check every 60 ms only (allow LED to light up approx 120 ms min.)
		if (txLedPin != NOT_A_PIN) {
			if (txLedPin == rxLedPin && rxLEDStatus) {	// combined Rx/Tx LED
				txLEDStatus = true;
				rxLEDStatus = false;
			}
			if ((configButtonStatus == 0 && txLedPin == ledPin) || txLedPin != ledPin) {	// configButton has priority for "config LED"
				digitalWrite(txLedPin, txLEDStatus);
				txLEDStatus = false;
			}
		}
		
		if (rxLedPin != NOT_A_PIN && txLedPin != rxLedPin) {		// combined Rx/Tx LED already handled above
			if ((configButtonStatus == 0 && rxLedPin == ledPin) || rxLedPin != ledPin) {
				digitalWrite(rxLedPin, rxLEDStatus);
				rxLEDStatus = false;
			}
		}
		lastStatusLEDsTime = millis();
	}
};


/****************************************
** DEBUG
****************************************/
#ifdef HBW_DEBUG
void hbwdebughex(uint8_t b) {
   if(!hbwdebugstream) return;
   hbwdebugstream->print(b >> 4, HEX);
   hbwdebugstream->print(b & 15, HEX);
};
#else
void hbwdebughex(uint8_t b) { };
#endif
