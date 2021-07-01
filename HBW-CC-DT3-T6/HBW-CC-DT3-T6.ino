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
// v0.02
// - change to peer: temp (sensor & actuator), removed deltaT virtual Keys (peering conflict)
// v0.30
// - allow deltaT channels peering with external switches (key/actuator peer role)
// v0.40
// - add config to apply hysteresis on maxT1, minT2 and OFF transistion
// - add config for output change frequency (CYCLE_TIME)
// v0.45
// - using config LED for Tx & Rx indicator
// v0.50
// - allow to set HBWDeltaT output when mode is IDLE/inactive (no temperature received)
// - option for error state (set OFF/ON), when error_temperature is received


#define HARDWARE_VERSION 0x01
#define FIRMWARE_VERSION 0x0032
#define HMW_DEVICETYPE 0x9C //device ID (make sure to import .xml into FHEM)

#define NUMBER_OF_TEMP_CHAN 6   // input channels - 1-wire temperature sensors
#define ADDRESS_START_CONF_TEMP_CHAN 0x7  // first EEPROM address for temperature sensors configuration
#define NUM_LINKS_TEMP 20    // requires Support_HBWLink_InfoEvent in HBWired.h
#define LINKADDRESSSTART_TEMP 0x100  // pering start_address for any sensor type peers, address_step has to be 6
#define NUMBER_OF_DELTAT_CHAN 3 // result output channels[, can peer with switch]
#define NUM_LINKS_DELTATX 6     // allow to peer input channels (T1 & T2) with one temperature sensor each
#define LINKADDRESSSTART_DELTATX 0x220  // step 7


//#define USE_HARDWARE_SERIAL   // use hardware serial (USART) for final device - this disables debug output
/* Undefine "HBW_DEBUG" in 'HBWired.h' to remove code not needed. "HBW_DEBUG" also works as master switch,
 * as hbwdebug() or hbwdebughex() used in channels will point to empty functions. */


// HB Wired protocol and module
#include <HBWired.h>
#include <HBWOneWireTempSensors.h>
//#include <HBWLinkInfoEventSensor.h>
#include "HBWLinkKeyInfoEventSensor.h"  // TODO: remove these files and add option to the lib, allowing to combine different LinkSender
#include <HBWLinkInfoEventActuator.h>
#include "HBWDeltaT.h"


// Pins
#ifdef USE_HARDWARE_SERIAL
  #define RS485_TXEN 2  // Transmit-Enable
  #define BUTTON A6  // Button fuer Factory-Reset etc.
  #define ADC_BUS_VOLTAGE A7  // analog input to measure bus voltage (optional)
  
  #define ONEWIRE_PIN	A3 // Onewire Bus
  #define RELAY_1 5
  #define RELAY_2 6
  #define RELAY_3 3
  
  #define BLOCKED_TWI_SDA A4  // used by I²C - SDA
  #define BLOCKED_TWI_SCL A5  // used by I²C - SCL
  
#else
  #define RS485_RXD 4
  #define RS485_TXD 2
  #define RS485_TXEN 3  // Transmit-Enable
  #define BUTTON 8  // Button fuer Factory-Reset etc.
  #define ADC_BUS_VOLTAGE A7  // analog input to measure bus voltage (optional)

  #define ONEWIRE_PIN	10 // Onewire Bus
  #define RELAY_1 5
  #define RELAY_2 6
  #define RELAY_3 A2

  #include "FreeRam.h"
  #include <HBWSoftwareSerial.h>
  HBWSoftwareSerial rs485(RS485_RXD, RS485_TXD); // RX, TX
#endif  //USE_HARDWARE_SERIAL

#define LED LED_BUILTIN        // Signal-LED


#define NUMBER_OF_CHAN NUMBER_OF_TEMP_CHAN + (NUMBER_OF_DELTAT_CHAN *3)


struct hbw_config {
  uint8_t logging_time;     // 0x01
  uint32_t central_address;  // 0x02 - 0x05
  uint8_t direct_link_deactivate:1;   // 0x06:0
  uint8_t              :7;   // 0x06:1-7
  hbw_config_onewire_temp TempOWCfg[NUMBER_OF_TEMP_CHAN]; // 0x07 - 0x5A (address step 14)
  hbw_config_DeltaT DeltaTCfg[NUMBER_OF_DELTAT_CHAN];     // 0x5B - 0x6F (address step 7)
  hbw_config_DeltaTx DeltaT1Cfg[NUMBER_OF_DELTAT_CHAN];  // 0x70 - 0x72 (address step 1)
  hbw_config_DeltaTx DeltaT2Cfg[NUMBER_OF_DELTAT_CHAN];  // 0x73 - 0x75 (address step 1)
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
               OneWire* oneWire = NULL, hbw_config_onewire_temp** _tempSensorconfig = NULL) :
               //,HBWLinkSender* _linkSenderTemp = NULL, HBWLinkReceiver* _linkReceiverTemp = NULL) :
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
void HBDTControlDevice::afterReadConfig()
{
  if(hbwconfig.logging_time == 0xFF) hbwconfig.logging_time = 50;
  
  HBWOneWireTemp::sensorSearch(d_ow, tempSensorconfig, (uint8_t) NUMBER_OF_TEMP_CHAN, (uint8_t) ADDRESS_START_CONF_TEMP_CHAN);
};

