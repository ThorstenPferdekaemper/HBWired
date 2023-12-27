  /*
 * HBWPids.cpp
 *
 * Created on: 15.04.2019
 * loetmeister.de
 * 
 * Author: Harald Glaser
 * some parts from the Arduino PID Library
 * by Brett Beauregard <br3ttb@gmail.com> brettbeauregard.com
 */
 
#include "HBWPids.h"


HBWPids::HBWPids(HBWValve* _valve, hbw_config_pid* _config)
{
  config = _config;
  valve = _valve;
  
  windowStartTime = 0;
  lastPidTime = 0;
  initDone = false;
  inErrorState = false;
  //pid lib
  input = DEFAULT_TEMP; // force channel to manual, of no input temperature is received
  output = 0;
  outputSum = 0;
}


/*
 * set Pids Parameter on Start and when changed in EEProm
 */
// channel specific settings or defaults
void HBWPids::afterReadConfig()
{
  if (config->kp == 0xFFFF)  config->kp = 1000;
  if (config->ki == 0xFFFF)  config->ki = 50; //todo do i need size 2 for autotune 0,5
  if (config->kd == 0xFFFF)  config->kd = 10; //dito 0,1
  if (config->windowSize == 0xFF || config->windowSize == 0)  config->windowSize = 60; // 10min

  setOutputLimits(200);  // map from 0 to 200 (i.e. 0 to 100%) for valve channel
  setTunings((float) config->kp, (float) config->ki / 100, (float) config->kd / 100);

  if (!initDone)  // only on device start - avoid to overwrite current output or inAuto mode
  {
    setPoint = (config->setPoint == 0xFF) ? DEFAULT_SETPOINT : (int16_t) config->setPoint *10;
    inAuto = config->startMode; // 1 automatic ; 0 manual
    valve->setPidsInAuto(inAuto);
    initDone = true;
  }
  setMode(inAuto);
  
#ifdef DEBUG_OUTPUT
  hbwdebug(F("pid_cfg, windowSize: ")); hbwdebug(config->windowSize);
  hbwdebug(F(" setPoint: ")); hbwdebug(setPoint);
  hbwdebug(F("\n"));
#endif
}


/* set special input value for a channel, via peering event. */
void HBWPids::setInfo(HBWDevice* device, uint8_t length, uint8_t const * const data)
{
  if (length == 2)  // temp value must have 2 bytes
  {
    input = ((data[0] << 8) | data[1]);  // set pid input (actual) temperature
    if (input < ERROR_TEMP)  input = ERROR_TEMP;
  #ifdef DEBUG_OUTPUT
  hbwdebug(F("setInfo: ")); hbwdebug(input); hbwdebug(F("\n"));
  #endif
  }
}


/* standard public function - set a channel, directly or via peering event. Data array contains new value or all peering details */
void HBWPids::set(HBWDevice* device, uint8_t length, uint8_t const * const data)
{
  if (length == 1)
  {
    // toogle autotune
    autoTuneRunning = !autoTuneRunning;
    //TODO, toggle with specific value? e.g. 255?
    // TODO: reply with INFO_AUTOTUNE & AUTOTUNE_FLAGS message
    
  #ifdef DEBUG_OUTPUT
  hbwdebug(F(" setPidsTemp autotune "));
  #endif
  }
  else //if (length > 1)
  {
    //set desired temperature
    uint16_t level = ((data[0] << 8) | data[1]);
    // right limits 0 - 30 °C
    if (level > 0 && level <= 3000) {
      setPoint = level;
    }
  }
}


/* standard public function - returns length of data array. Data array contains current channel reading */
uint8_t HBWPids::get(uint8_t* data)
{
  //TODO: add state_flags?
  // retVal = (pidConf[channel].setPoint << 8 | (pidConf[channel].autoTune << 4));

  //TODO: add? also return deviation_temp = (setPoint - input)
  
  // return desired temperature
  // MSB first
  *data++ = (setPoint >> 8);
  *data = setPoint & 0xFF;

  return 2;
}


void HBWPids::autoTune()
{
	// TODO: autotune
}


