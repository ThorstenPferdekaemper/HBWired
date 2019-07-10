//*******************************************************************
//
// HBW-CC-DT3-T6
//
// Homematic Wired Hombrew Hardware
// Arduino NANO als Homematic-Device
// 
// http://loetmeister.de/Elektronik/homematic/index.htm#modules
//
//*******************************************************************
// Changes
// v0.01
// - initial version



#define HARDWARE_VERSION 0x01
#define FIRMWARE_VERSION 0x0001

#define NUMBER_OF_TEMP_CHAN 6   // input channels - 1-wire temperature sensors
#define ADDRESS_START_CONF_TEMP_CHAN 0x7  // first EEPROM address for temperature sensors configuration
#define NUM_LINKS_TEMP 20    // requires Support_HBWLink_InfoEvent in HBWired.h
#define LINKADDRESSSTART_TEMP 0x100  // step 6
#define NUMBER_OF_DELTAT_CHAN 3 // result output channels, can peer with switch
#define NUM_LINKS_DELTATX 6     // peer  input channels (T1 & T2) with temperature sensor
#define LINKADDRESSSTART_DELTATX 0x220  // step 7
#define NUMBER_OF_KEY_CHAN 3  // input channels - pushbutton/switch
#define NUM_LINKS_KEY 12    // 3 DELTA_T, 9 for pushbutton/switch
#define LINKADDRESSSTART_KEY 0x100 //0x1A2, only one start address possible for all sensor type peers  // step 6

#define HMW_DEVICETYPE 0x71 //device ID (make sure to import .xml into FHEM)

//#define USE_HARDWARE_SERIAL   // use hardware serial (USART) - this disables debug output


// HB Wired protocol and module
#include <HBWired.h>
#include <HBWKey.h>
#include <HBWLinkKey.h>
#include <HBWOneWireTempSensors.h>
#include <HBWLinkInfoEventSensor.h>
#include <HBWLinkInfoEventActuator.h>
#include "HBWDeltaT.h"


// Pins
#ifdef USE_HARDWARE_SERIAL
  #define RS485_TXEN 2  // Transmit-Enable
  #define BUTTON A6  // Button fuer Factory-Reset etc.
  #define ADC_BUS_VOLTAGE A7  // analog input to measure bus voltage (optional)
  
  #define ONEWIRE_PIN	10 // Onewire Bus
  #define RELAY_1 5
  #define RELAY_2 6
  #define RELAY_3 7

  #define BUTTON_1 A1
  #define BUTTON_2 A2
  #define BUTTON_3 A3
  
  #define BLOCKED_TWI_SDA A4  // used by I²C - SDA
  #define BLOCKED_TWI_SCL A5  // used by I²C - SCL
  
#else
  #define RS485_RXD 4
  #define RS485_TXD 2
  #define RS485_TXEN 3  // Transmit-Enable
  #define BUTTON 8  // Button fuer Factory-Reset etc.
  #define ADC_BUS_VOLTAGE A7  // analog input to measure bus voltage (optional)

  #define ONEWIRE_PIN	10 // Onewire Bus
  #define RELAY_1 A1
  #define RELAY_2 A2
  #define RELAY_3 A3 //NOT_A_PIN

  #define BUTTON_1 6
  #define BUTTON_2 NOT_A_PIN
  #define BUTTON_3 NOT_A_PIN

  #include "FreeRam.h"
  #include "HBWSoftwareSerial.h"
  // HBWSoftwareSerial can only do 19200 baud
  HBWSoftwareSerial rs485(RS485_RXD, RS485_TXD); // RX, TX
#endif  //USE_HARDWARE_SERIAL

#define LED LED_BUILTIN        // Signal-LED


#define NUMBER_OF_CHAN NUMBER_OF_TEMP_CHAN + (NUMBER_OF_DELTAT_CHAN *3) + NUMBER_OF_KEY_CHAN


