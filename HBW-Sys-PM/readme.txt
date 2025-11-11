Homematic Wired Homebrew Busüberwachungsmodul
=============================================

Das Modul HBW-Sys-PM misst die Busspannung, Strom und Leistung. Es bietet darüber hinaus 4 Ausgänge und 2 Eingänge.
Basis ist ein Arduino NANO mit RS485-Interface. Das Messmodul ist ein SBCDVA mit INA236 I²C chip. (0 - 48 V DC & 0 - 8 A; 16-Bit) https://joy-it.net/de/products/SBC-DVA

Direktes Peering der Ausgänge möglich mit "HBWSwitchAdvanced & HBWLinkSwitchAdvanced"

Damit FHEM das Homebrew-Device richtig erkennt, muss die hbw-sys-pm.xml Datei in den Ordner FHEM/lib/HM485/Devices/xml kopiert werden (Das Device gibt sich als HW-Typ 0x8D aus).

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
A3 - Input 2 (oder 1 Onewire Bestückung möglich)


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