/* standard public function - called by device main loop for every channel in sequential order */
void HBWPids::loop(HBWDevice* device, uint8_t channel)
{
  if (millis() < 15000)  return; // wait 15 sec on start
    
  // start the first time
  // can't send everything at once to the bus, add an initial delay between channels
  if (windowStartTime == 0)
  {
    windowStartTime = (uint32_t) millis() - ((channel + 1) * 2000);
    
   #ifdef DEBUG_OUTPUT
   hbwdebug(F("PID ch: ")); hbwdebug(channel);
   hbwdebug(F(" temp ")); hbwdebug(input);
   hbwdebug(F(" starting...\n"));
   #endif
    return;
  }

  // apply new inAuto mode, if changed at the valve chan (we cannot set a PID chan to auto/manual directly)
  inAuto = valve->getPidsInAuto();

  // input temperature value comes back again
  if (inErrorState) {
    if (input > ERROR_TEMP) {
      inErrorState = false;
      setMode(oldInAuto);
      
      uint8_t newMode = inAuto ? SET_AUTOMATIC : SET_MANUAL;
      valve->set(device, sizeof(newMode), &newMode, SET_BY_PID);  // set new mode, force by setByPID = true
    }
  }
  
  // check if we had a temperature and in automatic mode
  // in manual mode we can ignore the temp
  if (inAuto == AUTOMATIC && (input == DEFAULT_TEMP || input == ERROR_TEMP)) {
    inErrorState = true;
    oldInAuto = inAuto;
    setMode(MANUAL);
    
    uint8_t newMode = SET_MANUAL;  // setting valve to manual will apply error position
    valve->set(device, sizeof(newMode), &newMode, SET_BY_PID);  // set new mode, force by setByPID = true
  }
  
  // compute the PID output
  // get output from PID. Doesn't do anything in Manual mode.
  compute();
  
  unsigned long now = millis();
  // new window
  if (now - windowStartTime > (uint32_t) config->windowSize * 10000)
  {
    uint8_t valveStatus = (uint8_t) output;  // mapped already from 0 to 200 (i.e. 0 to 100%)
    
    if (inAuto)   // only if inAuto (valve set() will only apply new level)
    {
      valve->set(device, sizeof(valveStatus), &valveStatus, SET_BY_PID);  // setByPID = true
    }
    windowStartTime = now;
  #ifdef DEBUG_OUTPUT
  hbwdebug(F("computePid ch: ")); hbwdebug(channel);
  hbwdebug(F(" inAuto: ")); hbwdebug(inAuto);
  hbwdebug(F(" windowSize: ")); hbwdebug((uint16_t)config->windowSize *10);
  hbwdebug(F(" output: ")); hbwdebug(output);
  hbwdebug(F(" desiredValveLevel: ")); hbwdebug(valveStatus);
  hbwdebug(F(" input: ")); hbwdebug(input);
  hbwdebug(F(" setpoint: ")); hbwdebug(setPoint);
  hbwdebug(F(" outMax: "));hbwdebug(outMax);
  hbwdebug(F(" Kp: "));hbwdebug(kp);
  hbwdebug(F(" Ki: "));hbwdebug(ki);
  hbwdebug(F(" Kd: "));hbwdebug(kd);
  hbwdebug(F("\n"));
  #endif
  }
  //  TODO: check if should/can ignore 1% changes?
}


/* Compute() **********************************************************************
 *     This, as they say, is where the magic happens.  this function should be called
 *   every time "void loop()" executes.  the function will decide for itself whether
 *   a new pid output needs to be computed.
 *   https://github.com/br3ttb/Arduino-PID-Library/
 **********************************************************************************/
void HBWPids::compute()
{
  if (!inAuto)  return;
  
	if ((millis() - lastPidTime) >= sampleTime)
	{
		/*Compute all the working error variables*/
		int16_t error = setPoint - input;
		
		outputSum += (ki * error);
		
		if (outputSum > outMax)	outputSum = outMax;
		else if (outputSum < 0)	outputSum = 0;
		
		int16_t dInput = input - lastInput;

		/*Compute PID output*/
		output = (kp * error + (outputSum - kd * dInput));
		
		if (output > outMax)	output = outMax;
		else if (output < 0)	output = 0;

		/*Remember some variables for next time*/
		lastInput = input;
		lastPidTime = millis();
	}
}


void HBWPids::setTunings(double Kp, double Ki, double Kd)
{
	if (Kp < 0 || Ki < 0 || Kd < 0)
		return;

	double SampleTimeInSec = ((double) sampleTime) / 1000;
	kp = Kp;
	ki = Ki * SampleTimeInSec;
	kd = Kd / SampleTimeInSec;
}

#ifndef FIXED_SAMPLE_TIME
void HBWPids::setSampleTime(int NewSampleTime)
{
	if (NewSampleTime > 0)
	{
		double ratio = (double) NewSampleTime / (double) sampleTime;
		ki *= ratio;
		kd /= ratio;
		sampleTime = (uint32_t) NewSampleTime;
	}
}
#endif

void HBWPids::setOutputLimits(double Max)
{
	outMax = Max;

	if (output > outMax)	output = outMax;
	else if (output < 0)	output = 0;

	if (outputSum > outMax)	outputSum = outMax;
	else if (outputSum < 0)	outputSum = 0;
}


/* SetMode(...)****************************************************************
 * Allows the controller Mode to be set to manual (0) or Automatic (1)
 * when the transition from manual to auto occurs, the controller is
 * automatically initialized
 ******************************************************************************/
void HBWPids::setMode(bool Mode)
{
	bool newAuto = (Mode == AUTOMATIC);
	if (newAuto && !inAuto)
	{ /*we just went from manual to auto*/
		initialize();
	}
	inAuto = newAuto;
}


/* Initialize()****************************************************************
 *  does all the things that need to happen to ensure a bumpless transfer
 *  from manual to automatic mode.
 ******************************************************************************/
void HBWPids::initialize()
{
  lastInput = input;
  outputSum = output;
  
  if (outputSum > outMax)   outputSum = outMax;
  else if (outputSum < 0)   outputSum = 0;
}

/*
 * map without in_min and out_min
 */
int32_t HBWPids::mymap(double x, double in_max, double out_max)
{
	return (x * out_max) / in_max;
}
