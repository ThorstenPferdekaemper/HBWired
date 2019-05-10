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
  
  pidConf.windowStartTime = 0;
  pidConf.oldInAuto = 0; // we switch to MANUAL in error Position. Store the old value here
  pidConf.initDone = false;
  pidConf.error = 0;
  //pid lib
  pidConf.Input = DEFAULT_TEMP; // force channel to manual, of no input temperature is received
  pidConf.Output = 0;
  pidConf.ITerm = 0;
  pidConf.sampleTime = 2000; // pid compute every 2 sec
  pidConf.setPoint = 2200; // default? 22.00°C
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
  if (config->windowSize == 0xFFFF)  config->windowSize = 600; // 10min

	setOutputLimits((uint32_t) config->windowSize * 1000);
	setTunings((float) config->kp, (float) config->ki / 100, (float) config->kd / 100);

  if (!pidConf.initDone)  // only on device start - avoid to overwrite current output or inAuto mode
  {
    pidConf.inAuto = config->startMode; // 1 automatic ; 0 manual
    valve->setPidsInAuto(pidConf.inAuto);
   //pidConf.Output = mymap(valve->getErrorPos(), 200.0, (uint32_t) config->windowSize * 1000);
    pidConf.initDone = true;
  }
  setMode(pidConf.inAuto);
}


/* set special input value for a channel, via peering event. */
void HBWPids::setInfo(HBWDevice* device, uint8_t length, uint8_t const * const data)
{
  if (length == 2)  // desired_temp has 2 bytes
  {
    pidConf.Input = ((data[0] << 8) | data[1]);  // set desired_temp
  #ifdef DEBUG_OUTPUT
  hbwdebug(F("setInfo: ")); hbwdebug(pidConf.Input); hbwdebug(F("\n"));
  #endif
  }
}


/* standard public function - set a channel, directly or via peering event. Data array contains new value or all peering details */
void HBWPids::set(HBWDevice* device, uint8_t length, uint8_t const * const data)
{
  if (length == 1)
  {
    // toogle autotune
    pidConf.autoTune = !pidConf.autoTune;
    //TODO, toggle with specific value? e.g. 255?
    // TODO: reply with INFO_AUTOTUNE & AUTOTUNE_FLAGS message
    
  #ifdef DEBUG_OUTPUT
  hbwdebug(F(" setPidsTemp autotune "));
  #endif
  }
  else //if (length > 1)
  {
  //set temperature
  uint16_t level = ((data[0] << 8) | data[1]);
    // right limits 0 - 30 °C
    if (level > 0 && level <= 3000) {
      pidConf.setPoint = level;
    }
  }
}


/* standard public function - returns length of data array. Data array contains current channel reading */
uint8_t HBWPids::get(uint8_t* data)
{
  //TODO: add state_flags? -> seperate get() function?
  // retVal = (pidConf[channel].setPoint << 8 | (pidConf[channel].autoTune << 4));
  
  // return desired temperature
  // MSB first
  *data++ = (pidConf.setPoint >> 8);
  *data = pidConf.setPoint & 0xFF;

	return 2;
}


void HBWPids::autoTune()
{
	// TODO: autotune
}


/* standard public function - called by device main loop for every channel in sequential order */
void HBWPids::loop(HBWDevice* device, uint8_t channel)
{
	uint32_t now = millis();

	if (now < 15000)  return; // wait 15 sec on start
    
	// start the first time
	// can't sending everything at once to the bus.
	// so make a delay between channels
	if (pidConf.windowStartTime == 0) {
	  pidConf.windowStartTime = now - ((channel + 1) * 2000);
    pidConf.lastPidTime = now - pidConf.sampleTime;
    
  #ifdef DEBUG_OUTPUT
  hbwdebug(F("PID ch: ")); hbwdebug(channel);
  hbwdebug(F(" temp ")); hbwdebug(pidConf.Input);
  hbwdebug(F(" starting...\n"));
  #endif
		return;
	}

  if (pidConf.inAuto != valve->getPidsInAuto()) {
    // apply new inAuto mode, if changed at the valve chan (we cannot set a PID chan to auto/manual directly)
    pidConf.inAuto = valve->getPidsInAuto();
  }

  // error temp values comes back again
  if (pidConf.error) {
  	if (pidConf.Input > ERROR_TEMP) {
  		pidConf.error = 0;
  		setMode(pidConf.oldInAuto);
      
      uint8_t newMode = pidConf.inAuto ? SET_AUTOMATIC : SET_MANUAL;
      valve->set(device, 1, &newMode);   // set new mode
  	}
  }
  
  // check if we had a temperature and in automatic mode
  // in manual mode we can ignore the temp
  if (pidConf.inAuto == AUTOMATIC && (pidConf.Input == DEFAULT_TEMP || pidConf.Input == ERROR_TEMP)) {
  	pidConf.error = 1;
  	pidConf.oldInAuto = pidConf.inAuto;
    setMode(MANUAL);
    
    uint8_t newMode = SET_MANUAL;  // setting valve to manual will apply error position
    valve->set(device, 1, &newMode);   // set new mode
  }
  
  // compute the PID Output
  if (computePid(channel))
  {
    valve->set(device, 1, &desiredValveLevel, true);  // setByPID = true
  }
}


