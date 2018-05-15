Homematic Wired Homebrew Relaismodul 8fach
==============================================

Das Modul HBW-LC-Sw-8 schaltet bis zu 8 angeschlossene Relais.
Basis ist ein Arduino NANO mit RS485-Interface.
(Hinweis Hex Dateien: Geeignet für Atmel ATMEGA328p! *with_bootloader* ist ebenfalls für ATMEGA328p, aber inkl. dem Arduino NANO Bootloader!)

Direktes Peering möglich (HBWLinkSwitchSimple).

Damit FHEM das Homebrew-Device richtig erkennt, muss die HBW-LC-Sw-8.xml Datei in den Ordner FHEM/lib/HM485/Devices/xml kopiert werden (Das Device gibt sich als HW-Typ 0x83 aus).

Standard-Pinbelegung:
4 - Rx RS485
2 - Tx RS485
3 - RS485 Enable
13 - Status LED
0 - Rx Debug
1 - Tx Debug
8 - Bedientaster (Reset)
A0 - Relais 1
A1 - Relais 2
A2 - Relais 3
A3 - Relais 4
A4 - Relais 5
A5 - Relais 6
10 - Relais 7
11 - Relais 8
