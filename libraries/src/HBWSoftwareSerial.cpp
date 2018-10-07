/*
HBWSoftwareSerial 
(Forked from SoftwareSerial of the "official" Arduino Library)
*/

// 
// Includes
// 
#include <avr/interrupt.h>
#include <Arduino.h>
#include <HBWSoftwareSerial.h>

#if F_CPU == 16000000

#define RX_DELAY_CENTER 54
#define RX_DELAY_INTRA  117
#define RX_DELAY_STOP   117
#define TX_DELAY        114
const int XMIT_START_ADJUSTMENT = 5;

#elif F_CPU == 8000000

#define RX_DELAY_CENTER 20
#define RX_DELAY_INTRA  55
#define RX_DELAY_STOP   55
#define TX_DELAY        52
const int XMIT_START_ADJUSTMENT = 4;

#elif F_CPU == 20000000

// 20MHz support courtesy of the good people at macegr.com.
// Thanks, Garrett!
#define RX_DELAY_CENTER 71
#define RX_DELAY_INTRA  148
#define RX_DELAY_STOP   148
#define TX_DELAY        145
const int XMIT_START_ADJUSTMENT = 6;

#else

#error This version of SoftwareSerial supports only 20, 16 and 8MHz processors

#endif

//
// Statics
//
HBWSoftwareSerial *HBWSoftwareSerial::active_object = 0;
uint8_t HBWSoftwareSerial::_receive_buffer[_SS_MAX_RX_BUFF]; 
volatile uint8_t HBWSoftwareSerial::_receive_buffer_tail = 0;
volatile uint8_t HBWSoftwareSerial::_receive_buffer_head = 0;

//
// Private methods
//

/* static */ 
inline void HBWSoftwareSerial::tunedDelay(uint16_t delay) { 
  uint8_t tmp=0;

  asm volatile("sbiw    %0, 0x01 \n\t"
    "ldi %1, 0xFF \n\t"
    "cpi %A0, 0xFF \n\t"
    "cpc %B0, %1 \n\t"
    "brne .-10 \n\t"
    : "+w" (delay), "+a" (tmp)
    : "0" (delay)
    );
}

// This function sets the current object as the "listening"
// one and returns true if it replaces another 
bool HBWSoftwareSerial::listen()
{
  if (active_object != this)
  {
    _buffer_overflow = false;
    uint8_t oldSREG = SREG;
    cli();
    _receive_buffer_head = _receive_buffer_tail = 0;
    active_object = this;
    SREG = oldSREG;
    return true;
  }

  return false;
}

//
// The receive routine called by the interrupt handler
//
void HBWSoftwareSerial::recv()
{

#if GCC_VERSION < 40302
// Work-around for avr-gcc 4.3.0 OSX version bug
// Preserve the registers that the compiler misses
// (courtesy of Arduino forum user *etracer*)
  asm volatile(
    "push r18 \n\t"
    "push r19 \n\t"
    "push r20 \n\t"
    "push r21 \n\t"
    "push r22 \n\t"
    "push r23 \n\t"
    "push r26 \n\t"
    "push r27 \n\t"
    ::);
#endif  

  uint8_t d = 0;

  // If RX line is high, then we don't see any start bit
  // so interrupt is probably not for us
  if (!rx_pin_read())
  {
    // Wait approximately 1/2 of a bit width to "center" the sample
    tunedDelay(RX_DELAY_CENTER);

    // Read each of the 8 bits
    for (uint8_t i=0x1; i; i <<= 1)
    {
      tunedDelay(RX_DELAY_INTRA);
      uint8_t noti = ~i;
      if (rx_pin_read())
        d |= i;
      else // else clause added to ensure function timing is ~balanced
        d &= noti;
    }

	// PFE skip parity bit
	tunedDelay(RX_DELAY_STOP);
	
    // skip the stop bit
    tunedDelay(RX_DELAY_STOP);

    // if buffer full, set the overflow flag and return
    if ((_receive_buffer_tail + 1) % _SS_MAX_RX_BUFF != _receive_buffer_head) 
    {
      // save new data in buffer: tail points to where byte goes
      _receive_buffer[_receive_buffer_tail] = d; // save new byte
      _receive_buffer_tail = (_receive_buffer_tail + 1) % _SS_MAX_RX_BUFF;
    } 
    else 
    {
      _buffer_overflow = true;
    }
  }

#if GCC_VERSION < 40302
// Work-around for avr-gcc 4.3.0 OSX version bug
// Restore the registers that the compiler misses
  asm volatile(
    "pop r27 \n\t"
    "pop r26 \n\t"
    "pop r23 \n\t"
    "pop r22 \n\t"
    "pop r21 \n\t"
    "pop r20 \n\t"
    "pop r19 \n\t"
    "pop r18 \n\t"
    ::);
#endif
}

void HBWSoftwareSerial::tx_pin_write(uint8_t pin_state)
{
  if (pin_state == LOW)
    *_transmitPortRegister &= ~_transmitBitMask;
  else
    *_transmitPortRegister |= _transmitBitMask;
}