struct hbw_config {
  uint8_t logging_time;     // 0x01
  uint32_t central_address;  // 0x02 - 0x05
  uint8_t direct_link_deactivate:1;   // 0x06:0
  uint8_t              :7;   // 0x06:1-7
  hbw_config_onewire_temp TempOWCfg[NUMBER_OF_TEMP_CHAN]; // 0x07 - 0x5A (address step 14)
  hbw_config_DeltaT DeltaTCfg[NUMBER_OF_DELTAT_CHAN];     // 0x5B - 0x69 (address step 5)
  hbw_config_DeltaTx DeltaT1Cfg[NUMBER_OF_DELTAT_CHAN];  // 0x6A - 0x7B (address step 2)
  hbw_config_DeltaTx DeltaT2Cfg[NUMBER_OF_DELTAT_CHAN];  // 0x7C - 0x8D (address step 2)
  hbw_config_key KeyCfg[NUMBER_OF_KEY_CHAN]; // 0x8E - 0x.. (address step 2)
} hbwconfig;


HBWChannel* channels[NUMBER_OF_CHAN];  // total number of channels for the device

// global pointer for OneWire channels
hbw_config_onewire_temp* tempConfig[NUMBER_OF_TEMP_CHAN]; // pointer for config


class HBDTControlDevice : public HBWDevice {
    public:
    HBDTControlDevice(uint8_t _devicetype, uint8_t _hardware_version, uint16_t _firmware_version,
               Stream* _rs485, uint8_t _txen, 
               uint8_t _configSize, void* _config, 
               uint8_t _numChannels, HBWChannel** _channels,
               Stream* _debugstream, HBWLinkSender* linksender = NULL, HBWLinkReceiver* linkreceiver = NULL,
               OneWire* oneWire = NULL, hbw_config_onewire_temp** _tempSensorconfig = NULL,
               HBWLinkSender* _linkSenderTemp = NULL, HBWLinkReceiver* _linkReceiverTemp = NULL) :
    HBWDevice(_devicetype, _hardware_version, _firmware_version,
              _rs485, _txen, _configSize, _config, _numChannels, ((HBWChannel**)(_channels)),
              _debugstream, linksender, linkreceiver)
              {
                d_ow = oneWire;
                tempSensorconfig = _tempSensorconfig;
                linkSenderInfo = _linkSenderTemp;
                linkReceiverInfo = _linkReceiverTemp;
    };
    virtual void afterReadConfig();
    
    private:
      OneWire* d_ow;
      hbw_config_onewire_temp** tempSensorconfig;

//HBWLinkSender* linkSenderTemp;
//HBWLinkReceiver* linkReceiverTemp;
    protected:
//      HBWLinkKey linkSender;
//      HBWLinkInfoEventSensor linkSenderTemp;
};

// device specific defaults
void HBDTControlDevice::afterReadConfig()
{
  if(hbwconfig.logging_time == 0xFF) hbwconfig.logging_time = 50;
  
  HBWOneWireTemp::sensorSearch(d_ow, tempSensorconfig, (uint8_t) NUMBER_OF_TEMP_CHAN, (uint8_t) ADDRESS_START_CONF_TEMP_CHAN);


//static const uint16_t sAddr = 0x100;
//hbwdebug(F("EE@ 0x100"));hbwdebug(F("\n"));
//static const uint8_t Esize = 64;
//byte eeFoo[Esize];
//readEEPROM(eeFoo, sAddr, Esize);
//for(byte i = 0; i < Esize; i++) {
//hbwdebughex(eeFoo[i]);
//if (i % 6 == 0 && i != 0) hbwdebug(F("\n"));
//} hbwdebug(F("\n"));

};

HBDTControlDevice* device = NULL;



