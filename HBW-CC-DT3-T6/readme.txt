Homematic Wired Homebrew Delta Temperature and Switch Module
============================================================

*** In Entwicklung! *** Work in progress! ***

DeltaT Regler + 6 OneWire Temperatursensoren.

Das Modul HHBW-CC-DT3-T6 wurde f�r eine Ofensteuerung entwickelt. Eine Umw�lzpumpe wird ab einer bestimmten Mindesttemperatur und Temperaturdifferenz ein- bzw. Ausgeschaltet.
Basis ist ein Arduino NANO mit RS485-Interface.

Damit FHEM das Homebrew-Device richtig erkennt, muss die HBW_CC_DT3_T6.xml Datei in den Ordner FHEM/lib/HM485/Devices/xml kopiert werden (Das Device gibt sich als HW-Typ 0x71 aus).


Es k�nnen bis zu 6 OneWire Temperatursensoren angeschlossen werden.
Unterst�tzte 1-Wire� Temperatursensoren:
DS18S20 Ger�tecode 0x10
DS18B20 Ger�tecode 0x28
DS1822  Ger�tecode 0x22


Kan�le:
3 DeltaT Relais Ausg�nge (Relais, o.�.)
3 DeltaT1 'virtuelle' Temperatureing�nge
3 DeltaT2 'virtuelle' Temperatureing�nge
6 OneWire Temperatursensoren

DeltaT Konfiguration:
HYSTERESIS: 0.0 - 3.0�C
DELTA_TEMP: 0.0 - 25.4�C
T1_MAX: -200.0 - 200.0�C
T2_MIN: -200.0 - 200.0�C

* DeltaT wird berechnet als T2 - T1, die Hysterese wird vom delta Wert subtrahiert, wenn die Temperaturdifferenz unterhalb von DELTA_TEMP liegt, ansonsten addiert. Der Ausgang wechselt sofort auf "Aus", wenn T1_MAX �berschritten oder T2_MIN unterschritten wird.


Standard-Pinbelegung:
(Seriell �ber USART - #define USE_HARDWARE_SERIAL)
0  - Rx RS485
1  - Tx RS485
2  - RS485 Enable
13 - Status LED
A6 - Bedientaster (Reset)
10 - OneWire Bus (parasit�re Stromversorgung)
 5 - RELAY_1
 6 - RELAY_2
 7 - RELAY_3


"Debug"-Pinbelegung:
4  - Rx RS485
2  - Tx RS485
3  - RS485 Enable
13 - Status LED
8  - Bedientaster (Reset)
10 - OneWire Bus (parasit�re Stromversorgung)
A1 - RELAY_1
A2 - RELAY_2
A3 - RELAY_3
  