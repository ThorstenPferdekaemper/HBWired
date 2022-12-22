/* UC121902-TNARX-A
 *
 * The display: http://www.pollin.de/shop/dt/MzE0OTc4OTk-/Bauelemente_Bauteile/Aktive_Bauelemente/Displays/LCD_Modul_SAMSUNG_UC121902_TNARX_A.html
 * How to install: http://www.arduino.cc/en/Guide/Libraries
 *
 * License: MIT Copyright (c) 2015 Nicco Kunzmann
 *
 * The Repository: https://github.com/niccokunzmann/UC121902-TNARX-A
 *
 */

#ifndef UC121902_TNARX_A_hpp
#define UC121902_TNARX_A_hpp

// save space, by only supporting numbers and a few other characters (sign, etc.)
#define UC121902_TNARX_A_ONLY_USE_NUMBERS

#include "Arduino.h"

namespace UC121902_TNARX_A {

  uint8_t DP = 4;
  uint8_t DQ = 2;
  uint8_t DR = 1;

 #ifndef UC121902_TNARX_A_ONLY_USE_NUMBERS
  const uint8_t segment_lookup_table_size = 127;
 #else
  const uint8_t segment_lookup_table_size = 27;
 #endif
  const uint8_t PROGMEM segment_lookup_table[segment_lookup_table_size] = {
 #ifndef UC121902_TNARX_A_ONLY_USE_NUMBERS
    0b1011100 /* '\x00' */,
    0b1011100 /* '\x01' */,
    0b1011100 /* '\x02' */,
    0b1011100 /* '\x03' */,
    0b1011100 /* '\x04' */,
    0b1011100 /* '\x05' */,
    0b1011100 /* '\x06' */,
    0b1011100 /* '\x07' */,
    0b1011100 /* '\x08' */,
    0b0000000 /*  '\t'  */,
    0b0000000 /*  '\n'  */,
    0b1011100 /* '\x0b' */,
    0b1011100 /* '\x0c' */,
    0b0000000 /*  '\r'  */,
    0b1011100 /* '\x0e' */,
    0b1011100 /* '\x0f' */,
    0b1011100 /* '\x10' */,
    0b1011100 /* '\x11' */,
    0b1011100 /* '\x12' */,
    0b1011100 /* '\x13' */,
    0b1011100 /* '\x14' */,
    0b1011100 /* '\x15' */,
    0b1011100 /* '\x16' */,
    0b1011100 /* '\x17' */,
    0b1011100 /* '\x18' */,
    0b1011100 /* '\x19' */,
    0b1011100 /* '\x1a' */,
    0b1011100 /* '\x1b' */,
    0b1011100 /* '\x1c' */,
    0b1011100 /* '\x1d' */,
    0b1011100 /* '\x1e' */,
    0b1011100 /* '\x1f' */,
 #endif
    0b0000000 /*  ' '   */,
    0b0010010 /*  '!'   */,
    0b0110000 /*  '"'   */,
    0b1011100 /*  '#'   */,
    0b1011100 /*  '$'   */,
    0b1001001 /*  '%'   */,
    0b1011100 /*  '&'   */,
    0b0100000 /*  "'"   */,
    0b1100101 /*  '('   */,
    0b1010011 /*  ')'   */,
    0b1011100 /*  '*'   */,
    0b1011100 /*  '+'   */,
    0b0000010 /*  ','   */,
    0b0001000 /*  '-'   */,
    0b0000001 /*  '.'   */,
    0b0011100 /*  '/'   */,
    0b1110111 /*  '0'   */,
    0b0010010 /*  '1'   */,
    0b1011101 /*  '2'   */,
    0b1011011 /*  '3'   */,
    0b0111010 /*  '4'   */,
    0b1101011 /*  '5'   */,
    0b1101111 /*  '6'   */,
    0b1010010 /*  '7'   */,
    0b1111111 /*  '8'   */,
    0b1111011 /*  '9'   */,
    0b0001001 /*  ':'   */,
 #ifndef UC121902_TNARX_A_ONLY_USE_NUMBERS
    0b0001010 /*  ';'   */,
    0b1101000 /*  '<'   */,
    0b0001001 /*  '='   */,
    0b1011000 /*  '>'   */,
    0b1011100 /*  '?'   */,
    0b1011100 /*  '@'   */,
    0b1111110 /*  'A'   */,
    0b0101111 /*  'B'   */,
    0b1100101 /*  'C'   */,
    0b0011111 /*  'D'   */,
    0b1101101 /*  'E'   */,
    0b1101100 /*  'F'   */,
    0b1100111 /*  'G'   */,
    0b0111110 /*  'H'   */,
    0b0100100 /*  'I'   */,
    0b0000010 /*  'J'   */,
    0b0101101 /*  'K'   */,
    0b0100101 /*  'L'   */,
    0b1110110 /*  'M'   */,
    0b1110110 /*  'N'   */,
    0b1110111 /*  'O'   */,
    0b1111100 /*  'P'   */,
    0b1111010 /*  'Q'   */,
    0b1111110 /*  'R'   */,
    0b1101011 /*  'S'   */,
    0b1100100 /*  'T'   */,
    0b0110111 /*  'U'   */,
    0b0110111 /*  'V'   */,
    0b0110111 /*  'W'   */,
    0b0111110 /*  'X'   */,
    0b0111010 /*  'Y'   */,
    0b1011101 /*  'Z'   */,
    0b1100101 /*  '['   */,
    0b0101010 /*  '\\'  */,
    0b1100101 /*  ']'   */,
    0b1000000 /*  '^'   */,
    0b0000001 /*  '_'   */,
    0b0010000 /*  '`'   */,
    0b1111110 /*  'a'   */,
    0b0101111 /*  'b'   */,
    0b0001101 /*  'c'   */,
    0b0011111 /*  'd'   */,
    0b1101101 /*  'e'   */,
    0b1101100 /*  'f'   */,
    0b1100111 /*  'g'   */,
    0b0101110 /*  'h'   */,
    0b0000100 /*  'i'   */,
    0b0000010 /*  'j'   */,
    0b0101101 /*  'k'   */,
    0b0100100 /*  'l'   */,
    0b0001110 /*  'm'   */,
    0b0001110 /*  'n'   */,
    0b0001111 /*  'o'   */,
    0b1111100 /*  'p'   */,
    0b1111010 /*  'q'   */,
    0b0001100 /*  'r'   */,
    0b1101011 /*  's'   */,
    0b0101100 /*  't'   */,
    0b0000111 /*  'u'   */,
    0b0000111 /*  'v'   */,
    0b0000111 /*  'w'   */,
    0b0111110 /*  'x'   */,
    0b0111010 /*  'y'   */,
    0b1011101 /*  'z'   */,
    0b1100101 /*  '{'   */,
    0b0010010 /*  '|'   */,
    0b1010011 /*  '}'   */,
    0b0001000 /*  '~'   */,
 #endif
  };
  
