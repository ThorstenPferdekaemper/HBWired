###### Under development!!! Not tested!!! ######
###### In Entwicklung!!! Nicht getestet!!! #####

Homematic Wired Homebrew Relaismodul 12fach
==============================================

Das Modul HBW-LC-Sw-12 schaltet bis zu 12 angeschlossene bistabile Relais.
Basis ist ein Arduino NANO mit RS485-Interface.
(Hinweis Hex Dateien: Geeigent für Atmel ATMEGA328p! *with_bootloader* ist ebenfalls für ATMEGA328p, aber inkl. dem Arduino NANO Bootloader!)

Direktes Peering möglich (HBWLinkSwitchSimple).

Damit FHEM das Homebrew-Device richtig erkennt, muss die HBW-LC-Sw-12.xml Datei in den Ordner FHEM/lib/HM485/Devices/xml kopiert werden (Das Device gibt sich als HW-Typ 0x93 aus).

Standard-Pinbelegung:
4 - Rx RS485
2 - Tx RS485
3 - RS485 Enable
13 - Status LED
0 - Rx Debug
1 - Tx Debug
8 - Bedientaster (Reset)
10 - shiftRegOne_Data   (Drei Shiftregister 74HC595 für LEDs und Relais)
9 - shiftRegOne_Clock
11 - shiftRegOne_Latch
5 - shiftRegTwo_Data   (Drei weitere Shiftregister 74HC595 für LEDs und Relais, in eigenem 6 TE Gehäuse)
7 - shiftRegTwo_Clock
6 - shiftRegTwo_Latch


Altenative Möglichkeit, per "#define USE_HARDWARE_SERIAL" aktivieren:
Hier wird Hardware Serial (USART) statt "HBWSoftwareSerial" genutzt! Der Bedientaster (Reset) ist ein Analogeingang!

Pinbelegung:
0 - Rx RS485
1 - Tx RS485
2 - RS485 Enable
13 - Status LED
A6 - Bedientaster (Reset)
10 - shiftRegOne_Data   (Drei Shiftregister 74HC595 für LEDs und Relais)
3 - shiftRegOne_Clock
4 - shiftRegOne_Latch
12 - shiftRegTwo_Data   (Drei weitere Shiftregister 74HC595 für LEDs und Relais, in eigenem 6 TE Gehäuse)
8 - shiftRegTwo_Clock
9 - shiftRegTwo_Latch
A0 - Stromsensor?
A1 - Stromsensor?
A2 - Stromsensor?
A3 - Stromsensor?
A4 - Stromsensor?
A5 - Stromsensor?
