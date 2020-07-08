Homematic Wired Homebrew Delta Temperature and Switch Module
============================================================

DeltaT Regler + 6 OneWire Temperatursensoren.

Das Modul HHBW-CC-DT3-T6 wurde für eine Ofensteuerung entwickelt. Eine Umwälzpumpe wird ab einer bestimmten Mindesttemperatur und Temperaturdifferenz ein- bzw. Ausgeschaltet.
Basis ist ein Arduino NANO mit RS485-Interface.

Damit FHEM das Homebrew-Device richtig erkennt, muss die HBW_CC_DT3_T6.xml Datei in den Ordner FHEM/lib/HM485/Devices/xml kopiert werden (Das Device gibt sich als HW-Typ 0x9C aus).

Es können bis zu 6 OneWire Temperatursensoren angeschlossen werden.
Unterstützte 1-Wire® Temperatursensoren:
DS18S20 Gerätecode 0x10
DS18B20 Gerätecode 0x28
DS1822  Gerätecode 0x22


Kanäle:
3 DeltaT Relais Ausgänge (Relais, o.ä.)
3 DeltaT1 'virtuelle' Temperatureingänge
3 DeltaT2 'virtuelle' Temperatureingänge
6 OneWire Temperatursensoren


DeltaT Konfiguration:
HYSTERESIS: 0.0 - 3.0°C
DELTA_TEMP: 0.0 - 25.4°C
T1_MAX: -200.0 - 200.0°C
T2_MIN: -200.0 - 200.0°C

* DeltaT wird berechnet als T2 - T1, die Hysterese wird vom delta Wert subtrahiert, wenn die Temperaturdifferenz unterhalb von DELTA_TEMP liegt, ansonsten addiert. Der Ausgang wechselt sofort auf "Aus", wenn T1_MAX überschritten oder T2_MIN unterschritten wird.


Standard-Pinbelegung:
(Seriell über USART - #define USE_HARDWARE_SERIAL)
0  - Rx RS485
1  - Tx RS485
2  - RS485 Enable
13 - Status LED
A6 - Bedientaster (Reset)
10 - OneWire Bus (parasitäre Stromversorgung)
 5 - RELAY_1
 6 - RELAY_2
 7 - RELAY_3


"Debug"-Pinbelegung:
4  - Rx RS485
2  - Tx RS485
3  - RS485 Enable
13 - Status LED
8  - Bedientaster (Reset)
10 - OneWire Bus (parasitäre Stromversorgung)
A1 - RELAY_1
A2 - RELAY_2
A3 - RELAY_3
  