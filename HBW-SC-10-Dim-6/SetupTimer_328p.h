/* 
 * Setup PWM, to have 4 channels with 122Hz output,
 * to interface with Eltako LUD12-230V (ideally 100HZ @ min. 10V)
*
* http://loetmeister.de/Elektronik/homematic/
*
*/

#ifndef SetupTimer_328p_h
#define SetupTimer_328p_h


  //change from fast-PWM to phase-correct PWM
  // (This is the timer0 controlled PWM module. Do not change prescaler, it would impact millis() & delay() functions.)
  //TCCR0A = _BV(COM0A1) | _BV(COM0B1) | _BV(WGM00);
//  TCCR0A = B00000001; // phase-correct PWM @490Hz
// TODO: fixme - not working! millis() is running two times slower when not in fast-PWM! - interrupt 'issue'


inline void setupPwmTimer1(void) {
  // Setup Timer1 for 122.5Hz PWM
  TCCR1A = 0;   // undo the configuration done by...
  TCCR1B = 0;   // ...the Arduino core library
  TCNT1  = 0;   // reset timer
  TCCR1A = _BV(COM1A1)  // non-inverted PWM on ch. A
         | _BV(COM1B1)  // same on ch. B
         | _BV(WGM11);  // mode 10: ph. correct PWM, TOP = ICR1
  TCCR1B = _BV(WGM13)   // ditto
  //| _BV(WGM12) // fast PWM: mode 14, TOP = ICR1
      | _BV(CS12);   // prescaler = 256
  //ICR1 = 312;  // 100Hz //TODO: ? create custom analogWrite() to consider ICR1 as upper limit: e.g. OCR1A = map(val,0,255,0,ICR1);
  ICR1 = 255; // 122.5Hz - this work with default analogWrite() function (which is setting a level between 0 and 255)
}


inline void setupPwmTimer2(void) {
  // Timer 2 @122.5Hz
    TCNT2  = 0;
    TCCR2A = B10100001; // mode 1: ph. correct  PWM, TOP = OxFF
    TCCR2B = B00000110;   // prescaler = 256
}

#endif


/*
https://www.arduino.cc/en/Tutorial/SecretsOfArduinoPWM

// For Arduino Uno, Nano, Micro Magician, Mini Driver, Lilly Pad and any other board using ATmega 8, 168 or 328**
 
//---------------------------------------------- Set PWM frequency for D5 & D6 -------------------------------
 
//TCCR0B = TCCR0B & B11111000 | B00000001;    // set timer 0 divisor to     1 for PWM frequency of 62500.00 Hz
//TCCR0B = TCCR0B & B11111000 | B00000010;    // set timer 0 divisor to     8 for PWM frequency of  7812.50 Hz
  TCCR0B = TCCR0B & B11111000 | B00000011;    // set timer 0 divisor to    64 for PWM frequency of   976.56 Hz (The DEFAULT)
//TCCR0B = TCCR0B & B11111000 | B00000100;    // set timer 0 divisor to   256 for PWM frequency of   244.14 Hz
//TCCR0B = TCCR0B & B11111000 | B00000101;    // set timer 0 divisor to  1024 for PWM frequency of    61.04 Hz
 
 
//---------------------------------------------- Set PWM frequency for D9 & D10 ------------------------------
 
//TCCR1B = TCCR1B & B11111000 | B00000001;    // set timer 1 divisor to     1 for PWM frequency of 31372.55 Hz
//TCCR1B = TCCR1B & B11111000 | B00000010;    // set timer 1 divisor to     8 for PWM frequency of  3921.16 Hz
  TCCR1B = TCCR1B & B11111000 | B00000011;    // set timer 1 divisor to    64 for PWM frequency of   490.20 Hz (The DEFAULT)
//TCCR1B = TCCR1B & B11111000 | B00000100;    // set timer 1 divisor to   256 for PWM frequency of   122.55 Hz
//TCCR1B = TCCR1B & B11111000 | B00000101;    // set timer 1 divisor to  1024 for PWM frequency of    30.64 Hz
 
//---------------------------------------------- Set PWM frequency for D3 & D11 ------------------------------
 
//TCCR2B = TCCR2B & B11111000 | B00000001;    // set timer 2 divisor to     1 for PWM frequency of 31372.55 Hz
//TCCR2B = TCCR2B & B11111000 | B00000010;    // set timer 2 divisor to     8 for PWM frequency of  3921.16 Hz
//TCCR2B = TCCR2B & B11111000 | B00000011;    // set timer 2 divisor to    32 for PWM frequency of   980.39 Hz
  TCCR2B = TCCR2B & B11111000 | B00000100;    // set timer 2 divisor to    64 for PWM frequency of   490.20 Hz (The DEFAULT)
//TCCR2B = TCCR2B & B11111000 | B00000101;    // set timer 2 divisor to   128 for PWM frequency of   245.10 Hz
//TCCR2B = TCCR2B & B11111000 | B00000110;    // set timer 2 divisor to   256 for PWM frequency of   122.55 Hz
//TCCR2B = TCCR2B & B11111000 | B00000111;    // set timer 2 divisor to  1024 for PWM frequency of    30.64 Hz


bitSet(TCCR1B, WGM12);
Just adding that single-bit change to setup() will force timer 1 into Fast PWM mode as well.

*/