//*******************************************************************
//
// HBW-Sen-Key-12
//
// Homematic Wired Hombrew Hardware
// Arduino Uno als Homematic-Device
// HBW-Sen-Key-12 zum Einlesen von 12 Tastern
// - Active HIGH oder LOW kann konfiguriert werden
// - Pullup kann aktiviert werden
// - Erkennung von Doppelklicks
// - Zusaetzliches Event beim Loslassen einer lang gedrueckten Taste
//
//*******************************************************************


#define HMW_DEVICETYPE 0x95
#define HARDWARE_VERSION 0x01
#define FIRMWARE_VERSION 0x0100

#define NUM_CHANNELS 12
#define NUM_LINKS 36
#define LINKADDRESSSTART 0x40


#include <ClickButton.h>

// HB Wired protocol and module
#include <HBWired.h>
#include <HBWLinkKey.h>
#include <HBW_eeprom.h>


// Pins and hardware config
#include "HBW-Sen-Key-12_config_example.h"  // When using custom device pinout or controller, copy this file and include it instead


// Config as C++ structure (without direct links)
struct hbw_config_sen_key {
  uint8_t input_locked:1;      // 0x07:0    0=LOCKED, 1=UNLOCKED
  uint8_t inverted:1;          // 0x07:1
  uint8_t pullup:1;            // 0x07:2
  uint8_t       :5;            // 0x07:3-7
  uint8_t long_press_time;     // 0x08
};

struct hbw_config {
  uint8_t logging_time;     // 0x01
  uint32_t central_address;  // 0x02 - 0x05
  uint8_t direct_link_deactivate:1;   // 0x06:0
  uint8_t              :7;   // 0x06:1-7
  hbw_config_sen_key keysCfg[NUM_CHANNELS]; // 0x07-0x1E
} hbwconfig;




// Class HBSenKey
class HBSenKey : public HBWChannel {
  public:
    HBSenKey(uint8_t _pin, hbw_config_sen_key* _config);
    virtual void loop(HBWDevice*, uint8_t channel);
    virtual void afterReadConfig();
  private:
    hbw_config_sen_key* config;
    uint8_t pin;   // Pin
    uint32_t lastSentLong;      // Zeit, zu der das letzte Mal longPress gesendet wurde
    uint8_t keyPressNum;
    int8_t keyState;
    ClickButton button;
};


HBSenKey* keys[NUM_CHANNELS];

HBWDevice* device = NULL;


HBSenKey::HBSenKey(uint8_t _pin, hbw_config_sen_key* _config) 
              : config(_config),
                pin(_pin),
                button(_pin,LOW,HIGH) { 
};

void HBSenKey::afterReadConfig(){
    if(config->long_press_time == 0xFF) config->long_press_time = 10;
    button = ClickButton(pin, config->inverted ? LOW : HIGH, config->pullup ? HIGH : LOW);
    button.debounceTime   = 20;   // Debounce timer in ms
    button.multiclickTime = 250;  // Time limit for multi clicks
    button.longClickTime  = config->long_press_time;
    button.longClickTime *= 100; // Time until long clicks register 
}


void HBSenKey::loop(HBWDevice* device, uint8_t channel) {

  long now = millis();
  uint8_t data; 

  button.Update();
  if (button.clicks) {
    keyState = button.clicks;
    keyPressNum++;
    if (button.clicks == 1) { // Einfachklick
        // TODO: Peering. Only for normal short and long click?
        // TODO: doesn't waiting for multi-clicks make everything slow?  
        device->sendKeyEvent(channel,keyPressNum, false);  // short press
    }
    // Multi-Click
    else if (button.clicks >= 2) {  // Mehrfachklick
        data = (keyPressNum << 2) + 1;
        device->sendKeyEvent(channel, 1, &data);

    } else if (button.clicks < 0) {  // erstes LONG
        lastSentLong = now;
        device->sendKeyEvent(channel,keyPressNum, true);  // long press
    }
  } else if (keyState < 0) {   // Taste ist oder war lang gedrückt
        if (button.depressed) {  // ist noch immer gedrueckt --> alle 300ms senden
          if(now - lastSentLong >= 300){ // alle 300ms erneut senden
            lastSentLong = lastSentLong + 300;
            device->sendKeyEvent(channel,keyPressNum, true);  // long press
          }
        } else {    // "Losgelassen" senden
            data = keyPressNum << 2; // + 0
            device->sendKeyEvent(channel, 1, &data);
            keyState = 0;
        }
  }
}


void setup()
{
  Serial.begin(115200);  // Serial->USB for debug
  rs485.begin(19200);   // RS485 via SoftwareSerial, must use 19200 baud!

  // create channels
  static const uint8_t pins[NUM_CHANNELS] = {Key1, Key2, Key3, Key4, Key5, Key6, Key7, Key8, Key9, Key10, Key11, Key12};
  // Keys
  for(uint8_t i = 0; i < NUM_CHANNELS; i++) {
    keys[i] = new HBSenKey(pins[i], &(hbwconfig.keysCfg[i]));
  }

  device = new HBWDevice(HMW_DEVICETYPE, HARDWARE_VERSION, FIRMWARE_VERSION,
                         &rs485,RS485_TXEN,sizeof(hbwconfig),&hbwconfig,
                         NUM_CHANNELS,(HBWChannel**)keys,
                         &Serial,
                         new HBWLinkKey<NUM_LINKS, LINKADDRESSSTART>(), NULL);

  device->setConfigPins(BUTTON, LED);  // 8 and 13 is the default
 
  hbwdebug(F("B: 2A "));
  hbwdebug(freeRam());
  hbwdebug(F("\n")); 
}


void loop()
{
  device->loop();
};
