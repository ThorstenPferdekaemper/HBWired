Homematic Wired Homebrew "Generic"
==================================

Das Modul HBW-Generic hat keine direkte Funktion. Es kann aber als Vorlage benutzt werden und dient dazu ein neues Gerät direkt am Bus über den HBW-Bootloader zu installieren / die korrekte Geräteadresse einzurichten.
D.h. das neue Gerät wird per ISP mit nur dem HBW-Bootloader geflasht. Es ist dann mit der Standardadresse 0x42FFFFFF am Bus erreichbar (Typ HMW-Generic in FEHM). Dann kann in FEHM /CCU ein Firmware update auf HBW-Generic durchgeführt werden. Mit diesem Gerätetyp lässt sich die Geräteadresse setzten. Danach kann die eigentliche Gerätefirmware hochgeladen werden. (mit FHEM ist der Weg über HBW-Generic nicht nötig. Siehe raw-Befehl Details in documentation/quick-setup.txt für eine direkte Methode die Geräteadresse zu ändern.)

Basis ist ein Arduino NANO mit RS485-Interface.
Damit FHEM das Homebrew-Device richtig erkennt, muss die Datei hbw-generic.xml in den Ordner \FHEM\lib\HM485\Devices\xml kopiert werden (Das Device gibt sich als HW-Typ 0xFF aus).

Config des Geräts kann über das FHEM-Webfrontend vorgenommen werden:
# OWN_ADDRESS (Eigene Geräteadresse, Wert zwischen 1000 und 4294966295. Werte kleiner ~500000 sind verm. von original Geräten belegt)


Debug-Pinbelegung:
0 - Rx Debug, Serial -> USB
1 - Tx Debug, Serial -> USB
3 - RS485 Enable
13 - Status LED
4 - Rx Debug
2 - Tx Debug
8 - Bedientaster (Reset)


Alternative Möglichkeit, per "#define USE_HARDWARE_SERIAL" aktivieren:
Hier wird Hardware Serial (USART) statt "HBWSoftwareSerial" genutzt, daher keine Debug Ausgabe über USB! Der Bedientaster (Reset) ist ein Analogeingang!

Standard-Pinbelegung:
0 - Rx RS485
1 - Tx RS485
2 - RS485 Enable
13 - Status LED
A6 - Bedientaster (Reset)

