//*******************************************************************
//
// HBW-Sen-SC8
//
// Homematic Wired Hombrew Hardware
// Arduino Uno als Homematic-Device
// HBW-Sen-SC8 zum Einlesen von 8 Tastern
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

#include "HBWSoftwareSerial.h"
#include "FreeRam.h"    

#include "ClickButton.h"

// HB Wired protocol and module
#include "HBWired.h"
// Links Key ->
#include "HBWLinkKey.h"

#define RS485_RXD 4
#define RS485_TXD 2
#define RS485_TXEN 3  // Transmit-Enable

// HBWSoftwareSerial can only do 19200 baud
HBWSoftwareSerial rs485(RS485_RXD, RS485_TXD); // RX, TX


// Pins
#define BUTTON 8  // Button fuer Factory-Reset etc.
#define LED 13        // Signal-LED

// Das folgende Define kann benutzt werden, wenn ueber die
// Kanaele "geloopt" werden soll
// als Define, damit es zentral definiert werden kann, aber keinen (globalen) Speicherplatz braucht
#define PIN_ARRAY uint8_t pins[NUM_CHANNELS] = {A0, A1, A2, A3, A4, A5, 5, 6, 7, 9, 10, 11};


// Config as C++ structure (without direct links)
struct hbw_config_key {
  uint8_t input_locked:1;      // 0x07:0    0=LOCKED, 1=UNLOCKED
  uint8_t inverted:1;          // 0x07:1
  uint8_t pullup:1;            // 0x07:2
  uint8_t       :5;            // 0x07:3-7
  byte long_press_time;       // 0x08
};

struct hbw_config {
  uint8_t logging_time;     // 0x01
  uint32_t central_address;  // 0x02 - 0x05
  uint8_t direct_link_deactivate:1;   // 0x06:0
  uint8_t              :7;   // 0x06:1-7
  hbw_config_key keys[NUM_CHANNELS]; // 0x07-0x1E
} hbwconfig;




// Class HBSenKey
class HBSenKey : public HBWChannel {
  public:
    HBSenKey(uint8_t _pin, hbw_config_key* _config);
    virtual void loop(HBWDevice*, uint8_t channel);
    void afterReadConfig();
  private:
    uint8_t pin;   // Pin
    uint32_t lastSentLong;      // Zeit, zu der das letzte Mal longPress gesendet wurde
    uint8_t keyPressNum;
    int8_t keyState;   
    hbw_config_key* config;    
    ClickButton button;
};


HBSenKey* keys[NUM_CHANNELS];

class HBSenDevice : public HBWDevice {
    public: 
    HBSenDevice(uint8_t _devicetype, uint8_t _hardware_version, uint16_t _firmware_version,
            Stream* _rs485, uint8_t _txen, 
            uint8_t _configSize, void* _config, 
        uint8_t _numChannels, HBWChannel** _channels,
        Stream* _debugstream, HBWLinkSender* linksender = NULL, HBWLinkReceiver* linkreceiver = NULL) :
          HBWDevice(_devicetype, _hardware_version, _firmware_version,
            _rs485, _txen, _configSize, _config, _numChannels, ((HBWChannel**)(_channels)),
            _debugstream, linksender, linkreceiver) {
      // looks like virtual methods are not properly called here
      afterReadConfig();        
    };

    void afterReadConfig() {
        // defaults setzen
        if(hbwconfig.logging_time == 0xFF) hbwconfig.logging_time = 20;
        // if(config.central_address == 0xFFFFFFFF) config.central_address = 0x00000001;
        for(uint8_t channel = 0; channel < NUM_CHANNELS; channel++){
            if(hbwconfig.keys[channel].long_press_time == 0xFF) 
                hbwconfig.keys[channel].long_press_time = 10;
            keys[channel]->afterReadConfig();    
        };
    };
};


HBSenDevice* device = NULL;



HBSenKey::HBSenKey(uint8_t _pin, hbw_config_key* _config) 
              : config(_config), pin(_pin), 
                button(_pin,LOW,HIGH) { 
};

void HBSenKey::afterReadConfig(){
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
  } else if (keyState < 0) {   // Taste ist oder war lang gedr�ckt
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
  pinMode(BUTTON, INPUT_PULLUP);
  pinMode(LED, OUTPUT);

  Serial.begin(19200);
  rs485.begin();    // RS485 via SoftwareSerial

   // create channels
   PIN_ARRAY
  // Keys
   for(uint8_t i = 0; i < NUM_CHANNELS; i++){
      keys[i] = new HBSenKey(pins[i], &(hbwconfig.keys[i]));
   };

  device = new HBSenDevice(HMW_DEVICETYPE, HARDWARE_VERSION, FIRMWARE_VERSION,
                         &rs485,RS485_TXEN,sizeof(hbwconfig),&hbwconfig,NUM_CHANNELS,(HBWChannel**)keys,&Serial,
                         new HBWLinkKey(NUM_LINKS,LINKADDRESSSTART), NULL);

  device->setConfigPins();  // 8 and 13 is the default
 
  hbwdebug(F("B: 2A "));
  hbwdebug(freeRam());
  hbwdebug(F("\n")); 
}


void loop()
{
  device->loop();
};