  uint8_t segmentToByte(uint8_t segment) {
    /*
      6     -               
      5   | | 4   
      3     -          
      2   | | 1
      0     -   

      x[2]  -               
      x[0] | | x[4]   
      x[3]  -          
      x[1] | | x[7]
      x[5]  -  
    */
    uint8_t byte = 0;
    uint8_t data_offset[7] = {2, 0, 4, 3, 1, 7, 5};
    for (int i = 0; i < 7; i++) {
      uint8_t segment_mask = 1 << (6 - i);
      if (segment & segment_mask) {
        byte |=  1 << data_offset[i];
      }
    }
    return byte;
  }

  class State {
    private:
      int m_CE_pin;
      int m_CK_pin;
      int m_DI_pin;

    public:
      /*  uint8_t data[14]
          ================
        data = {a, b, c, d, e, f, g, h, i, j, k, l, m, n}
                0  1  2  3  4  5  6  7  8  9 10 11 12 13
                x  x  x  x  x  x     x  x  x  x  x  x
        
        Definition: byte b = data[...]; b[x] means (b & (1 << x)) >> x 
        
        7 Segment Display:
        
            x[2]  -               
            x[0] | | x[4]   
            x[3]  -          
            x[1] | | x[7]
            x[5]  -   

            bits used for x: 1011 1111
                             7 54 3210
                             bit 6 is not in use
        
        Special elements:
         
            g[0] = ?         n[0] = ?
            g[1] = ?         n[1] = ?
            g[2] = ?         n[2] = ?
            g[3] = ?         n[3] = ?
            g[4] = CHAN      n[4] = BATTERY_SECOND_HALF
            g[5] = MEM       n[5] = BATTERY_FIRST_HALF
            g[6] = /BELL    n[6] = PROG
            g[7] = BELL     n[7] = SEC
      */
      uint8_t m_data[14]; /* 2 * 56bits */
      boolean writeOnChange;
      boolean started;

      State(int CE_pin = 2, int CK_pin = 3, int DI_pin = 4) {
        m_CE_pin = CE_pin;
        m_CK_pin = CK_pin;
        m_DI_pin = DI_pin;
        started = false;
        writeOnChange = true;
        erase();
      }
      
