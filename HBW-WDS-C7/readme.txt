Homematic Wired Homebrew Wetterstation
======================================

Diese Modul stellt die Messwerte einer Bresser 7 in 1 oder 5 in 1 Wetterstation als Homematic Wired Gerät zur Verfügung.
Die Code-Basis ist ein SIGNALDuino: https://github.com/Ralf9/SIGNALDuino/tree/dev-r335_cc1101 mit cc1101 868 Mhz Modul.
Es kann parallel am RS485 Bus und USB betrieben werden. Die Funktion des SIGNALDuino (advanced) ist nicht eingeschränkt.
Entwickelt auf/für einen Raspberry Pi Pico. Kompiliert mit dem Arduino Boards: https://github.com/earlephilhower/arduino-pico


Damit FHEM das Homebrew-Device richtig erkennt, muss die Datei hbw_sen_ep.xml in den Ordner \FHEM\lib\HM485\Devices\xml kopiert werden (Das Device gibt sich als HW-Typ 0x88 aus).
Config der Kanäle kann über das FHEM-Webfrontend vorgenommen werden:
# ...

--- WORK IN PROGRESS ---- 

Pinbelegung:
0 - Rx Serial -> USB
1 - Tx Serial -> USB
2 - RS485 Enable
25 PIN_LED (LED_BUILTIN)
20   // gdo0Pin TX out PIN_SEND              
21   // gdo2 PIN_RECEIVE           
8, 9 (UART1) - RS485 bus
22 - Bedientaster (Reset)
I2C EEPROM
SDA = PIN_WIRE0_SDA;
SCL = PIN_WIRE0_SCL;
cc1011 module:
SS = PIN_SPI0_SS;
MOSI = PIN_SPI0_MOSI;
MISO = PIN_SPI0_MISO;
SCK = PIN_SPI0_SCK;

FHEM:
