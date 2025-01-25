Homematic Wired Homebrew Relaismodul 12fach
===========================================

Das Modul HBW-LC-Sw-12 schaltet bis zu 12 angeschlossene bistabile Relais. (per Shiftregister)
Basis ist ein Arduino NANO mit RS485-Interface.


! Die 6 Kanal Strommessung ist noch in der Entwicklung!

Direktes Peering m�glich (HBWLinkSwitchAdvanced).
Peering mit TOGGLE, TOGGLE_TO_COUNTER, TOGGLE_INVERSE_TO_COUNTER, onTime, offTime (Ein-/Ausschaltdauer), onDelayTime, offDelayTime (Ein-/Ausschaltverz�gerung).
Maximale Ein-/Ausschaltdauer, Ein-/Ausschaltverz�gerung, jeweils: 63000 Sekunden (17,5 Stunden).

Damit FHEM das Homebrew-Device richtig erkennt, muss die HBW-LC-Sw-12.xml Datei in den Ordner FHEM/lib/HM485/Devices/xml kopiert werden (Das Device gibt sich als HW-Typ 0x93 aus).

Standard-Pinbelegung:
4 - Rx RS485
2 - Tx RS485
3 - RS485 Enable
13 - Status LED
0 - Rx Debug
1 - Tx Debug
8 - Bedientaster (Reset)
12 - shiftReg_OutputEnable (OE, output enable f�r alle Shiftregister)
10 - shiftRegOne_Data   (Drei Shiftregister 74HC595 f�r LEDs und Relais)
9 - shiftRegOne_Clock
11 - shiftRegOne_Latch
5 - shiftRegTwo_Data   (Drei weitere Shiftregister 74HC595 f�r LEDs und Relais, in eigenem 6 TE Geh�use)
7 - shiftRegTwo_Clock
6 - shiftRegTwo_Latch


Alternative M�glichkeit, per "#define USE_HARDWARE_SERIAL" aktivieren:
Hier wird Hardware Serial (USART) statt "HBWSoftwareSerial" genutzt, daher keine Debug Ausgabe �ber USB! Der Bedientaster (Reset) ist ein Analogeingang!

Pinbelegung:
0 - Rx RS485
1 - Tx RS485
2 - RS485 Enable
13 - Status LED
A6 - Bedientaster (Reset)
8 - shiftReg_OutputEnable (OE, output enable f�r alle Shiftregister)
10 - shiftRegOne_Data   (Drei Shiftregister 74HC595 f�r LEDs und Relais)
3 - shiftRegOne_Clock
4 - shiftRegOne_Latch
7 - shiftRegTwo_Data   (Drei weitere Shiftregister 74HC595 f�r LEDs und Relais, in eigenem 6 TE Geh�use)
12 - shiftRegTwo_Clock
9 - shiftRegTwo_Latch
A0 - Stromsensor?
A1 - Stromsensor?
A2 - Stromsensor?
A3 - Stromsensor?
A4 - Stromsensor?
A5 - Stromsensor?
A6 - Busspannung
