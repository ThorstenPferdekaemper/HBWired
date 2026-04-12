/* 
 * Setup set timer interrupt at ~50Hz
 * most closest at 50.08 Hz (with 16MHz system clock / OSC)
*
* http://loetmeister.de/Elektronik/homematic/
*
*/

#ifndef SetupTimer_328p_h
#define SetupTimer_328p_h


inline void setup_timer()  // call from main setup() //TODO: move hardware specific parts to HBW-CC-WW-SPktS_config_example.h - direct or nested
{
  cli();//stop interrupts

  //set timer1 interrupt at 50.08 ~50Hz (with 16MHz system clock / OSC)
  TCCR1A = 0;// set entire TCCR1A register to 0
  TCCR1B = 0;// same for TCCR1B
  TCNT1  = 0;//initialize counter value to 0
  // set compare match register for 1hz increments
  OCR1A = 312;// = (16*10^6) / (50*1024) - 1 (must be <65536)
  // turn on CTC mode
  TCCR1B |= (1 << WGM12);
  // Set CS12 and CS10 bits for 1024 prescaler
  TCCR1B |= (1 << CS12) | (1 << CS10);  
  // enable timer compare interrupt
  TIMSK1 |= (1 << OCIE1A);

  sei();//allow interrupts
};
#endif

