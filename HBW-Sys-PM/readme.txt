Homematic Wired Homebrew Busüberwachungsmodul
=============================================

Das Modul HBW-Sys-PM misst die Busspannung, Strom und Leistung. Es bietet darüber hinaus 4 Ausgänge und 2 Eingänge.
Basis ist ein Arduino NANO mit RS485-Interface. Das Messmodul ist ein SBCDVA mit INA236 I²C chip. (0 - 48 V DC & 0 - 8 A; 16-Bit) https://joy-it.net/de/products/SBC-DVA

Direktes Peering der Ausgänge möglich mit "HBWSwitchAdvanced & HBWLinkSwitchAdvanced"
Der Messkanal (Kanal 1) lässt sich als Taster (key) peeren, der einen kurzen Tastendruck sendet, wenn ein Alarm auftritt. Ein langer Tastendruck signalisiert das keine Alarme/Fehler mehr anstehen.

Damit FHEM das Homebrew-Device richtig erkennt, muss die hbw-sys-pm.xml Datei in den Ordner FHEM/lib/HM485/Devices/xml kopiert werden (Das Device gibt sich als HW-Typ 0x8D aus).

Kanalkonfiguration:
"BUS POWER" (Kanal 1)
ENABLED - Kanal komplett An- oder Abschalten
KEY_EVENT_ALARM - Tastendruck bei Alarm/Fehler senden (wird ohne Verzögerung gesendet)
SAMPLE_RATE - Pause zwischen dem Auslesen der Messwerte. Es wird ein Durchschnitt über die letzten 4 Messwerte gebildet (moving average)
SEND_MIN_INTERVAL - Mindestwartezeit zum senden der Werte
SEND_MAX_INTERVAL - Maximale Pause (nach Ablauf dieser Zeit werden alle Werte übermittelt, egal ob sie sich geändert haben)
ALARM_MAX_VOLTAGE - Setzt STATE "Over Voltage" bei überschreiten dieses Werts
ALARM_MIN_VOLTAGE - Setzt STATE "Under Voltage" bei unterschreiten dieses Werts
ALARM_MAX_POWER - Setzt STATE "Over Power" bei überschreiten dieses Werts



Standard-Pinbelegung:
(Seriell über USART - #define USE_HARDWARE_SERIAL)
0  - Rx RS485
1  - Tx RS485
2  - RS485 Enable
13 - Status LED
A6 - Bedientaster (Reset)
A4 - INA236 I²C (SDA)
A5 - INA236 I²C (SCL)
7  - open collector 1
8  - open collector 2
A1 - Relais 1
4  - Relais 2
A2 - Input 1
A3 - Input 2


"Debug"-Pinbelegung:
 4 - Rx RS485
 2 - Tx RS485
 3 - RS485 Enable
13 - Status LED
 0 - Rx Debug
 1 - Tx Debug
 8 - Bedientaster (Reset)
A4 - INA236 I²C (SDA)
A5 - INA236 I²C (SCL)
 6 - open collector 1
 7 - open collector 2
A1 - Relais 1
10 - Relais 2
A2 - Input 1
A3 - Input 2
