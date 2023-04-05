#ifndef HBW_Sen_EP_interrupt_h
#define HBW_Sen_EP_interrupt_h


#define EI_ARDUINO_INTERRUPTED_PIN
#include <EnableInterrupt.h>


/* arduinoPinState (included from EnableInterrupt.h, by #define EI_ARDUINO_INTERRUPTED_PIN)
   The value of the variable is either 0 or some power of two (i.e., 2, 4, 8).
   0 means that the pin changed from high to low signal.
   Other than 0 means the pin went from a low signal level to high.
*/

/* #if defined __AVR_ATmega168__ || defined __AVR_ATmega168A__ || defined __AVR_ATmega168P__ || \
   defined __AVR_ATmega168PA__ || defined __AVR_ATmega328__ || defined __AVR_ATmega328P__
*/

#define PINCOUNT(x) pin ##x ##Count
#define PINTIMER(x) pin ##x ##LastMillis


// approx. 14 ms debonce (standard S0 puls lenght 30 ms)
// input pulse will increment counter on next pin change interrupt - input was held LOW for debounce duration or more
#define interruptFunction(x) \
  volatile uint16_t PINCOUNT(x); \
  volatile uint32_t PINTIMER(x); \
  void interruptFunction ##x () { \
    if (arduinoPinState != LOW && millis() - PINTIMER(x) >= 14) { PINCOUNT(x)++; } \
    PINTIMER(x) = millis(); \
  }


#define setupInterrupt(x) \
  pinMode( x, INPUT_PULLUP); \
  enableInterrupt( x, interruptFunction##x, CHANGE)


#define getInterruptCounter(x) \
  PINCOUNT(x)


// #else
// #error This sketch supports 328-based Arduinos only at the moment. Possible to add others that support port change interrupts.
// #endif


#endif  //HBW_Sen_EP_interrupt_h
