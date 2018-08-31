Homematic Wired Homebrew Rollosteuerung 4 Kanal 
===============================================

Das Modul HBW-LC-Bl-4 schaltet bis zu 4 Rollos (mit Hilfe von 8 Relais).
Basis ist ein Arduino NANO mit RS485-Interface.
(Hinweis Hex Dateien: Geeignet für Atmel ATMEGA328p! *with_bootloader* ist ebenfalls für ATMEGA328p, aber inkl. dem Arduino NANO Bootloader!)

Direktes Peering möglich (HBWLinkBlindSimple).

Damit FHEM das Homebrew-Device richtig erkennt, muss die hbw_lc_bl-4.xml Datei in den Ordner FHEM/lib/HM485/Devices/xml kopiert werden (Das Device gibt sich als HW-Typ 0x82 aus).

Implementierte Befehle:
0x00-0xC8 = 0-100%
0xC9 = Stopp
0xFF = Toggle (bzw. Stopp, wenn Motor läuft)

Standard-Pinbelegung:
4 - Rx RS485
2 - Tx RS485
3 - RS485 Enable
13 - Status LED
0 - Rx Debug
1 - Tx Debug
8 - Bedientaster (Reset)
5  - BLIND1_ACT ("Ein-/Aus-Relais", Kanal 0)
6  - BLIND1_DIR ("Richungs-Relais", Kanal 0)
A0 - BLIND2_ACT ("Ein-/Aus-Relais", Kanal 1)
A1 - BLIND2_DIR ("Richungs-Relais", Kanal 1)
A2 - BLIND3_ACT
A3 - BLIND3_DIR
A4 - BLIND4_ACT
A5 - BLIND4_DIR


Quelle/Vorlage: https://github.com/kc-GitHub/HM485-Lib/tree/markus/HBW-LC-Bl4
