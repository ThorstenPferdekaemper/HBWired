Homematic Wired Homebrew Rollosteuerung 8 Kanal 
==============================================

Das Modul HBW-LC-Bl-8 schaltet bis zu 8 Rollos (mit Hilfe von 16 Relais).
Basis ist ein Arduino NANO mit RS485-Interface.
Hier wird Hardware Serial (USART) statt "HBWSoftwareSerial" genutzt! Der Bedientaster (Reset) ist ein Analogeingang! Keine Debug Ausgang! (16 IOs für Relais benötigt...)

Direktes Peering möglich (HBWLinkBlindSimple).

Damit FHEM das Homebrew-Device richtig erkennt, muss die hbw_lc_bl-8.xml Datei in den Ordner FHEM/lib/HM485/Devices/xml kopiert werden (Das Device gibt sich als HW-Typ 0x92 aus).

Standard-Pinbelegung:
0 - Rx RS485
1 - Tx RS485
2 - RS485 Enable
13 - Status LED
A6 - Bedientaster (Reset)
3  - BLIND1_ACT ("Ein-/Aus-Relais", Kanal 0)
4  - BLIND1_DIR ("Richungs-Relais", Kanal 0)
A0 - BLIND2_ACT ("Ein-/Aus-Relais", Kanal 1)
A1 - BLIND2_DIR ("Richungs-Relais", Kanal 1)
A2 - BLIND3_ACT
A3 - BLIND3_DIR
A4 - BLIND4_ACT
A5 - BLIND4_DIR
5  - BLIND5_ACT
6  - BLIND5_DIR
7  - BLIND6_ACT
8  - BLIND6_DIR
9  - BLIND7_ACT
10 - BLIND7_DIR
11 - BLIND8_ACT
12 - BLIND8_DIR
