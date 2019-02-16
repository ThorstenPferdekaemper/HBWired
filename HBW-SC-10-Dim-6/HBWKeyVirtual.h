#ifndef HBWKeyVirtual_h
#define HBWKeyVirtual_h

#include <inttypes.h>
#include "HBWired.h"


#define DEBUG_OUTPUT

#define KEY_DELAY_TIME 700  // ms



struct hbw_config_key_virt {
  uint8_t input_locked:1;   // 0x07:0    1=LOCKED, 0=UNLOCKED
  uint8_t n_inverted:1;     // 0x07:1    0=inverted, 1=not inverted
  uint8_t       :6;         // 0x07:2-7
  uint8_t threshold;	      // compare to channel value (0 - 200) // TODO: remove
};


// Class HBWKeyVirtual
class HBWKeyVirtual : public HBWChannel {
  public:
    HBWKeyVirtual(uint8_t _mappedChan, hbw_config_key_virt* _config);
    virtual void loop(HBWDevice*, uint8_t channel);
//    virtual void afterReadConfig();

  private:
    uint8_t mappedChan;   // mapped channel number
    uint8_t keyPressNum;
    hbw_config_key_virt* config;
    uint32_t keyPressedMillis;  // Zeit, zu der die Taste gedrueckt wurde (fuer's Entprellen)
    boolean sendLong;
    boolean lastSentLong;      // Zuletzt gesender "Tastendruck" long oder short
};

#endif
