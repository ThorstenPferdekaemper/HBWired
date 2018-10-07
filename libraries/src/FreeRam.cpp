/*
 * FreeRam.cpp
 *
 *  Created on: 210.11.2016
 *      Author: Thorsten Pferdekaemper thorsten@pferdekaemper.com
 */

#include "FreeRam.h"

int freeRam ()
{
  extern int __heap_start, *__brkval;
  int v;
  return (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval);
}; 
