Homematic Wired Homebrew PWM/0-10V Master Dimmer + 10 digital Eingänge
======================================================================

*** In Entwicklung! *** Work in progress! ***
// TODO: Implement dim peering params: ON_LEVEL_PRIO, RAMP_START_STEP


Das Modul HBW-SC-10-Dim-6 bietet 6 kombinierte analoge bzw. PMW Ausgänge und 10 digitale Eingänge.
Basis ist ein Arduino NANO mit RS485-Interface.
(Hinweis Hex Dateien: Geeignet für Atmel ATMEGA328p! *with_bootloader* ist ebenfalls für ATMEGA328p, aber inkl. dem Arduino NANO Bootloader!)

Direktes Peering möglich (HBWLinkDimmerAdvanced & HBWLinkKey).

Die 6 analogen Ausgangskanäle geben eine Spannung von "0-10V" (alternativ "1-10V") als Dimmer aus (0-100%). Parallel steht das PWM Signal mit 'open collector' (12V Ausgangsspannung) zur Verfügung. Vier der Sechs Ausgänge geben das PWM Signal mit 122,5 Hz aus, die Restlichen zwei mit 976 Hz. Der PWM, bzw. Spannungsbereich kann auf ein Maximalwert von 40-100% pro Kanal konfiguriert werden.
Statt "0-10V" können die analogen Ausgangskanäle, individuell per Jumper auf "0-20mA" geändert werden.

Die 10 Eingänge sind galvanisch getrennt und können mit einer Gleichspannung von ca. 12-24V betrieben werden. Die Eingänge stehen als Sensor oder Schalterkanal gleichzeitig zur Verfügung, d.h. pro Eingang sind zwei Kanäle vorhanden.
Die Schalterkanäle können normal als Taster/Schalter inkl. Peering genutzt werden. Mögliche Typen: PUSHBUTTON, SWITCH, MOTIONSENSOR, DOORSENSOR. (Weitere Details in HBWKey.h)
Die Sensor Kanäle müssen abgefragt werden (kein Peering), und liefern dann "sensor_open" oder "sensor_closed" zurück. (Möglichkeit der Invertierung über Kanalkonfiguration)

Damit FHEM das Homebrew-Device richtig erkennt, muss die hbw_sc-10_dim-6.xml Datei in den Ordner FHEM/lib/HM485/Devices/xml kopiert werden (Das Device gibt sich als HW-Typ 0x96 aus).

Mögliche Dimmer-"Endstufen":
0-10V: LED Dimmer, Finder Typ 15.11 - Slave Dimmer, etc.
0-90% PWM: Eltako LUD12-230V


Standard-Pinbelegung:
(Seriell über USART - #define USE_HARDWARE_SERIAL)
0  - Rx RS485
1  - Tx RS485
2  - RS485 Enable
13 - Status LED
A6 - Bedientaster (Reset)
9  - PWM1 out (controlled by timer1)
5  - PWM2 out (controlled by timer0)
10 - PWM3 out (controlled by timer1)
6  - PWM4 out (controlled by timer0)
11 - PWM5 out (controlled by timer2)
3  - PWM6 out (controlled by timer2)
12 - IO1
8  - IO2
7  - IO3
4  - IO4
A0 - IO5
A1 - IO6
A2 - IO7
A3 - IO8
A4 - IO9
A5 - IO10


"Debug"-Pinbelegung:
4  - Rx RS485
2  - Tx RS485
3  - RS485 Enable
13 - Status LED
8 - Bedientaster (Reset)
9  - PWM1 out (controlled by timer1)
5  - PWM2 out (controlled by timer0)
10 - PWM3 out (controlled by timer1)
6  - PWM4 out (controlled by timer0)
11 - PWM5 out (controlled by timer2)

7  - IO1
A5 - IO2
12 - IO3
A0 - IO4
A1 - IO5
A2 - IO6
A3 - IO7
A4 - IO8