uint8_t HBWSoftwareSerial::rx_pin_read()
{
  return *_receivePortRegister & _receiveBitMask;
}

//
// Interrupt handling
//

/* static */
inline void HBWSoftwareSerial::handle_interrupt()
{
  if (active_object)
  {
    active_object->recv();
  }
}

#if defined(PCINT0_vect)
ISR(PCINT0_vect)
{
  HBWSoftwareSerial::handle_interrupt();
}
#endif

#if defined(PCINT1_vect)
ISR(PCINT1_vect)
{
  HBWSoftwareSerial::handle_interrupt();
}
#endif

#if defined(PCINT2_vect)
ISR(PCINT2_vect)
{
  HBWSoftwareSerial::handle_interrupt();
}
#endif

#if defined(PCINT3_vect)
ISR(PCINT3_vect)
{
  HBWSoftwareSerial::handle_interrupt();
}
#endif

//
// Constructor
//
HBWSoftwareSerial::HBWSoftwareSerial(uint8_t receivePin, uint8_t transmitPin) : 
  _buffer_overflow(false)
{
  setTX(transmitPin);
  setRX(receivePin);
}

//
// Destructor
//
HBWSoftwareSerial::~HBWSoftwareSerial()
{
  end();
}

void HBWSoftwareSerial::setTX(uint8_t tx)
{
  pinMode(tx, OUTPUT);
  digitalWrite(tx, HIGH);
  _transmitBitMask = digitalPinToBitMask(tx);
  uint8_t port = digitalPinToPort(tx);
  _transmitPortRegister = portOutputRegister(port);
}

void HBWSoftwareSerial::setRX(uint8_t rx)
{
  pinMode(rx, INPUT);
  digitalWrite(rx, HIGH);  // pullup for normal logic!
  _receivePin = rx;
  _receiveBitMask = digitalPinToBitMask(rx);
  uint8_t port = digitalPinToPort(rx);
  _receivePortRegister = portInputRegister(port);
}

//
// Public methods
//

void HBWSoftwareSerial::begin()
{
  // Set up RX interrupts
  if (digitalPinToPCICR(_receivePin)){
      *digitalPinToPCICR(_receivePin) |= _BV(digitalPinToPCICRbit(_receivePin));
      *digitalPinToPCMSK(_receivePin) |= _BV(digitalPinToPCMSKbit(_receivePin));
  }
  tunedDelay(TX_DELAY); // if we were low this establishes the end
  listen();
}

void HBWSoftwareSerial::end()
{
  if (digitalPinToPCMSK(_receivePin))
    *digitalPinToPCMSK(_receivePin) &= ~_BV(digitalPinToPCMSKbit(_receivePin));
}


// Read data from buffer
int HBWSoftwareSerial::read()
{
  if (!isListening())
    return -1;

  // Empty buffer?
  if (_receive_buffer_head == _receive_buffer_tail)
    return -1;

  // Read from "head"
  uint8_t d = _receive_buffer[_receive_buffer_head]; // grab next byte
  _receive_buffer_head = (_receive_buffer_head + 1) % _SS_MAX_RX_BUFF;
  return d;
}

int HBWSoftwareSerial::available()
{
  if (!isListening())
    return 0;

  return (_receive_buffer_tail + _SS_MAX_RX_BUFF - _receive_buffer_head) % _SS_MAX_RX_BUFF;
}

size_t HBWSoftwareSerial::write(uint8_t b)
{

  // PFE Parity handling
  uint8_t oldSREG = SREG;
  cli();  // turn off interrupts for a clean txmit

  // Write the start bit
  tx_pin_write(LOW);
  tunedDelay(TX_DELAY + XMIT_START_ADJUSTMENT);

   uint8_t p = 0;
   uint8_t t;
   for (t = 0x80; t; t >>= 1)
     if (b & t) p++;

  // Write each of the 8 bits
	for (byte mask = 0x01; mask; mask <<= 1)
    {
      if (b & mask) // choose bit
        tx_pin_write(HIGH); // send 1
      else
        tx_pin_write(LOW); // send 0
      tunedDelay(TX_DELAY);
    }

	// parity
	if (p & 0x01)
	   tx_pin_write(HIGH); // send 1
	else
	   tx_pin_write(LOW); // send 0
	tunedDelay(TX_DELAY);

    tx_pin_write(HIGH); // restore pin to natural state

  SREG = oldSREG; // turn interrupts back on
  tunedDelay(TX_DELAY);
  
  return 1;
}

void HBWSoftwareSerial::flush()
{
  if (!isListening())
    return;

  uint8_t oldSREG = SREG;
  cli();
  _receive_buffer_head = _receive_buffer_tail = 0;
  SREG = oldSREG;
}

int HBWSoftwareSerial::peek()
{
  if (!isListening())
    return -1;

  // Empty buffer?
  if (_receive_buffer_head == _receive_buffer_tail)
    return -1;

  // Read from "head"
  return _receive_buffer[_receive_buffer_head];
}
