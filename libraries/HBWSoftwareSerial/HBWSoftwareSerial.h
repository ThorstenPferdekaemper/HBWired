/*
HBWSoftwareSerial 
(Forked from SoftwareSerial of the "official" Arduino Library)
*/

#ifndef HBWSoftwareSerial_h
#define HBWSoftwareSerial_h

#include <inttypes.h>
#include <Stream.h>

/******************************************************************************
* Definitions
******************************************************************************/

#ifndef _SS_MAX_RX_BUFF
#define _SS_MAX_RX_BUFF 64 // RX buffer size
#endif

#ifndef GCC_VERSION
#define GCC_VERSION (__GNUC__ * 10000 + __GNUC_MINOR__ * 100 + __GNUC_PATCHLEVEL__)
#endif

class HBWSoftwareSerial : public Stream
{
private:
  // per object data
  uint8_t _receivePin;
  uint8_t _receiveBitMask;
  volatile uint8_t *_receivePortRegister;
  uint8_t _transmitBitMask;
  volatile uint8_t *_transmitPortRegister;
  volatile uint8_t *_pcint_maskreg;
  uint8_t _pcint_maskvalue;

  uint16_t _buffer_overflow:1;

  // static data
  static uint8_t _receive_buffer[_SS_MAX_RX_BUFF]; 
  static volatile uint8_t _receive_buffer_tail;
  static volatile uint8_t _receive_buffer_head;
  static HBWSoftwareSerial *active_object;

  // private methods
  inline void recv() __attribute__((__always_inline__));
  inline void tx_pin_write(uint8_t pin_state);
  uint8_t rx_pin_read();
  void setTX(uint8_t transmitPin);
  void setRX(uint8_t receivePin);
  inline void setRxIntMsk(bool enable) __attribute__((__always_inline__));

  // Return num - sub, or 1 if the result would be < 1
  static uint16_t subtract_cap(uint16_t num, uint16_t sub);

  // private static method for timing
  static inline void tunedDelay(uint16_t delay);

public:
  // public methods
  HBWSoftwareSerial(uint8_t receivePin, uint8_t transmitPin);
  ~HBWSoftwareSerial();
  void begin();
  bool listen();
  void end();
  bool isListening() { return this == active_object; }
  bool stopListening();
  bool overflow() { bool ret = _buffer_overflow; if (ret) _buffer_overflow = false; return ret; }
  int peek();

  virtual size_t write(uint8_t byte);
  virtual int read();
  virtual int available();
  virtual void flush();
  operator bool() { return true; }
  
  using Print::write;

  // public only for easy access by interrupt handlers
  static inline void handle_interrupt() __attribute__((__always_inline__));
};

#endif
