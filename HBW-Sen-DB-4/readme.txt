Homematic Wired Homebrew Doorbell (4 channel)
=============================================


------ in Entwicklung! ------

Das Modul HBW-Sen-DB-4 ermöglicht den Anschluss von 4 Klingeltastern an den Homematic Bus. Zusätzlich ist ein Ausgang für die Beleuchtung des Klingeltableau vorhanden (Dimmbar).
Die Klingeltasterkanäle unterstützen Direktverknüpfungen (Peering).

Basis ist ein Arduino NANO mit RS485-Interface.

Damit FHEM das Homebrew-Device richtig erkennt, muss die Datei hbw-sen-db-4.xml in den Ordner FHEM/lib/HM485/Devices/xml kopiert werden (Das Device gibt sich als HW-Typ 0x98 aus).


Kanäle:
1x dimmer (Klingeltableau Hintergrundbeleuchtung, mit Auto-off(?) & automatischer Helligkeit)
4x Tastereingang







Standard-Pinbelegung:
(Seriell über USART - #define USE_HARDWARE_SERIAL)
0  - Rx RS485
1  - Tx RS485
2  - RS485 Enable
13 - Status LED
A6 - Bedientaster (Reset)
A0 - BUTTON_1
A1 - BUTTON_2
A2 - BUTTON_3
A3 - BUTTON_4
5  - BACKLIGHT (PWM Pin!)
A7 - LDR (Photo R (ADC Pin!))



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


