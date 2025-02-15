/* 
 *  HBWKeyVirtual.h
 *  
 *  HBWKeyVirtual can be used to read actor channels, like switch or dimmer
 *  It will send a short KeyEvent, if the attached channel (mappedChan) state not 0
 *  and long KeyEvent for state equals 0.
 *  HBWKeyVirtual can be peered like any normal key channel (using HBWLinkKey.h)
 */
 // TODO: link this by dimmer channel, to have a trigger and avoid polling? (e.g. HBWDimmerAdvanced(uint8_t _pin, hbw_config_dim* _config, HBWKeyVirtual* _vKey))

#ifndef HBWKeyVirtual_h
#define HBWKeyVirtual_h

#include <inttypes.h>
#include "HBWired.h"


// #define DEBUG_OUTPUT


// config of each virtual key channel, address step 1
struct hbw_config_key_virt {
  uint8_t input_locked:1;   // 0x07:0    default 1=LOCKED, 0=UNLOCKED
  uint8_t n_inverted:1;     // 0x07:1    0=inverted, 1=not inverted
  uint8_t n_update_on_start:1;     // 0x07:2    0 = send key event on device start, 1 = don't
  uint8_t       :5;         // 0x07:3-7
  //TODO: use 3 bit for OFF_DELAY_TIME? (0..7 seconds?)
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
    boolean sendLong;          // Zu sendender "Tastendruck" long (=true) oder short
    boolean lastSentLong;      // Zuletzt gesender "Tastendruck" long oder short
    boolean updateDone;

    static const uint16_t OFF_DELAY_TIME = 2600;  // ms (should be multiple of POLLING_WAIT_TIME)
    static const uint16_t POLLING_WAIT_TIME = 325;  // ms, get linked channel state not faster than this
};

#endif
