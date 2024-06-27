//*******************************************************************
//
// HBW-WDS-C7
//
// Homematic Wired Hombrew Hardware
// Raspberry Pi Pico als Homematic-Device
// - foobar
//
// http://loetmeister.de/Elektronik/homematic/index.htm#modules
//
//*******************************************************************
// Changes
// v0.01
// - initial


#define HARDWARE_VERSION 0x01
#define FIRMWARE_VERSION 0x0001
#define HMW_DEVICETYPE 0x88

#define NUM_CHANNELS 2
#define NUM_LINKS 36
#define LINKADDRESSSTART 0x80


/* Undefine "HBW_DEBUG" in 'HBWired.h' to remove code not needed. "HBW_DEBUG" also works as master switch,
 * as hbwdebug() or hbwdebughex() used in channels will point to empty functions. */


// #include <FreeRam.h>

#if defined (ARDUINO_ARCH_RP2040)
  // #include "MBED_RPi_Pico_TimerInterrupt.h"  // Timer for LED Blinking
  #include <Scheduler.h>
  #include <SparkFun_External_EEPROM.h>
  ExternalEEPROM* EepromPtr;
  // EepromPtr = new ExternalEEPROM;
  // ExternalEEPROM* EEPROM = new ExternalEEPROM;
  // // ExternalEEPROM *EEPROM;
  // Valid types: 0, 1, 2, 4, 8, 16, 32, 64, 128, 256, 512, 1025, 2048
  #define EEPROM_Memory_Type 4
#else
  #error Target Plattform not supported! Please contribute.
#endif

// HB Wired protocol and module
#include <HBWired.h>
#include "HBWSIGNALDuino_adv.h"


// Pins
// TODO move to own file (pins_default.h pins_custom.h - don't check-in last one)
#define LED LED_BUILTIN      // Signal-LED

#define RS485_TXEN 2  // Transmit-Enable
#define BUTTON 21  // Button fuer Factory-Reset etc.

#define SWITCH1_PIN 6  // Ausgangpins fuer die Relais
#define SWITCH2_PIN 7

// default pins:
// USB Rx / Tx 0, 1
// static const uint8_t SPIpins[] = {MOSI, SS, MISO, SCK}; // pin 16 - 19
// static const uint8_t I2Cpins[] = {PIN_WIRE_SDA, PIN_WIRE_SCL}; // pin 4 & 5
// UART1 8 & 9

struct hbw_config {
  uint8_t logging_time;     // 0x01
  uint32_t central_address;  // 0x02 - 0x05
  uint8_t direct_link_deactivate:1;   // 0x06:0
  uint8_t              :7;   // 0x06:1-7
  hbw_config_signalduino_adv signalduinoCfg[NUM_CHANNELS]; // 0x07-0x... ? (address step ?)
  // hbw_config_signalduino_wds wds7in1Cfg[NUM_CHANNELS]; // 0x07-0x... ? (address step ?)
} hbwconfig;


// create object for the channel
HBWChannel* channels[NUM_CHANNELS];


class HBWDSDevice : public HBWDevice {
    public: 
    HBWDSDevice(uint8_t _devicetype, uint8_t _hardware_version, uint16_t _firmware_version,
               Stream* _rs485, uint8_t _txen, 
              //  UART* _rs485, uint8_t _txen, 
               uint8_t _configSize, void* _config, 
               uint8_t _numChannels, HBWChannel** _channels,
               Stream* _debugstream, HBWLinkSender* linksender = NULL, HBWLinkReceiver* linkreceiver = NULL) :
              //  UART* _debugstream, HBWLinkSender* linksender = NULL, HBWLinkReceiver* linkreceiver = NULL) :
    HBWDevice(_devicetype, _hardware_version, _firmware_version,
              _rs485, _txen, _configSize, _config, _numChannels, ((HBWChannel**)(_channels)),
              _debugstream, linksender, linkreceiver) {
    };
    // virtual void afterReadConfig();
};

// device specific defaults
// void HBWDSDevice::afterReadConfig() {
//   if(hbwconfig.logging_time == 0xFF) hbwconfig.logging_time = 50;
// };


HBWDSDevice* device = NULL;


void setup()
{
  pinMode(LED, OUTPUT);
  digitalWrite(LED, HIGH);
  Serial.begin(115200);  // Serial->USB for debug
  Serial1.begin(19200, SERIAL_8E1);  // RS485 bus

  #if defined (ARDUINO_ARCH_RP2040)
    Wire.begin();
    EepromPtr->setMemoryType(EEPROM_Memory_Type);
    if (! EepromPtr->begin())
    {
      while (true) {
        digitalWrite(LED, HIGH);
        delay(200);
        digitalWrite(LED, LOW);
        delay(200);
      }
    }
    // TODO stop without EEPROM?
	#endif

  // create channels
  static const uint8_t pins[NUM_CHANNELS] = {SWITCH1_PIN, SWITCH2_PIN};
  
  // creating to channels
  for(uint8_t i = 0; i < NUM_CHANNELS; i++) {
    channels[i] = new HBWSIGNALDuino_adv(pins[i], &(hbwconfig.signalduinoCfg[i]));
  }

  // create the device
  device = new HBWDSDevice(HMW_DEVICETYPE, HARDWARE_VERSION, FIRMWARE_VERSION,
                         &Serial1,
                         RS485_TXEN, sizeof(hbwconfig), &hbwconfig,
                         NUM_CHANNELS, (HBWChannel**)channels,
                        //  NULL, new HBWLinkSwitchAdvanced<NUM_LINKS, LINKADDRESSSTART>());
                         &Serial);

  device->setConfigPins(BUTTON, LED);
    
  // hbwdebug(F("B: 2A "));
  // hbwdebug(freeRam());
  // hbwdebug(F("\n"));

  Scheduler.startLoop(loopHBW);
  Scheduler.startLoop(loop3);
  // digitalWrite(LED, LOW);
}

void loop()
{
  // device->loop();
  // POWERSAVE();  // go sleep a bit
};

void loopHBW()
{
  // device->loop();
  // POWERSAVE();  // go sleep a bit
  // IMPORTANT:
  // We must call 'yield' at a regular basis to pass control to other tasks.
  yield();
};

// TODO 3rd loop for serial input?
void loop3() {
  if (Serial.available()) {
    char c = Serial.read();
    if (c == '0') {
      // digitalWrite(led1, LOW);
      Serial.println("Led turned off!");
    }
    if (c == '1') {
      // digitalWrite(led1, HIGH);
      Serial.println("Led turned on!");
    }
    if (c == 'm') {
        if (EepromPtr->isConnected())
        {
          uint32_t memSize = EepromPtr->getMemorySizeBytes();
          Serial.print("EEProm Size bytes ");
          Serial.println(memSize);
        }
        else {
          Serial.println("No Memory detected!");
        }
    }
  }
};
