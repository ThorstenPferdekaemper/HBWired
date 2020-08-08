Homematic Wired Homebrew Display Module + 4 Key Input
=====================================================

--- still in development ---

Das Modul HBW-DIS-Key-4 bindet ein LCD Character Display an den Bus an. Die Anzahl der Zeilen und Zeichen pro Zeile kann angepasst werden.
Zusätzlich sind 4 Tasterkanäle vorhanden.
Die Tasterkanäle unterstützen Direktverknüpfungen (Peering). Ebenso können die virtuellen Switch- und Temperaturkanäle mit Sensoren direkt verknüpft werden.

Basis ist ein Arduino NANO mit RS485-Interface.

Damit FHEM das Homebrew-Device richtig erkennt, muss die Datei hbw-dis-key-4.xml in den Ordner FHEM/lib/HM485/Devices/xml kopiert werden (Das Device gibt sich als HW-Typ 0x71 aus).


Kanäle:
1x Display
1x Dimmer (Display Hintergrundbeleuchtung)
2x (4x) display_line
4x Key (Taster, Schalter)
4x display_v_temp	(Speichert Temperaturmesswerte oder beliebige Werte von -32768 bis 32767)
4x display_v_switch	(Speichert einen binären Wert, 0 oder 1)


Für die "display_line" Kanäle, welche je eine Zeile des Displays repräsentieren, kann mit FHEM ein Wert übertragen werden (siehe FHEM RAW Befehle weiter unten) oder es wird einer der vordefinierten Zeilen angezeigt.
Wenn für eine Zeile "Innen: %1%°C" gesetzt ist, dann wird %1% mit dem Wert des ersten display_v_* Kanals ersetzt. Bei einem Temperaturmesswert würde dann z.B. im Display "Innen: 22.4°C" angezeigt. Mit einem Key (Taster) Peering kann zwischen der vordefinierten Zeile und der per FHEM gesetzten umgeschaltet werden.

Die 8 display_v_* Kanäle können über die Platzhalter %1% bis %8% angezeigt werden. Erhalten diese Kanäle neue Werte (über FHEM oder Peering), so wird dies im Display automatisch aktualisiert.
Für "display_v_temp" Kanäle können ein Faktor (1; 10; 100; 1000) und Anzeigeformat (999; 999.9; 999.99; 99.999) konfiguriert werden.
Für "display_v_switch" Kanäle ein Anzeigetext, z.B. Ein/Aus, Auf/Zu, Auto/Manu.



# RAW Befehle #
Zeile 1 setzen:
{my $hstring = unpack ("H*","Innen: %1%\xDFC");; fhem "set HBW_DIS_Key_4_HBW7296375 raw 7302$hstring"}
(\xDF hex Wert für "°" - abhängig vom Zeichensatz des Displays)
Zeile 2 setzen:
{my $hstring = unpack ("H*","Test2: %02%");; fhem "set HBW_DIS_Key_4_HBW7296375 raw 7303$hstring"}

73 - "set" Befehl
02 - Kanal 2 des Device, muss entsprechend der Displayzeile angepasst werden. (display_line Kanäle)
$hstring - Der zu sendende Text, als Hex String (hexstring_bytearray) umgewandelt




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
12 - LCD_RS
11 - LCD_EN
10 - LCD_D4
9  - LCD_D5
7  - LCD_D6
6  - LCD_D7
5  - LCD_BACKLIGHT (PWM Pin!)
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
12 - LCD_RS
11 - LCD_EN
10 - LCD_D4
9  - LCD_D5
7  - LCD_D6
6  - LCD_D7
5  - LCD_BACKLIGHT (PWM Pin!)
A7 - LDR (Photo R (ADC Pin!))


