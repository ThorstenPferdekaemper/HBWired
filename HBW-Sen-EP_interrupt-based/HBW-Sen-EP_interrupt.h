#ifndef HBW_Sen_EP_interrupt_h
#define HBW_Sen_EP_interrupt_h


#include <EnableInterrupt.h>


/* #if defined __AVR_ATmega168__ || defined __AVR_ATmega168A__ || defined __AVR_ATmega168P__ || \
   defined __AVR_ATmega168PA__ || defined __AVR_ATmega328__ || defined __AVR_ATmega328P__
*/

#define PINCOUNT(x) pin ##x ##Count
#define PINTIMER(x) pin ##x ##Millis


// 14 ms debonce (standard S0 puls lenght 30 ms)
// debonce inside of interrupt function causes one count to be missed after startup - should be fine
#define interruptFunction(x) \
  volatile uint16_t PINCOUNT(x); \
  volatile uint32_t PINTIMER(x); \
  void interruptFunction ##x () { \
    if (millis() - PINTIMER(x) > 14)  PINCOUNT(x)++; \
    PINTIMER(x) = millis(); \
  }


#define setupInterrupt(x) \
  pinMode( x, INPUT_PULLUP); \
  enableInterrupt( x, interruptFunction##x, FALLING)


#define getInterruptCounter(x) \
  PINCOUNT(x)


// #else
// #error This sketch supports 328-based Arduinos only at the moment. Possible to add others that support port change interrupts.
// #endif


#endif  //HBW_Sen_EP_interrupt_h
