Homematic Wired Homebrew Rollosteuerung 8 Kanal 
===============================================

Das Modul HBW-LC-Bl-8 schaltet bis zu 8 Rollos (mit Hilfe von 16 Relais).
Basis ist ein Arduino NANO mit RS485-Interface.
(Hinweis Hex Dateien: Geeignet für Atmel ATMEGA328p! *with_bootloader* ist ebenfalls für ATMEGA328p, aber inkl. dem Arduino NANO Bootloader!)
Achtung:
Hier wird Hardware Serial (USART) statt "HBWSoftwareSerial" genutzt! Der Bedientaster (Reset) ist ein Analogeingang! Keine Debug Ausgabe! (16 IOs für Relais benötigt...)
-> Schaltung: http://loetmeister.de/Elektronik/homematic/hbw-lc-bl-4_8.htm

Direktes Peering möglich (HBWLinkBlindSimple).

Damit FHEM das Homebrew-Device richtig erkennt, muss die hbw_lc_bl-8.xml Datei in den Ordner FHEM/lib/HM485/Devices/xml kopiert werden (Das Device gibt sich als HW-Typ 0x92 aus).

Implementierte Befehle:
0x00-0xC8 = 0-100%
0xC9 = Stopp
0xFF = Toggle (bzw. Stopp, wenn Motor läuft)

Standard-Pinbelegung:
0 - Rx RS485
1 - Tx RS485
2 - RS485 Enable
13 - Status LED
A6 - Bedientaster (Reset)
10 - BLIND1_ACT ("Ein-/Aus-Relais", Kanal 0)
9  - BLIND1_DIR ("Richungs-Relais", Kanal 0)
8  - BLIND2_ACT ("Ein-/Aus-Relais", Kanal 1)
7  - BLIND2_DIR ("Richungs-Relais", Kanal 1)
6  - BLIND3_ACT
5  - BLIND3_DIR
4  - BLIND4_ACT
3  - BLIND4_DIR
A5 - BLIND5_ACT
A4 - BLIND5_DIR
A3 - BLIND6_ACT
A2 - BLIND6_DIR
A0 - BLIND7_ACT
A1 - BLIND7_DIR
12 - BLIND8_ACT
11 - BLIND8_DIR


Vorlage: HBW-LC-Bl-4 von https://github.com/kc-GitHub/HM485-Lib/tree/markus/HBW-LC-Bl4