void setup()
{
  // variables for all OneWire channels
  OneWire* g_ow = NULL;
  uint32_t g_owLastReadTime = 0;
  uint8_t g_owCurrentChannel = OW_CHAN_INIT; // always initialize with OW_CHAN_INIT value!
  g_ow = new OneWire(ONEWIRE_PIN);

  // create channels
  for(uint8_t i = 0; i < NUMBER_OF_TEMP_CHAN; i++) {
    channels[i] = new HBWOneWireTemp(g_ow, &(hbwconfig.TempOWCfg[i]), &g_owLastReadTime, &g_owCurrentChannel);
    tempConfig[i] = &(hbwconfig.TempOWCfg[i]);
  }

  byte relayPin[NUMBER_OF_DELTAT_CHAN] = {RELAY_1, RELAY_2, RELAY_3};  // assing pins
  HBWDeltaTx* deltaTxCh[NUMBER_OF_DELTAT_CHAN *2];  // pointer to deltaTx channels, to link in deltaT channels

  for(uint8_t i = 0; i < NUMBER_OF_DELTAT_CHAN; i++) {
    deltaTxCh[i] = new HBWDeltaTx( &(hbwconfig.DeltaT1Cfg[i]) );
    deltaTxCh[i +NUMBER_OF_DELTAT_CHAN] = new HBWDeltaTx( &(hbwconfig.DeltaT2Cfg[i]) );
    channels[i +NUMBER_OF_TEMP_CHAN] = new HBWDeltaT(relayPin[i], deltaTxCh[i], deltaTxCh[i +NUMBER_OF_DELTAT_CHAN], &(hbwconfig.DeltaTCfg[i]));
    channels[i +NUMBER_OF_TEMP_CHAN + NUMBER_OF_DELTAT_CHAN] = deltaTxCh[i];
    channels[i +NUMBER_OF_TEMP_CHAN + NUMBER_OF_DELTAT_CHAN *2] = deltaTxCh[i +NUMBER_OF_DELTAT_CHAN];
  }
  
  byte buttonPin[NUMBER_OF_KEY_CHAN] = {BUTTON_1, BUTTON_2, BUTTON_3};  // assing pins
  for(uint8_t i = 0; i < NUMBER_OF_KEY_CHAN; i++) {
    channels[i +NUMBER_OF_TEMP_CHAN +NUMBER_OF_DELTAT_CHAN *3] = new HBWKey(buttonPin[i], &(hbwconfig.KeyCfg[i]));
  }


#ifdef USE_HARDWARE_SERIAL  // RS485 via UART Serial, no debug (_debugstream is NULL)
  Serial.begin(19200, SERIAL_8E1);
  
  device = new HBDTControlDevice(HMW_DEVICETYPE, HARDWARE_VERSION, FIRMWARE_VERSION,
                             &Serial, RS485_TXEN, sizeof(hbwconfig), &hbwconfig,
                             NUMBER_OF_CHAN, (HBWChannel**)channels,
                             NULL,
  #if defined(NUM_LINKS_KEY) && defined(NUM_LINKS_TEMP)
                             new HBWLinkKey(NUM_LINKS_KEY + NUM_LINKS_TEMP,LINKADDRESSSTART_TEMP), NULL,
  #else
                             NULL, NULL,
  #endif
                             g_ow, tempConfig,
                             new HBWLinkInfoEventSensor(NUM_LINKS_TEMP + NUM_LINKS_KEY,LINKADDRESSSTART_TEMP),
                             new HBWLinkInfoEventActuator(NUM_LINKS_DELTATX,LINKADDRESSSTART_DELTATX)
                             );
  
  device->setConfigPins(BUTTON, LED);  // use analog input for 'BUTTON'
  
#else
  Serial.begin(19200);
  rs485.begin();    // RS485 via SoftwareSerial
  
  device = new HBDTControlDevice(HMW_DEVICETYPE, HARDWARE_VERSION, FIRMWARE_VERSION,
                             &rs485, RS485_TXEN, sizeof(hbwconfig), &hbwconfig,
                             NUMBER_OF_CHAN, (HBWChannel**)channels,
                             &Serial,
                             new HBWLinkKey(NUM_LINKS_KEY + NUM_LINKS_TEMP,LINKADDRESSSTART_TEMP), NULL,
                             //new HBWLinkInfoEventActuator(NUM_LINKS_DELTATX,LINKADDRESSSTART_KEY),
                             g_ow, tempConfig,
                             new HBWLinkInfoEventSensor(NUM_LINKS_TEMP + NUM_LINKS_KEY,LINKADDRESSSTART_TEMP),
                             new HBWLinkInfoEventActuator(NUM_LINKS_DELTATX,LINKADDRESSSTART_DELTATX)
                             );
  
  device->setConfigPins(BUTTON, LED);  // 8 (button) and 13 (led) is the default

  hbwdebug(F("B: 2A "));
  hbwdebug(freeRam());
  hbwdebug(F("\n"));
#endif
}


void loop()
{
  device->loop();
};


// check if HBWLinkInfoEvent support is enabled, when links are set
#if !defined(Support_HBWLink_InfoEvent) && defined(NUM_LINKS_TEMP)
#error enable/define Support_HBWLink_InfoEvent in HBWired.h
#endif
  
