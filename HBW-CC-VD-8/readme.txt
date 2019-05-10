Homematic Wired Homebrew 8-channel PID Valve actuator
=====================================================

*** In Entwicklung! *** Work in progress! ***


Das Modul HBW-CC-VD-8 bietet die Möglichkeit, bis zu 8 auf einem Thermoelement basierende Ventile individuell zu steuern. Mit den Ventilstellern sind 8 PID Regler fest verknüpft. Des weiteren können bis zu 8 OneWire Temperatursensoren angeschlossen werden.
Basis ist ein Arduino NANO mit RS485-Interface.
(Hinweis Hex Dateien: Geeignet für Atmel ATMEGA328p! *with_bootloader* ist ebenfalls für ATMEGA328p, aber inkl. dem Arduino NANO Bootloader!)

Direktes Peering ist für die Temperatursensoren mit den PID Regler möglich (HBWLinkInfoMessageActuator & HBWLinkInfoMessageSensor).


Die Ventile können wie ein Dimmer von 0-100% gesetzt werden.
Die Steuerung für die thermischen Ventile erfolgt per "time proportioning control" eine Art extrem langsames PWM. Bei über 50% schaltet das Ventil zuerst ein, unter 50% zuerst aus. Nach einer Änderung wird die erste Ein- oder Auszeit einmal halbiert

Damit FHEM das Homebrew-Device richtig erkennt, muss die HBW-CC-VD-8.xml Datei in den Ordner FHEM/lib/HM485/Devices/xml kopiert werden (Das Device gibt sich als HW-Typ 0x97 aus).

Konfiguration über FHEM:
PID Kanäle:
	POWERON_MODE (automatic/manual)
	Proportional
	Integral
	Derivative
	CYCLE_TIME

Valve/Ventil, Kanäle:
	LOGGING
	OUTPUT_LOCKED
	INVERTED
	SEND_MAX_INTERVALL
	VALVE_ERROR_POS
	SWITCH_TIME (Zeit die zum vollständigen öffnen benötigt wird)

Temperaturkanäle:
	SEND_DELTA_TEMP
	OFFSET
	SEND_MIN_INTERVALL
	SEND_MAX_INTERVALL



Standard-Pinbelegung:
(Seriell über USART - #define USE_HARDWARE_SERIAL)
0  - Rx RS485
1  - Tx RS485
2  - RS485 Enable
13 - Status LED
A6 - Bedientaster (Reset)
10 - Onewire Bus
3  - VD1
4  - VD2
5  - VD3
6  - VD4
7  - VD5
8  - VD6
9  - VD7
A5 - VD8


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

