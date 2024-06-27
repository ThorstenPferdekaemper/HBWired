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


// #include <HBWSoftwareSerial.h>
#include <FreeRam.h>

// HB Wired protocol and module
#include <HBWired.h>
#include "HBWSIGNALDuino_adv.h"

#if defined (ARDUINO_ARCH_RP2040)
  // #include "MBED_RPi_Pico_TimerInterrupt.h"  // Timer for LED Blinking
  #include <Scheduler.h>
#else
  #error Target Plattform not supported! Please contribute.
#endif

// Pins
#define LED LED_BUILTIN      // Signal-LED

#define RS485_TXEN 2  // Transmit-Enable
#define BUTTON 21  // Button fuer Factory-Reset etc.

#define SWITCH1_PIN A4  // Ausgangpins fuer die Relais
#define SWITCH2_PIN A2



struct hbw_config {
  uint8_t logging_time;     // 0x01
  uint32_t central_address;  // 0x02 - 0x05
  uint8_t direct_link_deactivate:1;   // 0x06:0
  uint8_t              :7;   // 0x06:1-7
  hbw_config_signalduino_adv signalduinoCfg[NUM_CHANNELS]; // 0x07-0x... ? (address step ?)
  hbw_config_signalduino_wds wds7in1Cfg[NUM_CHANNELS]; // 0x07-0x... ? (address step ?)
} hbwconfig;


// create object for the channel
HBWChannel* channels[NUM_CHANNELS];


class HBSwDevice : public HBWDevice {
    public: 
    HBSwDevice(uint8_t _devicetype, uint8_t _hardware_version, uint16_t _firmware_version,
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
void HBSwDevice::afterReadConfig() {
  if(hbwconfig.logging_time == 0xFF) hbwconfig.logging_time = 50;
};


HBSwDevice* device = NULL;



void setup()
{
  Serial.begin(115200);  // Serial->USB for debug
  Serial1.begin(19200, SERIAL_8E1);


  // create channels
  static const uint8_t pins[NUM_CHANNELS] = {SWITCH1_PIN, SWITCH2_PIN};
  
  // creating to channels
  for(uint8_t i = 0; i < NUM_CHANNELS; i++) {
    channels[i] = new HBWSIGNALDuino_adv(pins[i], &(hbwconfig.switchCfg[i]));
  }

  device = new HBSwDevice(HMW_DEVICETYPE, HARDWARE_VERSION, FIRMWARE_VERSION,
                         &Serial,
                         &Serial1,
                         RS485_TXEN, sizeof(hbwconfig), &hbwconfig,
                         NUM_CHANNELS, (HBWChannel**)channels,
                         NULL, new HBWLinkSwitchAdvanced<NUM_LINKS, LINKADDRESSSTART>());

  device->setConfigPins(BUTTON, LED);
  
  #elif defined (ARDUINO_ARCH_RP2040)
    Wire.begin();
    EEPROM.setMemoryType(4); // Valid types: 0, 1, 2, 4, 8, 16, 32, 64, 128, 256, 512, 1025, 2048
    EEPROM.begin();
	#endif
  
  hbwdebug(F("B: 2A "));
  hbwdebug(freeRam());
  hbwdebug(F("\n"));

  Scheduler.startLoop(loopHBW);
  Scheduler.startLoop(loop3);
}

void loop()
{
  // device->loop();
  // POWERSAVE();  // go sleep a bit
};

void loopHBW()
{
  device->loop();
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
        if (EEPROM.begin())
        {
          uint32_t memSize = EEPROM.detectMemorySizeBytes();
          Serial.print("EEProm Size ");
          Serial.println(memSize);
        }
        else {
          Serial.println("No Memory detected!");
        }
    }
  }
