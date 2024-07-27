Homematic Wired Homebrew Wetterstation
======================================

Dieses Modul stellt die Messwerte einer Bresser 7 in 1 [oder 5 in 1 - ToDo] Wetterstation als Homematic Wired Gerät zur Verfügung.
Die Code-Basis ist ein SIGNALDuino: https://github.com/Ralf9/SIGNALDuino/tree/dev-r335_cc1101 mit cc1101 868 MHz Modul.
Es kann parallel am RS-485 Bus und USB betrieben werden. Die Funktion des SIGNALDuino (advanced) ist nicht eingeschränkt.
Entwickelt auf/für einen Raspberry Pi Pico. Kompiliert mit dem Arduino Boards von earlephilhower: https://github.com/earlephilhower/arduino-pico


Damit FHEM das Homebrew-Device richtig erkennt, muss die Datei hbw_wds_c7.xml in den Ordner \FHEM\lib\HM485\Devices\xml kopiert werden (Das Device gibt sich als HW-Typ 0x88 aus).
Config der Kanäle kann über das FHEM-Webfrontend vorgenommen werden:
# HBWSIGNALDuino
- Bisher keine Konfig

# Weather
* SENSOR_ID
* STORM_THRESHOLD_LEVEL
* STORM_READINGS_TRIGGER
* SEND_DELTA_TEMP
* SEND_MIN_INTERVAL
* SEND_MAX_INTERVAL


--- WORK IN PROGRESS ---- 

Pinbelegung:
0 - Rx Serial -> USB
1 - Tx Serial -> USB
7 - RS485 Enable
25 PIN_LED (LED_BUILTIN) - SIGNALDuino
20 - gdo0Pin TX out PIN_SEND              
21 - gdo2 PIN_RECEIVE           
8, 9 (UART1) - RS485 bus
6 LED - HBW
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
