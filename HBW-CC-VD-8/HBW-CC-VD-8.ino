//*******************************************************************
//
// HBW-CC-VD-8, RS485 2-channel PID Valve actuator
//
// Homematic Wired Hombrew Hardware
// Arduino NANO als Homematic-Device
// 
// - Direktes Peering für ___ möglich. ()
//
// http://loetmeister.de/Elektronik/homematic/index.htm#modules
//
//*******************************************************************
// Changes
// v0.01
// - initial version


#define HARDWARE_VERSION 0x01
#define FIRMWARE_VERSION 0x0001

#define NUMBER_OF_VD_CHAN 2   // output channels - valve actuator
#define NUMBER_OF_PID_CHAN 2   // output channels - PID regulator
#define NUMBER_OF_TEMP_CHAN 0   // input channels - 1-wire temperature sensors


#define NUM_LINKS_VD 20    // address step 20
#define LINKADDRESSSTART_VD 0x038   // ends @0x
#define NUM_LINKS_PID 18    // address step 6
#define LINKADDRESSSTART_PID 0x370   // ends @0x
//#define NUM_LINKS_TEMP 18    // address step 6
//#define LINKADDRESSSTART_TEMP 0x370   // ends @0x


#define HMW_DEVICETYPE 0x97 //device ID (make sure to import hbw_ .xml into FHEM)

//#define USE_HARDWARE_SERIAL   // use hardware serial (USART) - this disables debug output


// HB Wired protocol and modules
#include <HBWired.h>
#include <HBWOneWireTempSensors.h>
#include "HMWPids.h"
//#include <HBWLinkInfoMessageSensor.h>new HBWLinkInfoMessageSensor(NUM_LINKS_TEMP_CHAN,ADDRESS_START_LINK_TEMP_CHAN)
#include <HBWLinkInfoMessageActuator.h>


// Pins
#ifdef USE_HARDWARE_SERIAL
  #define RS485_TXEN 2  // Transmit-Enable
  #define BUTTON A6  // Button fuer Factory-Reset etc.
  #define ADC_BUS_VOLTAGE A7  // analog input to measure bus voltage
  
  #define ONEWIRE_PIN	A5 // Onewire Bus
  #define VD1  3  // valve "PWM on valium" output
  #define VD1  4
//  #define VD3  5
//  #define VD4  6
//  #define VD5  7
//  #define VD6  8
//  #define VD7  9
//  #define VD8  10

#else
  #define RS485_RXD 4
  #define RS485_TXD 2
  #define RS485_TXEN 3  // Transmit-Enable
  #define BUTTON 8  // Button fuer Factory-Reset etc.
  #define ADC_BUS_VOLTAGE A7  // analog input to measure bus voltage

  #define ONEWIRE_PIN	A3 // Onewire Bus
  #define VD1  A1
  #define VD2  A2
//  #define VD3  5
//  #define VD4  6
//  #define VD5  7
//  #define VD6  12
//  #define VD7  9
//  #define VD8  10
  //#define xx10 NOT_A_PIN  // dummy pin to fill the array elements
  
  #include "FreeRam.h"
  #include "HBWSoftwareSerial.h"
  // HBWSoftwareSerial can only do 19200 baud
  HBWSoftwareSerial rs485(RS485_RXD, RS485_TXD); // RX, TX
#endif  //USE_HARDWARE_SERIAL

#define LED LED_BUILTIN        // Signal-LED

#define NUMBER_OF_CHAN NUMBER_OF_VD_CHAN + NUMBER_OF_PID_CHAN + NUMBER_OF_TEMP_CHAN


struct hbw_config {
  uint8_t logging_time;     // 0x01
  uint32_t central_address;  // 0x02 - 0x05
  uint8_t direct_link_deactivate:1;   // 0x06:0
  uint8_t              :7;   // 0x06:1-7
  hbw_config_onewire_temp TempOWCfg[NUMBER_OF_TEMP_CHAN]; // 0x07 - 0x..(address step 14)
  hbw_config_pid pidCfg[NUMBER_OF_PID_CHAN];
  hbw_config_pid_valve pidValveCfg[NUMBER_OF_VD_CHAN];
} hbwconfig;


HBWChannel* channels[NUMBER_OF_CHAN];  // total number of channels for the device


class HBVdDevice : public HBWDevice {
    public:
    HBVdDevice(uint8_t _devicetype, uint8_t _hardware_version, uint16_t _firmware_version,
               Stream* _rs485, uint8_t _txen, 
               uint8_t _configSize, void* _config, 
               uint8_t _numChannels, HBWChannel** _channels,
               Stream* _debugstream, HBWLinkSender* linksender = NULL, HBWLinkReceiver* linkreceiver = NULL) :
    HBWDevice(_devicetype, _hardware_version, _firmware_version,
              _rs485, _txen, _configSize, _config, _numChannels, ((HBWChannel**)(_channels)),
              _debugstream, linksender, linkreceiver) {
    };
    virtual void afterReadConfig();
};

// device specific defaults
void HBVdDevice::afterReadConfig() {
  if (hbwconfig.logging_time == 0xFF) hbwconfig.logging_time = 50;
};

HBVdDevice* device = NULL;
//HBWPids* g_pids[NUMBER_OF_VD_CHAN];


void setup()
{
  // create channels

  byte valvePin[2] = {VD1, VD2};  // assing pins
  HBWPids* g_pids[NUMBER_OF_PID_CHAN];
  
  for(uint8_t i = 0; i < NUMBER_OF_PID_CHAN; i++) {
    g_pids[i] = new HBWPids(&(hbwconfig.pidValveCfg[i]), &(hbwconfig.pidCfg[i]));
    channels[i] = g_pids[i];
    channels[i + NUMBER_OF_PID_CHAN] = new HBWPidsValve(valvePin[i], g_pids[i], &(hbwconfig.pidValveCfg[i]));
  };



#ifdef USE_HARDWARE_SERIAL  // RS485 via UART Serial, no debug (_debugstream is NULL)
  Serial.begin(19200, SERIAL_8E1);
  
  device = new HBVdDevice(HMW_DEVICETYPE, HARDWARE_VERSION, FIRMWARE_VERSION,
                             &Serial, RS485_TXEN, sizeof(hbwconfig), &hbwconfig,
                             NUMBER_OF_CHAN, (HBWChannel**)channels,
                             NULL,
                             NULL, NULL);
  
  device->setConfigPins(BUTTON, LED);  // use analog input for 'BUTTON'
  //device->setStatusLEDPins(LED, LED); // Tx, Rx LEDs
  
#else
  Serial.begin(19200);
  rs485.begin();    // RS485 via SoftwareSerial
  
  device = new HBVdDevice(HMW_DEVICETYPE, HARDWARE_VERSION, FIRMWARE_VERSION,
                             &rs485, RS485_TXEN, sizeof(hbwconfig), &hbwconfig,
                             NUMBER_OF_CHAN, (HBWChannel**)channels,
                             &Serial,
                             NULL, new HBWLinkInfoMessageActuator(NUM_LINKS_PID,LINKADDRESSSTART_PID));
  
  device->setConfigPins(BUTTON, LED);  // 8 (button) and 13 (led) is the default
  //device->setStatusLEDPins(LED, LED); // Tx, Rx LEDs

  hbwdebug(F("B: 2A "));
  hbwdebug(freeRam());
  hbwdebug(F("\n"));
#endif
}


void loop()
{
  device->loop();
};
