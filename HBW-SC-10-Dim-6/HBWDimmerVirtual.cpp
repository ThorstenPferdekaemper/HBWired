/*
 * HBWDimmerVirtual.cpp
 *
 * Created (www.loetmeister.de): 10.05.2021
 * 
 */

#include "HBWDimmerVirtual.h"

// Class HBWDimmerVirtual
HBWDimmerVirtual::HBWDimmerVirtual(HBWChannel* _dimChan, hbw_config_dim_virt* _config) :
dimChan(_dimChan),
config(_config)
{
  level = 255;  // inactive by default
}


void HBWDimmerVirtual::set(HBWDevice* device, uint8_t length, uint8_t const * const data)
{
  if (length == 1 && *(data) <= 200) {    // level set by FHEM/CCU
    level = *(data);
  }
  else if (length == NUM_PEER_PARAMS +1)
  {
    // create new (writable) array
    uint8_t dataNew[NUM_PEER_PARAMS +1];
    memcpy(dataNew, data, NUM_PEER_PARAMS +1);
    
    //handleLogic(config->logic, data, dataNew);
    dataNew[D_POS_onLevel] = handleLogic(dataNew[D_POS_onLevel], level);
    //dataNew[D_POS_dimMaxLevel] = handleLogic(dataNew[D_POS_dimMaxLevel], level);  // TODO: also check dimMaxLevel in the same way?

    dimChan->set(device, length, dataNew);
  }
};


/* standard public function - returns length of data array. Data array contains current channel reading */
uint8_t HBWDimmerVirtual::get(uint8_t* data)
{
  *data++ = level;
  *data = (level != 255) ? 48 : 0;  // level of 255 is "logic inactive" state
  
  return 2;
};

uint8_t HBWDimmerVirtual::handleLogic(uint8_t levelPeer, uint8_t _level)
//uint8_t HBWDimmerVirtual::handleLogic(uint8_t levelPeer)
{
  if (_level > 200) return levelPeer;  // exit if we have no value for proper comparison. level have to set first
  
  int levelNew = levelPeer;

  //switch(logic)
  switch(config->logic)
  {
    case LOGIC_INACTIVE:
      // skip
    break;
    
//    case LOGIC_ORINVERS:
//      _level = 200 - _level; //?
    case LOGIC_OR: // use larger value
      if (_level > levelPeer) return _level;
    break;
    
//    case LOGIC_ANDINVERS: //?
    case LOGIC_AND: // use smaller value
      if (_level < levelPeer) return _level;
    break;
    
//    case LOGIC_XOR: //?
//    break;

    case LOGIC_NOR: // use larger value & invert
      if (_level > levelPeer) levelNew = _level;
      return (uint8_t)(200 - levelNew);
    break;
    
    case LOGIC_NAND: // use smaller value & invert
      if (_level < levelPeer) levelNew = _level;
      return (uint8_t)(200 - levelNew);
    break;
    
    case LOGIC_PLUS: // addition (max 100%)
      levelNew = levelPeer + _level;
      if (levelNew > 200) return 200;
      else return (uint8_t)levelNew;
    break;
    
    case LOGIC_MINUS: // substraction (min 0%)
      levelNew = levelPeer - _level;
      if (levelNew < 0) return 0;
      else return (uint8_t)levelNew;
    break;
  }
  return levelPeer;
};

//void HBWDimmerVirtual::handleLogic(uint8_t logic, uint8_t const * const data, uint8_t* dataNew)
//{
//  switch(logic) {
//    case LOGIC_INACTIVE:
//      // skip
//    break;
////    case LOGIC_ORINVERS: //?
//    case LOGIC_OR: // use larger value
//      if (level > data[D_POS_onLevel]) dataNew[D_POS_onLevel] = level;
//    break;
////    case LOGIC_ANDINVERS: //?
//    case LOGIC_AND: // use smaller value
//      if (level < data[D_POS_onLevel]) dataNew[D_POS_onLevel] = level;
//    break;
////    case LOGIC_XOR: //?
////    break;
//    case LOGIC_NOR: // use larger value & invert
//      if (level > data[D_POS_onLevel]) dataNew[D_POS_onLevel] = level;
//      dataNew[D_POS_onLevel] = 200 - dataNew[D_POS_onLevel];
//      //dataNew[D_POS_onLevel] = 200 - (level > data[D_POS_onLevel] ? level : data[D_POS_onLevel]);
//    break;
//    case LOGIC_NAND: // use smaller value & invert
//      if (level < data[D_POS_onLevel]) dataNew[D_POS_onLevel] = level;
//      dataNew[D_POS_onLevel] = 200 - dataNew[D_POS_onLevel];
//      //dataNew[D_POS_onLevel] = 200 - (level < data[D_POS_onLevel] ? level : data[D_POS_onLevel]);
//    break;
//    case LOGIC_PLUS: // addition (max 100%)
//    {
//      int sum = data[D_POS_onLevel] + level;
//      if (sum > 200) dataNew[D_POS_onLevel] = 200;
//      else dataNew[D_POS_onLevel] = (uint8_t)sum;
//    }
//    break;
//    case LOGIC_MINUS: // substraction (min 0%)
//    {
//      int dif = data[D_POS_onLevel] - level;
//      if (dif < 0) dataNew[D_POS_onLevel] = 0;
//      else dataNew[D_POS_onLevel] = (uint8_t)dif;
//    }
//    break;
//    
//  }
//};
