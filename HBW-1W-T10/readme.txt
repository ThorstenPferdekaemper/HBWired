Homematic Wired Homebrew OneWire 10 Channel Temperature Module
===============================================================

Am Modul HBW-1W-T10 können bis zu 10 OneWire Temperatursensoren angeschlossen werden.
Basis ist ein Arduino NANO mit RS485-Interface.
(Hinweis Hex Dateien: Geeignet für Atmel ATMEGA328p! *with_bootloader* ist ebenfalls für ATMEGA328p, aber inkl. dem Arduino NANO Bootloader!)

Unterstützte 1-Wire® Temperatursensoren:
DS18S20 Gerätecode 0x10
DS18B20 Gerätecode 0x28
DS1822  Gerätecode 0x22


Damit FHEM das Homebrew-Device richtig erkennt, muss die hbw_1w_t10_v1.xml Datei in den Ordner FHEM/lib/HM485/Devices/xml kopiert werden (Das Device gibt sich als HW-Typ 0x81 aus).



Standard-Pinbelegung:
(Seriell über USART - #define USE_HARDWARE_SERIAL)
0  - Rx RS485
1  - Tx RS485
2  - RS485 Enable
13 - Status LED
A6 - Bedientaster (Reset)
10 - OneWire Bus (parasitäre Stromversorgung)



"Debug"-Pinbelegung:
4  - Rx RS485
2  - Tx RS485
3  - RS485 Enable
13 - Status LED
8 - Bedientaster (Reset)
10 - OneWire Bus (parasitäre Stromversorgung)
