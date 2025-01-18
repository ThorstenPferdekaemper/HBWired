/*
 * HBWDimmerVirtual.cpp
 *
 * Created (www.loetmeister.de): 10.05.2021
 * 
 */

#include "HBWDimmerVirtual.h"
#include <HBWDimmerAdvanced.h>

// Class HBWDimmerVirtual
HBWDimmerVirtual::HBWDimmerVirtual(HBWChannel* _dimChan, hbw_config_dim_virt* _config) :
dimChan(_dimChan),
config(_config)
{
  level = 255;  // inactive by default
}


void HBWDimmerVirtual::set(HBWDevice* device, uint8_t length, uint8_t const * const data)
{
  if (length == 1 && *(data) <= 200) {    // reference level set by FHEM/CCU; TODO: allow level > 200 to disable again??
    level = *(data);
  }
  else if (length == HBWDimmerAdvanced::NUM_PEER_PARAMS +2)
  {
    hbwdebug(F("vDim, onLvl:"));hbwdebug(data[D_POS_onLevel]);
    // create new (writable) array
    // uint8_t dataNew[HBWDimmerAdvanced::NUM_PEER_PARAMS +2];
    // memcpy(dataNew, data, HBWDimmerAdvanced::NUM_PEER_PARAMS +2);
    // dataNew[D_POS_onLevel] = handleLogic(dataNew[D_POS_onLevel]);//, level);
    // dimChan->set(device, length, dataNew);

    uint8_t new_onLevel = handleLogic(data[D_POS_onLevel]);
    memcpy((uint8_t*)&(data[D_POS_onLevel]), &new_onLevel, 1);  // overwrite onLevel byte in data[] array
    hbwdebug(F(" new"));hbwdebug(data[D_POS_onLevel]);hbwdebug(F("\n"));
    dimChan->set(device, length, data);
  }
};


/* standard public function - returns length of data array. Data array contains current channel reading */
uint8_t HBWDimmerVirtual::get(uint8_t* data)
{
  *data++ = level;
  *data = (level != 255) ? 48 : 0;  // level of 255 is "logic inactive" state
  
  return 2;
};

uint8_t HBWDimmerVirtual::handleLogic(uint8_t levelPeer)
{
  if (level > 200) return levelPeer;  // exit if we have no value for proper comparison. level have to be set first
  
  if (levelPeer > 200) levelPeer = 200;  // this would ignore onLevel = 201 (old_onlevel)... cannot handle it from here anyway..

  switch(config->logic)
  {
    case LOGIC_INACTIVE:
      // skip
    break;
    
  //  case LOGIC_ORINVERS:
    //  level = 200 - level; //?
    // levelPeer = 200 - levelPeer;
    // return (level > levelPeer ? level : levelPeer);

    case LOGIC_OR: // use larger value
      if (level > levelPeer) return level;
    break;
    
  //  case LOGIC_ANDINVERS: //?
    // levelPeer = 200 - levelPeer;
      // return level < levelPeer ? level : levelPeer;

    case LOGIC_AND: // use smaller value
      if (level < levelPeer) return level;
    break;
    
    case LOGIC_XOR: // one level has to be 0
      return (level == 0 ? levelPeer : (levelPeer == 0 ? level : 0));
    break;

    case LOGIC_NOR: // use larger value & invert
      return (200 - ((level > levelPeer) ? level : levelPeer));
    break;
    
    case LOGIC_NAND: // use smaller value & invert
      return (200 - ((level < levelPeer) ? level : levelPeer));
    break;
    
    case LOGIC_PLUS: // addition (max 100%)
      return ((uint16_t)(levelPeer + level) > 200) ? 200 : (levelPeer + level);
    break;
    
    case LOGIC_MINUS: // substraction (min 0%)
      return levelPeer - ((levelPeer > level) ? level : levelPeer);
    break;
    default: break;
  }
  return levelPeer;
};