      void begin() {
        pinMode(m_CE_pin, OUTPUT);
        pinMode(m_CK_pin, OUTPUT);
        pinMode(m_DI_pin, OUTPUT);
        started = true;
        flush();
      }
      
      void end() {
        started = false;
      }

      void write_bits(uint8_t data[7]) {
        /* assuming length 7 */
        digitalWrite(m_CK_pin, LOW);
        digitalWrite(m_DI_pin, LOW);
        delayMicroseconds(1); // t1, at least 1 microsecond
        digitalWrite(m_CE_pin, HIGH); // t3 must be at least 4 microseconds HIGH
        delayMicroseconds(1); // t1, at least 1 microsecond
        for(int data_index = 0; data_index < 7; data_index++) {
          uint8_t data_byte = data[data_index];
          for (uint8_t bit_mask = 128; bit_mask > 0; bit_mask >>= 1) {
            delayMicroseconds(1); // tSU, at least 1 microseconds and tSU
            digitalWrite(m_DI_pin, data_byte & bit_mask ? HIGH : LOW);
            digitalWrite(m_CK_pin, HIGH);
            delayMicroseconds(1); // tDH, at least 0.25 microseconds
            digitalWrite(m_CK_pin, LOW);
          }
        }
        delayMicroseconds(1);
        digitalWrite(m_DI_pin, LOW);
        delayMicroseconds(1); // t2 - tDH; t2 at least 1.25 microseconds
        digitalWrite(m_CE_pin, LOW); 
      }

      void flush() {
        if (!started) return;
        uint8_t data1[7];
        uint8_t data2[7];
        for (int i = 0; i < 7; i++) data1[i] = m_data[i];
        for (int i = 0; i < 7; i++) data2[i] = m_data[7 + i];
        data1[6] &= 248;
        data1[6] |= DP;
        data2[6] &= 248;
        data2[6] |= DR;
        write_bits(data1);
        delayMicroseconds(4); // t4 at least 4 microseconds
        write_bits(data2);
      }
      
      uint8_t* data() {
        return m_data;
      }
      
      void changed() {
        if (writeOnChange) {
          flush();
        }
      }
      
      void erase() {
        for (int i = 0; i < 14; i++) {
          m_data[i] = 0;
        }
      }
      void eraseNumbers() {
        for (int i = 0; i < 6; i++) {
          m_data[i] = m_data[i + 7] = 0;
        }
      }
      
      void set(uint8_t segment, int position) {
        if (position < 0) position = 12 + position;
        if (position < 0 || position >= 12) return;
        if (position >= 6) position ++;
        m_data[12 - position] = segmentToByte(segment);
      }
  };

  class Switch {
    private: 
      State* state;
      int byte_index;
      int bit_index;
    public: 
      Switch(State* state = NULL, int byte_index = 0, int bit_index = 0) : 
        state(state), byte_index(byte_index), bit_index(bit_index) {
      }
      ~Switch () {
        
      }
      
      void turnOn() {
        state->data()[byte_index] |= 1 << bit_index;
        state->changed();
      }
      
      void turnOff() {
        state->data()[byte_index] &= 255 - (1 << bit_index);
        state->changed();
      }
      
      boolean isOn() {
        return state->data()[byte_index] & (1 << bit_index);
      }
      
      boolean isOff() {
        return !isOn();
      }
      
      void toggle() {
        if (isOn()) {
          turnOff();
        } else {
          turnOn();
        }
      }
      
  };
  
  class Bell {
    public:
      Switch muted;
      Switch bell;
      Bell(State* state = NULL) {
        muted = Switch(state, 6, 6);
        bell = Switch(state, 6, 7);
      }
      void turnOn() {
        bell.turnOn();
      }
      void turnOff() {
        bell.turnOff();
      }
      boolean isOn() {
        return bell.isOn();
      }
      boolean isOff() {
        return bell.isOff();
      }
      void toggle() {
        bell.toggle();
      }
      void mute() {
        muted.turnOn();
      }
      void unmute() {
        muted.turnOff();
      }
      boolean isMuted() {
        return muted.isOn();
      }
      boolean isUnmuted() {
        return muted.isOff();
      }
      void toggleMuted() {
        muted.toggle();
      }
  };
  
