Homematic Wired Homebrew 8-channel PID Valve actuator
=====================================================

Das Modul HBW-CC-VD-8 bietet die M�glichkeit, bis zu 8 auf einem Thermoelement basierende Ventile individuell zu steuern. Mit den Ventilstellern sind 8 PID Regler fest verkn�pft. Des weiteren k�nnen bis zu 8 OneWire Temperatursensoren angeschlossen werden.
Basis ist ein Arduino NANO mit RS485-Interface.
(Hinweis: HBW-CC-VD-8 mit debug Ausgabe ben�tigt zu viel Speicher f�r einem Arduino NANO. Zum Testen weniger Kan�le oder HBW-CC-VD-2 nutzen!)

Direktes Peering ist f�r die Temperatursensoren mit den PID Regler m�glich (HBWLinkInfoEventActuator & HBWLinkInfoEventSensor).


Die Ventile k�nnen wie ein Dimmer von 0-100% gesetzt werden.
Die Steuerung f�r die thermischen Ventile erfolgt per "time proportioning control" eine Art extrem langsames PWM. Bei �ber 50% schaltet das Ventil zuerst ein, unter 50% zuerst aus. Nach einer �nderung wird die erste Ein- oder Auszeit einmal halbiert.

Damit FHEM das Homebrew-Device richtig erkennt, muss die HBW-CC-VD-8.xml Datei in den Ordner FHEM/lib/HM485/Devices/xml kopiert werden (Das Device gibt sich als HW-Typ 0x97 aus).

Konfiguration �ber FHEM:
PID Kan�le:
	POWERON_MODE (automatic/manual)
	Proportional
	Integral
	Derivative
	CYCLE_TIME

Valve/Ventil, Kan�le:
	LOGGING
	OUTPUT_LOCKED
	INVERTED
	VALVE_ERROR_POS
	SWITCH_TIME (Zeit die zum vollst�ndigen �ffnen ben�tigt wird)

Temperaturkan�le:
	SEND_DELTA_TEMP
	OFFSET
	SEND_MIN_INTERVALL
	SEND_MAX_INTERVALL



Standard-Pinbelegung:
(Seriell �ber USART - #define USE_HARDWARE_SERIAL)
0  - Rx RS485
1  - Tx RS485
2  - RS485 Enable
13 - Status LED
A6 - Bedientaster (Reset)
A5 - Onewire Bus
A4 - VD1
8  - VD2
7  - VD3
4  - VD4
A0 - VD5
A1 - VD6
A2 - VD7
A3 - VD8


"Debug"-Pinbelegung:
4  - Rx RS485
2  - Tx RS485
3  - RS485 Enable
13 - Status LED
8 - Bedientaster (Reset)
10 - Onewire Bus
A1  - VD1
A2  - VD2
5  - VD3
6  - VD4
7  - VD5
12 - VD6
9  - VD7
A5 - VD8

