/*
 * FreeRam.cpp
 *
 *  Created on: 210.11.2016
 *      Author: Thorsten Pferdekaemper thorsten@pferdekaemper.com
 */

#include "FreeRam.h"

int freeRam ()
{
#if defined (__AVR__)
  extern int __heap_start, *__brkval;
  int v;
  return (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval);
#elif defined (ARDUINO_ARCH_RP2040)
  extern char __StackLimit, __bss_end__;
  struct mallinfo mi = mallinfo();
  return (int)((uint32_t)(&__StackLimit  - &__bss_end__) - mi.uordblks);
#else
  return -1;
#endif
};
