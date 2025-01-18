/*
 * FreeRam.h
 *
 *  Created on: 21.11.2016
 *      Author: Thorsten Pferdekaemper
 */

#ifndef FreeRam_h
#define FreeRam_h

#include "Arduino.h"

#if defined (ARDUINO_ARCH_RP2040)
  #include <malloc.h>
#endif

int freeRam ();

#endif /* FreeRam_h */