int8_t HBWPids::computePid(uint8_t channel)
{
  uint32_t now = millis();
  uint8_t retVal = 0;
  
  // get Output from PID. Doesn't do anything in Manual mode.
  compute();
  
  // new window
  if (now - pidConf.windowStartTime > (uint32_t) config->windowSize * 1000)
  {
    uint16_t valveStatus = (uint16_t) mymap(pidConf.Output, (uint32_t) config->windowSize *1000, 200.0); //map from 0 to 200
    if (pidConf.inAuto) {   // only if inAuto (valve set() will only apply new level)
    	retVal = 1;
    }
    desiredValveLevel = valveStatus;
    pidConf.windowStartTime = now;
  #ifdef DEBUG_OUTPUT
  hbwdebug(F("computePid ch: ")); hbwdebug(channel);
  hbwdebug(F(" inAuto: ")); hbwdebug(pidConf.inAuto);
  hbwdebug(F(" windowSize: ")); hbwdebug(config->windowSize);
  hbwdebug(F(" output: ")); hbwdebug(pidConf.Output);
  hbwdebug(F(" desiredValveLevel: ")); hbwdebug(desiredValveLevel);
  hbwdebug(F(" input: ")); hbwdebug(pidConf.Input);
  hbwdebug(F(" setpoint: ")); hbwdebug(pidConf.setPoint);
  hbwdebug(F(" outMax: "));hbwdebug(pidConf.outMax);
  hbwdebug(F(" Kp: "));hbwdebug(pidConf.kp);
  hbwdebug(F(" Ki: "));hbwdebug(pidConf.ki);
  hbwdebug(F(" Kd: "));hbwdebug(pidConf.kd);
  hbwdebug(F("\n"));
  #endif
	}
  // under 2 seconds or under 1% of windowsize we do nothing.
  // it makes no sense to send on's and off's over the bus in
  // such a short time
//  TODO: check if shoud/can ignore 1% changes?

  return retVal;
}

/* Compute() **********************************************************************
 *     This, as they say, is where the magic happens.  this function should be called
 *   every time "void loop()" executes.  the function will decide for itself whether
 *   a new pid Output needs to be computed.
 *   https://github.com/br3ttb/Arduino-PID-Library/
 **********************************************************************************/
void HBWPids::compute()
{
  if (!pidConf.inAuto)  return;
   
	uint32_t now = millis();
  
	if ((now - pidConf.lastPidTime) >= pidConf.sampleTime) {
		/*Compute all the working error variables*/
		int16_t error = pidConf.setPoint - pidConf.Input;
		pidConf.ITerm += (pidConf.ki * error);
		if (pidConf.ITerm > pidConf.outMax)
			pidConf.ITerm = pidConf.outMax;
		else if (pidConf.ITerm < 0)
			pidConf.ITerm = 0;
		int16_t dInput = pidConf.Input - pidConf.lastInput;

		/*Compute PID Output*/
		pidConf.Output = (pidConf.kp * error + (pidConf.ITerm - pidConf.kd * dInput));
		if (pidConf.Output > pidConf.outMax)
			pidConf.Output = pidConf.outMax;
		else if (pidConf.Output < 0)
			pidConf.Output = 0;

		/*Remember some variables for next time*/
		pidConf.lastInput = pidConf.Input;
		pidConf.lastPidTime = now;
	}
}


void HBWPids::setTunings(double Kp, double Ki, double Kd)
{
	if (Kp < 0 || Ki < 0 || Kd < 0)
		return;

	double SampleTimeInSec = ((double) pidConf.sampleTime) / 1000;
	pidConf.kp = Kp;
	pidConf.ki = Ki * SampleTimeInSec;
	pidConf.kd = Kd / SampleTimeInSec;
}


void HBWPids::setSampleTime(int NewSampleTime)
{
	if (NewSampleTime > 0) {
		double ratio = (double) NewSampleTime / (double) pidConf.sampleTime;
		pidConf.ki *= ratio;
		pidConf.kd /= ratio;
		pidConf.sampleTime = (uint32_t) NewSampleTime;
	}
}


void HBWPids::setOutputLimits(uint32_t Max)
{
	pidConf.outMax = Max;

	if (pidConf.Output > pidConf.outMax) {
		pidConf.Output = pidConf.outMax;
	}
	else if (pidConf.Output < 0) {
		pidConf.Output = 0;
	}

	if (pidConf.ITerm > pidConf.outMax) {
		pidConf.ITerm = pidConf.outMax;
	}
	else if (pidConf.ITerm < 0) {
		pidConf.ITerm = 0;
	}
}


/* SetMode(...)****************************************************************
 * Allows the controller Mode to be set to manual (0) or Automatic (1)
 * when the transition from manual to auto occurs, the controller is
 * automatically initialized
 ******************************************************************************/
void HBWPids::setMode(bool Mode)
{
	bool newAuto = (Mode == AUTOMATIC);
	if (newAuto && !pidConf.inAuto) { /*we just went from manual to auto*/
		initialize();
	}
	pidConf.inAuto = newAuto;
}


/* Initialize()****************************************************************
 *  does all the things that need to happen to ensure a bumpless transfer
 *  from manual to automatic mode.
 ******************************************************************************/
void HBWPids::initialize()
{
  pidConf.lastInput = pidConf.Input;
  pidConf.ITerm = pidConf.Output;
  if (pidConf.ITerm > pidConf.outMax)
    pidConf.ITerm = pidConf.outMax;
  else if (pidConf.ITerm < 0)
    pidConf.ITerm = 0;
}

/*
 * map without in_min and out_min
 */
int32_t HBWPids::mymap(double x, double in_max, double out_max)
{
	return (x * out_max) / in_max;
}
