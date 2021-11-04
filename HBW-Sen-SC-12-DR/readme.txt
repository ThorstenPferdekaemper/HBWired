Homematic Wired Homebrew Key / Sensorkontakte
=============================================

Das Modul HBW-Sen-SC-12-DR stellt 12 digitale Eing�nge als Key/Taster & parallel als Sensor (Sensorkontakte) zu Verf�gung.

Direktes Peering der Key Kan�le m�glich (HBWLinkKey).

Damit FHEM das Homebrew-Device richtig erkennt, muss die Datei hbw_sen_sc_12_dr.xml in den Ordner \FHEM\lib\HM485\Devices\xml kopiert werden (Das Device gibt sich als HW-Typ 0xA6 aus).


Konfigurationsoptionen der Kan�le:
SENSOR
* INPUT_LOCKED
* INVERTED
* NOTIFY

KEY
* INPUT_LOCKED
* INVERTED
* INPUT_TYPE (DOORSENSOR, MOTIONSENSOR, SWITCH, PUSHBUTTON [default])
* LONG_PRESS_TIME (nur f�r INPUT_TYPE PUSHBUTTON relevant)



Pinbelegung:
0 - Rx Debug, Serial -> USB
1 - Tx Debug, Serial -> USB
3 - RS485 Enable
13 - Status LED
4 - Rx Debug
2 - Tx Debug
8 - Bedientaster (Reset)
A0 - ... siehe HBW-Sen-SC-12-DR.ino


Alternative M�glichkeit, per "#define USE_HARDWARE_SERIAL" aktivieren:
Hier wird Hardware Serial (USART) statt "HBWSoftwareSerial" genutzt, daher keine Debug Ausgabe �ber USB! Der Bedientaster (Reset) ist ein Analogeingang!
