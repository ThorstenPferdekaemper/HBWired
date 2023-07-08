Homematic Wired Homebrew Delta Temperature and Switch Module
============================================================

DeltaT Regler + 6 OneWire Temperatursensoren.

Das Modul HBW-CC-DT3-T6 wurde f�r eine Ofensteuerung entwickelt. Eine Umw�lzpumpe wird ab einer bestimmten Mindesttemperatur und Temperaturdifferenz ein- bzw. Ausgeschaltet.
Basis ist ein Arduino NANO mit RS485-Interface.

Damit FHEM das Homebrew-Device richtig erkennt, muss die hbw_cc_dt3_t6.xml Datei in den Ordner FHEM/lib/HM485/Devices/xml kopiert werden (Das Device gibt sich als HW-Typ 0x9C aus).

Es k�nnen bis zu 6 OneWire Temperatursensoren angeschlossen werden.
Unterst�tzte 1-Wire� Temperatursensoren:
DS18S20 Ger�tecode 0x10
DS18B20 Ger�tecode 0x28
DS1822  Ger�tecode 0x22

Direktes Peering:
Die tempsensor Kan�le k�nnen mit delta_t1, delta_t2 verkn�pft werden, oder anderen externen Kan�len. (delta_t1/delta_t2 sollte nur jeweils ein Temperatursensor zugewiesen werden!)
Der delta_t Kanaltyp kann mit Aktor Kan�len verkn�pft werden, z.B. externe Relais, Display, o.�.. (delta_t sendet short/long keyEvents)


Kan�le:
3x DeltaT Relais Ausg�nge (Relais, o.�.)
3x DeltaT1 'virtuelle' Temperatureing�nge
3x DeltaT2 'virtuelle' Temperatureing�nge
6x OneWire Temperatursensoren


DeltaT Konfiguration:
* HYSTERESIS: 0.0 - 3.0�C
* DELTA_TEMP: 0.0 - 25.4�C
* T1_MAX: -200.0 - 200.0�C
* T2_MIN: -200.0 - 200.0�C
* CYCLE_TIME: 5 - 40 Sekunden

* DeltaT wird berechnet als T2 - T1. Die Hysterese wird vom delta Wert subtrahiert, wenn die Temperaturdifferenz unterhalb von DELTA_TEMP liegt, ansonsten addiert (Als Option zuschaltbar). Der Ausgang wechselt sofort auf "Aus", wenn T1_MAX �berschritten oder T2_MIN unterschritten wird. Alternativ kann hier auch die Hysterese angewendet werden.


Standard-Pinbelegung:
(Seriell �ber USART - #define USE_HARDWARE_SERIAL)
0  - Rx RS485
1  - Tx RS485
2  - RS485 Enable
13 - Status LED
A6 - Bedientaster (Reset)
A3 - OneWire Bus (parasit�re oder dedizierte Stromversorgung)
 5 - RELAY_1
 6 - RELAY_2
 3 - RELAY_3


"Debug"-Pinbelegung:
4  - Rx RS485
2  - Tx RS485
3  - RS485 Enable
13 - Status LED
8  - Bedientaster (Reset)
10 - OneWire Bus (parasit�re oder dedizierte Stromversorgung)
5 - RELAY_1
6 - RELAY_2
A2 - RELAY_3
