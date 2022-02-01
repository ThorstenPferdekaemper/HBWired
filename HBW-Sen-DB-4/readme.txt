Homematic Wired Homebrew Doorbell (4 channel)
=============================================

Das Modul HBW-Sen-DB-4 ermöglicht den Anschluss von 4 Klingeltasten an den Homematic Bus. Im Gegensatz zu standard Tastern (HBWKey) lassen sich hier eine bestimmten Anzahl von Tastendrücken innerhalb einer bestimmten Zeit unterdrücken. (Kanalkonfiguration: SUPPRESS_NUM, SUPPRESS_TIME)
Zusätzlich ist ein Ausgang für die Beleuchtung des Klingeltableau vorhanden (Dimmbar).
Die Klingeltastenkanäle unterstützen Direktverknüpfungen (Peering).

Basis ist ein Arduino NANO mit RS485-Interface.

Damit FHEM das Homebrew-Device richtig erkennt, muss die Datei hbw-sen-db-4.xml in den Ordner FHEM/lib/HM485/Devices/xml kopiert werden (Das Device gibt sich als HW-Typ 0x98 aus).


Kanäle:
1x dimmer (Klingeltableau Hintergrundbeleuchtung, mit Auto-off & automatischer Helligkeit)
4x Tastereingang


Neustart des Moduls mit "!!" (raw message 2121) möglich.


Standard-Pinbelegung:
(Seriell über USART - #define USE_HARDWARE_SERIAL)
0  - Rx RS485
1  - Tx RS485
2  - RS485 Enable
13 - Status LED
A6 - Bedientaster (Reset)
8  - BUTTON_1
7  - BUTTON_2
4  - BUTTON_3
A1 - BUTTON_4
9  - BACKLIGHT (PWM Pin!)
A0 - LDR (Photo R (ADC Pin!))



"Debug"-Pinbelegung:
4  - Rx RS485
2  - Tx RS485
3  - RS485 Enable
13 - Status LED
8  - Bedientaster (Reset)
A0 - BUTTON_1
A1 - BUTTON_2
A2 - BUTTON_3
A3 - BUTTON_4
5  - BACKLIGHT (PWM Pin!)
A7 - LDR (Photo R (ADC Pin!))


