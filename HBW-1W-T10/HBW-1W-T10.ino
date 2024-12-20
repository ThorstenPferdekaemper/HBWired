//*******************************************************************
//
// HBW-1W-T10
//
// Homematic Wired Hombrew Hardware
// Arduino NANO als Homematic-Device
// 
// nach einer Vorlage von
// Thorsten Pferdekaemper (thorsten@pferdekaemper.com), Dirk Hoffmann (hoffmann@vmd-jena.de)
//
// http://loetmeister.de/Elektronik/homematic/index.htm#modules
//
//*******************************************************************
// Changes
// v0.01
// - initial version
// v0.02
// - improved startup and error handling (disconnected sensors)
// v0.03
// - validate supported devices (see HBWOneWireTempSensors.h)
// - optimized conversion and measurement sequence to avoid wrong readings


#define HARDWARE_VERSION 0x01
#define FIRMWARE_VERSION 0x0003
#define HMW_DEVICETYPE 0x81 //device ID (make sure to import hbw_1w_t10_v1.xml into FHEM)

#define NUMBER_OF_TEMP_CHAN 10   // input channels - 1-wire temperature sensors
#define ADDRESS_START_CONF_TEMP_CHAN 0x7  // first EEPROM address for temperature sensors configuration
//#define NUM_LINKS_TEMP 30    // requires Support_HBWLink_InfoEvent in HBWired.h
#define LINKADDRESSSTART_TEMP 0xE6   // step 6


// HB Wired protocol and module
#include <HBWired.h>
#include <HBWOneWireTempSensors.h>
#include <HBWLinkInfoEventSensor.h>
#include <HBW_eeprom.h>


// Pins and hardware config
#include "HBW-1W-T10_config_example.h"  // When using custom device pinout or controller, copy this file and include it instead


#define NUMBER_OF_CHAN NUMBER_OF_TEMP_CHAN


struct hbw_config {
  uint8_t logging_time;     // 0x01
  uint32_t central_address;  // 0x02 - 0x05
  uint8_t direct_link_deactivate:1;   // 0x06:0
  uint8_t              :7;   // 0x06:1-7
  hbw_config_onewire_temp TempOWCfg[NUMBER_OF_TEMP_CHAN]; // 0x07 - 0x.. (address step 14)
} hbwconfig;


HBWChannel* channels[NUMBER_OF_CHAN];  // total number of channels for the device

hbw_config_onewire_temp* tempConfig[NUMBER_OF_TEMP_CHAN]; // global pointer for OneWire channels config


class HBTempOWDevice : public HBWDevice {
    public:
    HBTempOWDevice(uint8_t _devicetype, uint8_t _hardware_version, uint16_t _firmware_version,
               Stream* _rs485, uint8_t _txen, 
               uint8_t _configSize, void* _config, 
               uint8_t _numChannels, HBWChannel** _channels,
               Stream* _debugstream, HBWLinkSender* linksender = NULL, HBWLinkReceiver* linkreceiver = NULL,
               OneWire* oneWire = NULL, hbw_config_onewire_temp** _tempSensorconfig = NULL) :
    HBWDevice(_devicetype, _hardware_version, _firmware_version,
              _rs485, _txen, _configSize, _config, _numChannels, ((HBWChannel**)(_channels)),
              _debugstream, linksender, linkreceiver)
              {
                d_ow = oneWire;
                tempSensorconfig = _tempSensorconfig;
    };
    virtual void afterReadConfig();
    
    private:
      OneWire* d_ow;
      hbw_config_onewire_temp** tempSensorconfig;
};

// device specific defaults
void HBTempOWDevice::afterReadConfig()
{
  HBWOneWireTemp::sensorSearch(d_ow, tempSensorconfig, (uint8_t) NUMBER_OF_TEMP_CHAN, (uint8_t) ADDRESS_START_CONF_TEMP_CHAN);
};

HBTempOWDevice* device = NULL;



void setup()
{
  // variables for all OneWire channels
  OneWire* g_ow = new OneWire(ONEWIRE_PIN);
  uint32_t g_owLastReadTime = 0;
  uint8_t g_owCurrentChannel = OW_CHAN_INIT; // always init with OW_CHAN_INIT! used as trigger/reset in channel loop()

  // create channels
  for(uint8_t i = 0; i < NUMBER_OF_TEMP_CHAN; i++) {
    channels[i] = new HBWOneWireTemp(g_ow, &(hbwconfig.TempOWCfg[i]), &g_owLastReadTime, &g_owCurrentChannel);
    tempConfig[i] = &(hbwconfig.TempOWCfg[i]);
  }


#ifdef USE_HARDWARE_SERIAL  // RS485 via UART Serial, no debug (_debugstream is NULL)
  Serial.begin(19200, SERIAL_8E1);
  
  device = new HBTempOWDevice(HMW_DEVICETYPE, HARDWARE_VERSION, FIRMWARE_VERSION,
                             &Serial, RS485_TXEN, sizeof(hbwconfig), &hbwconfig,
                             NUMBER_OF_CHAN, (HBWChannel**)channels,
                             NULL,
  #if defined(NUM_LINKS_TEMP)
                             new HBWLinkInfoEventSensor<NUM_LINKS_TEMP, LINKADDRESSSTART_TEMP>(), NULL,
  #else
                             NULL, NULL,
  #endif
                             g_ow, tempConfig);
  
  device->setConfigPins(BUTTON, LED);  // use analog input for 'BUTTON'
  //device->setStatusLEDPins(LED, LED); // Tx, Rx LEDs
  
#else
  Serial.begin(115200);  // Serial->USB for debug
  rs485.begin(19200);   // RS485 via SoftwareSerial, must use 19200 baud!
  
  device = new HBTempOWDevice(HMW_DEVICETYPE, HARDWARE_VERSION, FIRMWARE_VERSION,
                             &rs485, RS485_TXEN, sizeof(hbwconfig), &hbwconfig,
                             NUMBER_OF_CHAN, (HBWChannel**)channels,
                             &Serial,
  #if defined(NUM_LINKS_TEMP)
                             new HBWLinkInfoEventSensor<NUM_LINKS_TEMP, LINKADDRESSSTART_TEMP>(), NULL,
  #else
                             NULL, NULL,
  #endif
                             g_ow, tempConfig);
  
  device->setConfigPins(BUTTON, LED);  // 8 (button) and 13 (led) is the default
  // device->setStatusLEDPins(LED, LED); // Tx, Rx LEDs

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
  
