Homematic Wired Homebrew S0-Interface
=====================================

Das Modul HBW-Sen-EP liest bis zu 8 angeschlossene S0-Signale ein und sendet die aktuellen Z�hlerst�nde an die Zentrale.
�ber das S0-Interface k�nnen bspw. Stromz�hler, Gasz�hler, W�rmemengenz�hler, Wasserz�hler, etc. eingelesen werden.
Basis ist ein Arduino NANO, oder Wattuino Pro Mini PB (mit ATmega328PB), o.�. mit RS485-Interface.
S0-Signal Eing�nge sind �ber Port Interrupts m�glich, aber nicht in Kombination mit HBWSoftwareSerial / SoftwareSerial. (Bei Nutzung der Arduino IDE, die Datei HBWSoftwareSerial.cpp z.B. in HBWSoftwareSerial.cpp_ umbenennen, um das Projekt erfolgreich zu Kompilieren.)


Damit FHEM das Homebrew-Device richtig erkennt, muss die Datei hbw_sen_ep.xml in den Ordner \FHEM\lib\HM485\Devices\xml kopiert werden (Das Device gibt sich als HW-Typ 0x84 aus).
Config der Kan�le kann �ber das FHEM-Webfrontend vorgenommen werden:
# Deaktivierung
# Invertierung
# Minimum-Sendeintervall
# Maximum-Sendeintervall
# Z�hlerdifferenz, ab der gesendet wird


Hardware Serial (USART) statt "HBWSoftwareSerial", daher keine Debug Ausgabe �ber USB! Der Bedientaster (Reset) ist ein Analogeingang! (nicht bei ATmega328PB)
Mit "#define USE_HARDWARE_SERIAL" ist auch Port Interrupts "USE_INTERRUPTS_FOR_INPUT_PINS" m�glich.
Standard-Pinbelegung:
0 - Rx RS485
1 - Tx RS485
2 - RS485 Enable
13 - Status LED
A6 - Bedientaster (Reset)
8  - Input des 1. S0-Signals
7  - Input des 2. S0-Signals
10 - Input des 3. S0-Signals
9  - Input des 4. S0-Signals
A0 - Input des 5. S0-Signals
A1 - Input des 6. S0-Signals
A2 - Input des 7. S0-Signals
A3 - Input des 8. S0-Signals


Debug-Pinbelegung:
(nicht mit "USE_INTERRUPTS_FOR_INPUT_PINS" - Port Interrupts m�glich)
0 - Rx Debug, Serial -> USB
1 - Tx Debug, Serial -> USB
3 - RS485 Enable
13 - Status LED
4 - Rx Debug
2 - Tx Debug
8 - Bedientaster (Reset)
A0 - Input des 1. S0-Signals
A1 - Input des 2. S0-Signals
A2 - Input des 3. S0-Signals
A3 - Input des 4. S0-Signals
A4 - Input des 5. S0-Signals
A5 - Input des 6. S0-Signals
5  - Input des 7. S0-Signals
7  - Input des 8. S0-Signals



Das Device gibt nur die Anzahl der Impulse weiter - eine Umrechnung in die entsprechende Einheit muss in FHEM konfiguriert werden.
Beispielkonfiguration:

define EnergieMessung HM485 AAAABBBB
attr EnergieMessung firmwareVersion 3.06
attr EnergieMessung model HBW_Sen_EP
attr EnergieMessung room HM485
attr EnergieMessung serialNr HBW3315899
define FileLog_EnergieMessung FileLog ./log/EnergieMessung-%Y.log EnergieMessung*|EnergieMessung_07:energy:.*
attr FileLog_EnergieMessung logtype text
attr FileLog_EnergieMessung room HM485

define EnergieMessung_01 HM485 AAAABBBB_01
attr EnergieMessung_01 firmwareVersion 3.06
attr EnergieMessung_01 model HBW_Sen_EP
attr EnergieMessung_01 room HM485
attr EnergieMessung_01 serialNr HBW3315899
attr EnergieMessung_01 stateFormat energy
attr EnergieMessung_01 subType counter
attr EnergieMessung_01 userReadings energy monotonic {ReadingsVal("EnergieMessung_07","counter",0)/100.0;;;;}, power differential {ReadingsVal("EnergieMessung_07","counter",0);;;;}