HBDTControlDevice* device = NULL;



void setup()
{
  // variables for all OneWire channels
  uint32_t g_owLastReadTime = 0;
  uint8_t g_owCurrentChannel = OW_CHAN_INIT; // always initialize with OW_CHAN_INIT value!
  OneWire* g_ow = new OneWire(ONEWIRE_PIN);

  // create channels
  for(uint8_t i = 0; i < NUMBER_OF_TEMP_CHAN; i++) {
    channels[i] = new HBWOneWireTemp(g_ow, &(hbwconfig.TempOWCfg[i]), &g_owLastReadTime, &g_owCurrentChannel);
    tempConfig[i] = &(hbwconfig.TempOWCfg[i]);
  }

  static const uint8_t relayPin[NUMBER_OF_DELTAT_CHAN] = {RELAY_1, RELAY_2, RELAY_3};  // assing pins
  HBWDeltaTx* deltaTxCh[NUMBER_OF_DELTAT_CHAN *2];  // pointer to deltaTx channels, to link in deltaT channels

  for(uint8_t i = 0; i < NUMBER_OF_DELTAT_CHAN; i++) {
    deltaTxCh[i] = new HBWDeltaTx( &(hbwconfig.DeltaT1Cfg[i]) );
    deltaTxCh[i +NUMBER_OF_DELTAT_CHAN] = new HBWDeltaTx( &(hbwconfig.DeltaT2Cfg[i]) );
    channels[i +NUMBER_OF_TEMP_CHAN] = new HBWDeltaT(relayPin[i], deltaTxCh[i], deltaTxCh[i +NUMBER_OF_DELTAT_CHAN], &(hbwconfig.DeltaTCfg[i]));
    channels[i +NUMBER_OF_TEMP_CHAN + NUMBER_OF_DELTAT_CHAN] = deltaTxCh[i];
    channels[i +NUMBER_OF_TEMP_CHAN + NUMBER_OF_DELTAT_CHAN *2] = deltaTxCh[i +NUMBER_OF_DELTAT_CHAN];
  }
  

#ifdef USE_HARDWARE_SERIAL  // RS485 via UART Serial, no debug (_debugstream is NULL)
  Serial.begin(19200, SERIAL_8E1);
  
  device = new HBDTControlDevice(HMW_DEVICETYPE, HARDWARE_VERSION, FIRMWARE_VERSION,
                             &Serial, RS485_TXEN, sizeof(hbwconfig), &hbwconfig,
                             NUMBER_OF_CHAN, (HBWChannel**)channels,
                             NULL,
                             new HBWLinkKeyInfoEventSensor<NUM_LINKS_TEMP, LINKADDRESSSTART_TEMP>(),
                             new HBWLinkInfoEventActuator<NUM_LINKS_DELTATX, LINKADDRESSSTART_DELTATX>(),
                             g_ow, tempConfig
                             );
  
  device->setConfigPins(BUTTON, LED);  // use analog input for 'BUTTON'
  device->setStatusLEDPins(LED, LED); //  using config LED for Tx & Rx indicator
  
#else
  Serial.begin(115200);  // Serial->USB for debug
  rs485.begin(19200);   // RS485 via SoftwareSerial, must use 19200 baud!
  
  device = new HBDTControlDevice(HMW_DEVICETYPE, HARDWARE_VERSION, FIRMWARE_VERSION,
                             &rs485, RS485_TXEN, sizeof(hbwconfig), &hbwconfig,
                             NUMBER_OF_CHAN, (HBWChannel**)channels,
                             &Serial,
                             new HBWLinkKeyInfoEventSensor<NUM_LINKS_TEMP, LINKADDRESSSTART_TEMP>(),
                             new HBWLinkInfoEventActuator<NUM_LINKS_DELTATX, LINKADDRESSSTART_DELTATX>(),
                             g_ow, tempConfig
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
  POWERSAVE();  // go sleep a bit
};


// check if HBWLinkInfoEvent support is enabled, when links are set
#if !defined(Support_HBWLink_InfoEvent) && defined(NUM_LINKS_TEMP)
#error enable/define Support_HBWLink_InfoEvent in HBWired.h
#endif
  
