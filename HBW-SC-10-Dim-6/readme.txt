Homematic Wired Homebrew PWM/0-10V Master Dimmer + 10 digital Eingänge
======================================================================

// TODO: Implement dim peering params: RAMP_START_STEP


Das Modul HBW-SC-10-Dim-6 bietet 6 kombinierte analoge bzw. PMW Ausgänge und 10 digitale Eingänge.
Basis ist ein Arduino NANO mit RS485-Interface.

Direktes Peering möglich (HBWLinkDimmerAdvanced & HBWLinkKey).

Die 6 analogen Ausgangskanäle geben eine Spannung von "0-10V" (alternativ "1-10V") als Dimmer aus (0-100%). Parallel steht das PWM Signal mit 'open collector' (12V Ausgangsspannung) zur Verfügung. Vier der Sechs Ausgänge geben das PWM Signal mit 122,5 Hz aus, die Restlichen zwei mit 976 Hz. Der PWM, bzw. Spannungsbereich kann auf ein Maximalwert von 40-100% pro Kanal konfiguriert werden.
Statt "0-10V" können die analogen Ausgangskanäle, individuell per Jumper auf "0-20mA" geändert werden.

Die 10 Eingänge sind galvanisch getrennt und können mit einer Gleichspannung von ca. 12-24V betrieben werden. Die Eingänge stehen als Sensor oder Schalterkanal gleichzeitig zur Verfügung, d.h. pro Eingang sind zwei Kanäle vorhanden.
Die Schalterkanäle können normal als Taster/Schalter inkl. Peering genutzt werden. Mögliche Typen: PUSHBUTTON, SWITCH, MOTIONSENSOR, DOORSENSOR. (Weitere Details in HBWKey.h)
Achtung: Die Schalterkanäle sind standardmäßig aktiv und als "PUSHBUTTON" konfiguriert. In den Kanaleinstellungen kann dies geändert, bzw. die Kanäle deaktiviert werden.
Die Sensor Kanäle können abgefragt werden (kein Peering), und liefern dann "sensor_open" oder "sensor_closed" zurück. Eine 'event/notify' broadcast Nachricht kann bei einer Statusänderung versendet werden. Dies ist standardmäßig in der Kanalkonfiguration deaktiviert. Eine Invertierung lässt sich ebenfalls aktivieren.

Damit FHEM das Homebrew-Device richtig erkennt, muss die hbw_sc-10_dim-6.xml Datei in den Ordner FHEM/lib/HM485/Devices/xml kopiert werden (Das Device gibt sich als HW-Typ 0x96 aus).

Mögliche Dimmer-"Endstufen":
0-10V: LED Dimmer, Finder Typ 15.11 - Slave Dimmer, etc.
0-90% PWM: Eltako LUD12-230V


Konfigurationsoptionen der verschiedenen Kanaltypen:
DIMMER
* LOGGING
* PWM_RANGE (40 - 100%) - Maximaler PWM Wert am Ausgang
* OUTPUT_VOLTAGE (0-10V oder 1-10V)

SENSOR
* INPUT_LOCKED
* INVERTED
* NOTIFY

KEY
* INPUT_LOCKED
* INVERTED
* INPUT_TYPE (DOORSENSOR, MOTIONSENSOR, SWITCH, PUSHBUTTON [default])
* LONG_PRESS_TIME

removed:[DIMMER KEY (virtuelle Taster, welche einen Tastendruck senden, wenn der jeweilige Dimmerkanal ein- oder ausgeschaltet wird [0% = Aus])
* INPUT_LOCKED
* INVERTED]
added:
VIRTUAL_DIMMER
* LOGIC_COMBINATION


Standard-Pinbelegung:
(Seriell über USART - #define USE_HARDWARE_SERIAL)
0  - Rx RS485
1  - Tx RS485
2  - RS485 Enable
13 - Status LED
A6 - Bedientaster (Reset)
9  - PWM1 out (122,5 Hz)
5  - PWM2 out (  976 Hz)
10 - PWM3 out (122,5 Hz)
6  - PWM4 out (  976 Hz)
11 - PWM5 out (122,5 Hz)
3  - PWM6 out (122,5 Hz)
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