  class Battery {
    public:
      Switch  first_half;
      Switch second_half;
      Battery(State* state = NULL) {
        first_half  = Switch(state, 13, 5);
        second_half = Switch(state, 13, 4);
      }
      void empty() {
        first_half.turnOff();
        second_half.turnOff();
      }
      boolean isEmpty() {
          return first_half.isOff() && second_half.isOff();
      }
      void full() {
        first_half.turnOn();
        second_half.turnOn();
      }
      boolean isFull() {
          return first_half.isOn() && second_half.isOn();
      }
      void halfFull() {
        first_half.turnOn();
        second_half.turnOff();
      }
      boolean isHalfFull() {
          return first_half.isOn() && second_half.isOff();
      }
      void turnOff() {
        empty();
      }
      boolean isOff() {
        return isEmpty();
      }
      void turnOn() {
        full();
      }
      boolean isOn() {
        return isFull();
      }
      void toggle() {
        if (isOff()) {
          turnOn();
        } else {
          turnOff();
        }
      }
  };
  
  class Display {
    private:
      State* state;
      
    public: 
      Switch chan;
      Switch mem;
      Switch prog;
      Switch sec;
      Bell bell;
      Battery battery;
      
      Display(int CE_pin = 2, int CK_pin = 3, int DI_pin = 4) {
        state = new State(CE_pin, CK_pin, DI_pin);
        
        chan = Switch(state, 6, 4);
        mem = Switch(state, 6, 5);
        prog = Switch(state, 13, 6);
        sec = Switch(state, 13, 7);
        bell = Bell(state);
        battery = Battery(state);
      }
      ~Display() {
        delete(state);
      }
      
      boolean writeOnChange() {
        return state->writeOnChange;
      }
      void writeOnChange(boolean value) {
        state->writeOnChange = value;
      }
      
      void begin() {
        state->begin();
      }
      
      void end() {
        state->end();
      }
      
      void flush() {
        state->flush();
      }
      void turnOff() {
        eraseAll();
      }
      void eraseAll() {
        state->erase();
        state->changed();
      }
      
      void eraseNumbers() {
        state->eraseNumbers();
        state->changed();
      }
      
      void eraseText() {
        eraseNumbers();
      }
      
      void print(const String string) {
        eraseText();
        put(string);
        state->changed();
      }
      
      void print(int32_t number) {
        eraseText();
        put(number);
        state->changed();
      }
      
      void print(char character, int position) {
        put(character, position);
        state->changed();
      }
      
      void print(const String string, int32_t number) {
        eraseText();
        put(string);
        put(number);
        state->changed();
      }
       
      void put(const String string) {
        for (unsigned char i = 0; (i < string.length() && i < 12); i++) {
          put(string[i], i);
        }      
      }
      
      void put(uint16_t number) {
        int position = 11;  // right "justified"
        // 65,535 has 5 digits
        do {
          put('0' + number % 10, position);
          number /= 10;
          position --;
        } while (number);
      }
            
      void put(int32_t number) {
        boolean sign = number < 0;
        number = abs(number);
        int position = 11;
        // -2,147,483,648 has 10 digits
        do {
          put('0' + number % 10, position);
          number /= 10;
          position --;
        } while (number);
        put(sign ? '-' : ' ', position);
      }
    
      void put(char character, int position) {
       #ifndef UC121902_TNARX_A_ONLY_USE_NUMBERS
        if (character > segment_lookup_table_size) {
          character = '?';
       #else
        if (character -32 > segment_lookup_table_size) {
          character = '.';
       #endif
        }
       #ifndef UC121902_TNARX_A_ONLY_USE_NUMBERS
        state->set(pgm_read_byte(&segment_lookup_table[(unsigned char)character]), position);
       #else
        state->set(pgm_read_byte(&segment_lookup_table[(unsigned char)character -32]), position);
       #endif
      }
      
      void put(                    boolean top, 
                  boolean topLeft,               boolean topRight,
                                  boolean middle,
               boolean bottomLeft,               boolean bottomRight,
                                  boolean bottom, 
               int position) {
        /*
          T     -               
          TL   | | TR   
          M     -          
          BL   | | BR
          B     -   
        */
        uint8_t byte = 0;
        if (top        ) byte |= 1 << 6;
        if (topLeft    ) byte |= 1 << 5;
        if (topRight   ) byte |= 1 << 4;
        if (middle     ) byte |= 1 << 3;
        if (bottomLeft ) byte |= 1 << 2;
        if (bottomRight) byte |= 1 << 1;
        if (bottom     ) byte |= 1 << 0;
        state->set(byte, position);
      }
      
      void putSegment(uint8_t byte, int position) {
        /* see the segment_lookup_table for examples 
          byte     0b1000000  0b1110111  0b0101001  0b0010010
          segment      _          _                       
                                 | |       |_            |
                                 |_|        _            |
                    
          position is 0 (left) to 11 (right)
        */
        state->set(byte, position);
      }
      
  };
};






#endif
