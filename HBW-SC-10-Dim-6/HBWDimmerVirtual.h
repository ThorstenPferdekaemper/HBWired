/* 
 *  HBWDimmerVirtual.h
 *  
 *  HBWDimmerVirtual should be used with a dimmer channel (HBWDimmerAdvanced & HBWLinkDimmerAdvanced).
 *  It allows to influence the on_level of the physical dimmer channel, by combining the level set
 *  from peering with the level of its own channel with various logic.
 *  Not all logic combinations are implemneted (may also not useful for a single virtual channel).
 *  
 *  Flow: key ->(peering)-> dimmer_virtual ->(levels + logic)-> dimmer channel
 *
 *  TODO: A virtual dimmer channel should actually have a full state machine...
 */

#ifndef HBWDimmerVirtual_h
#define HBWDimmerVirtual_h

#include <inttypes.h>
#include "HBWired.h"
#include <HBWDimmerAdvanced.h>

#define DEBUG_OUTPUT


// config of each virtual dimmer channel, address step 1
struct hbw_config_dim_virt {
  uint8_t logic:5;     //    LOGIC_INACTIVE, LOGIC_OR, ...
  uint8_t       :3;         // not used
};


// Class HBWDimmerVirtual
class HBWDimmerVirtual : public HBWChannel {
  public:
    HBWDimmerVirtual(HBWChannel* _dimChan, hbw_config_dim_virt* _config);
    // virtual void loop(HBWDevice*, uint8_t channel);
    virtual void set(HBWDevice*, uint8_t length, uint8_t const * const data);
    virtual uint8_t get(uint8_t* data);

    enum logic {
      LOGIC_INACTIVE = 0,
      LOGIC_OR,
      LOGIC_AND,
      LOGIC_XOR,
      LOGIC_NOR,
      LOGIC_NAND,
      LOGIC_ORINVERS,
      LOGIC_ANDINVERS,
      LOGIC_PLUS,
      LOGIC_MINUS,
      LOGIC_MULTI,
      LOGIC_PLUSINVERS,
      LOGIC_MINUSINVERS,
      LOGIC_MULTIINVERS,
      LOGIC_INVERSPLUS,
      LOGIC_INVERSMINUS,
      LOGIC_INVERSMULTI
	};

  private:
    HBWChannel* dimChan;   // mapped dimmer channel (or another virtual dimmer?)
    hbw_config_dim_virt* config;
    // uint8_t dimMaxLevel; // needed?? only via peering
    //uint8_t onLevel;  // level from peering - need to be stored?
    uint8_t level;  // level (0...100%) set for the channel to use in LOGIC condition

    //void handleLogic(uint8_t logic, uint8_t const * const data, uint8_t* dataNew);
    uint8_t handleLogic(uint8_t levelPeer, uint8_t _level);
};

#endif
