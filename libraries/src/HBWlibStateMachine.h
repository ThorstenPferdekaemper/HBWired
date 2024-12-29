/* 
* HBWlibStateMachine
*
* Support lib for state machine
*
* http://loetmeister.de/Elektronik/homematic/
*
*/

#ifndef HBWlibStateMachine_h
#define HBWlibStateMachine_h

#include <inttypes.h>
#include <Arduino.h>



#define DELAY_NO 0x00
#define DELAY_INFINITE 0xffffffff

#define ON_LEVEL_PRIO_HIGH  0
#define ON_LEVEL_PRIO_LOW   1

#define BITMASK_ActionType        0b00001111
#define BITMASK_LongMultiexecute  0b00100000
#define BITMASK_OffTimeMode       0b01000000
#define BITMASK_OnTimeMode        0b10000000

#define BITMASK_OnDelayMode   0b00000001
#define BITMASK_OnLevelPrio   0b00000010
#define BITMASK_OffDelayBlink 0b00000100
#define BITMASK_RampStartStep 0b11110000



/* convert time value stored in EEPROM to milliseconds - 1 byte value */
inline uint32_t convertTime(uint8_t timeValue) {
  
  uint8_t factor = timeValue & 0xC0;    // mask out factor (higest two bits)
  timeValue &= 0x3F;    // keep time value only

  // factors: 1,60,1000,6000 (last one is not used)
  switch (factor) {
    case 0:          // x1
      return (uint32_t)timeValue *1000;
      break;
    case 64:         // x60
      return (uint32_t)timeValue *60000;
      break;
    case 128:        // x1000
      return (uint32_t)timeValue *1000000;
      break;
 //    case 192:        // not used value
//      return 0; // TODO: check how to handle this properly, what does on/off time == 0 mean? infinite on/off??
//TODO: check; case 255: // not used value return DELAY_INFINITE?
  }
  return DELAY_INFINITE; //0
};


/* convert time value stored in EEPROM to milliseconds - 2 byte value */
inline uint32_t convertTime(uint16_t timeValue) {
  
  uint8_t factor = timeValue >> 14;    // mask out factor (higest two bits)
  timeValue &= 0x3FFF;    // keep time value only

  // factors: 0.1,1,60,1000 (last one is not used)
  switch (factor) {
    case 0:          // x0.1
      return (uint32_t)timeValue *100;
      break;
    case 1:          // x1
      return (uint32_t)timeValue *1000;
      break;
    case 2:         // x60
      return (uint32_t)timeValue *60000;
      break;
 //    case 192:        // not used value
//      return 0; // TODO: check how to handle this properly, what does on/off time == 0 mean? infinite on/off??
//TODO: check; case 255: // not used value return DELAY_INFINITE?
  }
  return DELAY_INFINITE; //0
};


#endif
