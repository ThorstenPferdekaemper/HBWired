Homematic Wired Homebrew Schwingungspaketsteuerung mit Zusatzfunktionen
=======================================================================

Das Modul HBW-CC-WW-SPktS bietet einen Ausgang mit Schwingungspaketsteuerung (für 50Hz Heizgeräte, o.ä.), dazu 5 OneWire Temperatursensoren und zwei DeltaT Regler als Schaltausgang für bspw. eine Umwälzpumpe.
Basis ist ein Arduino NANO mit RS485-Interface.

Unterstützte 1-Wire® Temperatursensoren:
DS18S20 Gerätecode 0x10
DS18B20 Gerätecode 0x28
DS1822  Gerätecode 0x22

Direktes Peering:
Die tempsensor Kanäle können mit delta_t1, delta_t2 verknüpft werden, oder anderen externen Kanälen. (delta_t1/delta_t2 sollte nur jeweils ein Temperatursensor zugewiesen werden!)
Der delta_t Kanaltyp kann mit Aktor Kanälen verknüpft werden, z.B. externe Relais, Display, o.ä.. (delta_t sendet short/long keyEvents)


Damit FHEM das Homebrew-Device richtig erkennt, muss die hbw-cc-ww-spkts.xml Datei in den Ordner FHEM/lib/HM485/Devices/xml kopiert werden (Das Device gibt sich als HW-Typ 0x99 aus).

Kanäle:
1x Schwingungspaketsteuerung
2x DeltaT Relais Ausgänge (Relais, o.ä.)
2x DeltaT1 'virtuelle' Temperatureingänge
2x DeltaT2 'virtuelle' Temperatureingänge
5x OneWire Temperatursensoren


DeltaT Konfiguration:
* HYSTERESIS: 0.0 - 3.0°C
* DELTA_TEMP: 0.0 - 25.4°C
* T1_MAX: -200.0 - 200.0°C
* T2_MIN: -200.0 - 200.0°C
* CYCLE_TIME: 5 - 40 Sekunden

* DeltaT wird berechnet als T2 - T1. Die Hysterese wird vom delta Wert subtrahiert, wenn die Temperaturdifferenz unterhalb von DELTA_TEMP liegt, ansonsten addiert (Als Option zuschaltbar). Der Ausgang wechselt sofort auf "Aus", wenn T1_MAX überschritten oder T2_MIN unterschritten wird. Alternativ kann hier auch die Hysterese angewendet werden.

DIMMER (Schwingungspaketsteuerung)
* LOGGING
* MAX_OUTPUT_RANGE (30 - 100%) - Begrenzung für Ausgang
* MAX_TEMP - Maximale Temperatur
* MAX_ON_TIME - Maximale Einschaltzeit


Schwingungspaketsteuerung Periodenlänge (Wellenpaket Schritte) = 25; Schwingungspaketdauer 500ms


Standard-Pinbelegung:
(Seriell über USART - #define USE_HARDWARE_SERIAL)
0  - Rx RS485
1  - Tx RS485
2  - RS485 Enable
13 - Status LED
A6 - Bedientaster (Reset)
A3 - OneWire Bus (parasitäre oder dedizierte Stromversorgung)
 - Schwingungspaketsteuerung
 5 - RELAY_1 (DeltaT)
 6 - RELAY_2 (DeltaT)


"Debug"-Pinbelegung:
4  - Rx RS485
2  - Tx RS485
3  - RS485 Enable
13 - Status LED
8 - Bedientaster (Reset)
10 - OneWire Bus (parasitäre oder dedizierte Stromversorgung)
 - Schwingungspaketsteuerung
5 - RELAY_1 (DeltaT)
6 - RELAY_2 (DeltaT)


