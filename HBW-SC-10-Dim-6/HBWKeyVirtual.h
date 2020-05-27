#ifndef HBWKeyVirtual_h
#define HBWKeyVirtual_h

#include <inttypes.h>
#include "HBWired.h"


#define DEBUG_OUTPUT


struct hbw_config_key_virt {
  uint8_t input_locked:1;   // 0x07:0    1=LOCKED, 0=UNLOCKED
  uint8_t n_inverted:1;     // 0x07:1    0=inverted, 1=not inverted
  uint8_t       :6;         // 0x07:2-7
  //TODO: add option to force update on start? (forceUpdate = !config->update_on_start;)
  //uint8_t dummy;    //TODO: use for OFF_DELAY_TIME? (0-254 seconds?)
};


// Class HBWKeyVirtual
class HBWKeyVirtual : public HBWChannel {
  public:
    HBWKeyVirtual(uint8_t _mappedChan, hbw_config_key_virt* _config);
    virtual void loop(HBWDevice*, uint8_t channel);


  private:
    uint8_t mappedChan;   // mapped channel number
    hbw_config_key_virt* config;
    
    uint8_t keyPressNum;
    uint32_t keyPressedMillis;  // Zeit, zu der die Taste gedrueckt wurde (fuer's Entprellen)
    boolean sendLong;
    boolean lastSentLong;      // Zuletzt gesender "Tastendruck" long oder short
    //boolean forceUpdate;

    static const uint16_t OFF_DELAY_TIME = 2600;  // ms
    static const uint16_t POLLING_WAIT_TIME = 330;  // get linked channel state every 330 ms
};

#endif
