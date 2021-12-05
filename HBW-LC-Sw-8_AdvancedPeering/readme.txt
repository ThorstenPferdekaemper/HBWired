Homematic Wired Homebrew Relaismodul 8-fach
===========================================

Das Modul HBW-LC-Sw-8 schaltet bis zu 8 angeschlossene Relais.
Basis ist ein Arduino NANO mit RS485-Interface.

Direktes Peering möglich (HBWSwitchAdvanced & HBWLinkSwitchAdvanced):
Peering mit TOGGLE, TOGGLE_TO_COUNTER, TOGGLE_INVERSE_TO_COUNTER, onTime, offTime (Ein-/Ausschaltdauer), onDelayTime, offDelayTime (Ein-/Ausschaltverzögerung).
Maximale Ein-/Ausschaltdauer, Ein-/Ausschaltverzögerung, jeweils: 63000 Sekunden (17,5 Stunden).

Damit FHEM das Homebrew-Device richtig erkennt, muss die HBW-LC-Sw-8.xml Datei in den Ordner FHEM/lib/HM485/Devices/xml kopiert werden (Das Device gibt sich als HW-Typ 0x83 aus).

Standard-Pinbelegung:
(Seriell über USART - #define USE_HARDWARE_SERIAL)
0  - Rx RS485
1  - Tx RS485
2  - RS485 Enable
13 - Status LED
A6 - Bedientaster (Reset)
A4 - Relais 1
A2 - Relais 2
A0 - Relais 3
10 - Relais 4
A1 - Relais 5
 9 - Relais 6
A3 - Relais 7
 5 - Relais 8


"Debug"-Pinbelegung:
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
