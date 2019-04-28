/*
 * HMWPids.cpp
 *
 * Created on: 15.04.2019
 * loetmeister.de
 * 
 * Author: Harald Glaser
 * some parts from the Arduino PID Library
 * by Brett Beauregard <br3ttb@gmail.com> brettbeauregard.com
 */
 
#include "HMWPids.h"


HBWPids::HBWPids(hbw_config_pid_valve* _configValve, hbw_config_pid* _config)
{
  config = _config;
  configValve = _configValve;
  
  pidConf.windowStartTime = 0;
  pidConf.upDown = 0; // 0 up; 1 down
  pidConf.oldInAuto = 0; // we switch to MANUAL in error Position. Store the old value here
  pidConf.status = 0;
  pidConf.initDone = false;
  //pid lib
  pidConf.Input = 0;
  pidConf.Output = 0;
  pidConf.ITerm = 0;
  pidConf.sampleTime = 2000; // pid compute every 2 sec
  pidConf.outMax = 0;
  pidConf.setPoint = 2300; // default? 22.00°C?
}


HBWPidsValve::HBWPidsValve(uint8_t _pin, HBWPids* _pid, hbw_config_pid_valve* _config)
{
  config = _config;
  pin = _pin;
  pid = _pid; // linked PID channel

  pidValveConf.lastSentTime = 0;
//pidValveConf.counter = 0;  // key press counter?
  digitalWrite(pin, LOW);
  pinMode(pin, OUTPUT);
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
  
  if (configValve->error_pos == 0xFF)  configValve->error_pos = 30;   // check PidsValve setting, as we use it now
	
	setOutputLimits(config->windowSize * 1000);
	setTunings((float) config->kp, (float) config->ki / 100, (float) config->kd / 100);

  if (!pidConf.initDone)  // avoid to overwrite current output or inAuto mode
  {
    pidConf.inAuto = config->startMode; // 1 automatic ; 0 manual
    pidConf.Output = mymap(configValve->error_pos, 200.0, config->windowSize * 1000);
    pidConf.outputMap = mymap(configValve->error_pos, 200.0, MAPSIZE);
    pidConf.initDone = true;
  }
  setMode(pidConf.inAuto);
}


// channel specific settings or defaults
void HBWPidsValve::afterReadConfig()
{
  if (config->send_max_interval == 0xFFFF)  config->send_max_interval = 150;
  if (config->error_pos == 0xFF)  config->error_pos = 30;

  pidValveConf.status = !pid->getPidsValveStatus();
}


void HBWPids::setInfo(HBWDevice* device, uint8_t length, uint8_t const * const data)
{
  if (length == 2)
  {
    pidConf.Input = ((data[0] << 8) | data[1]);
#ifdef DEBUG_OUTPUT
  hbwdebug(F("setInfo: ")); hbwdebug(pidConf.Input);
  hbwdebug(F("\n"));
#endif
  }
}


// setPidsTemp
void HBWPids::set(HBWDevice* device, uint8_t length, uint8_t const * const data)
{
  if (length == 1)
  {
    // toogle autotune
    pidConf.autoTune = !pidConf.autoTune;
    //TODO, toggle with specific value? e.g. 255?
    
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


//getPidsTemp
uint8_t HBWPids::get(uint8_t* data)
{
// todo which commands do i really need ?
//	case 0x73: //s -- level set
//		retVal = (pidConf[channel].setPoint << 8 | (pidConf[channel].autoTune << 4));
//	case 0x78: //x -- level set
//		retVal = pidConf[channel].setPoint;


  //TODO: add state_flags?
  
  // MSB first
  *data++ = (pidConf.setPoint >> 8);
  *data = pidConf.setPoint & 0xFF;

	return 2;
}


void HBWPidsValve::set(HBWDevice* device, uint8_t length, uint8_t const * const data)
{
  //dataLenght == 1 ? level = data[0] : level = ((data[0] << 8) | data[1]);
  pid->setPidsValve(data);
}
/*
 * set the desired Valve State in Manual Mode
 * level = 0 - 200 like a Blind or Dimmer
 */
void HBWPids::setPidsValve(uint8_t const * const data)
{

  if (*(data) == 0) {
    // toogle PID mode
    pidConf.inAuto = !pidConf.inAuto;
    setMode(pidConf.inAuto);
    pidConf.inAuto ? pidConf.outputMap = mymap(configValve->error_pos, 200.0, MAPSIZE) : pidConf.outputMap;
    pidConf.status = 0;
  }
  else if ((*(data) >= 0 && *(data) <= 200) && (pidConf.inAuto == MANUAL)) {  // right limits only if manual
    // map to PID's WindowSize
    pidConf.outputMap = mymap(*(data), 200.0, MAPSIZE);
    pidConf.Output = mymap(*(data), 200.0, config->windowSize * 1000);
  }
}


uint8_t HBWPidsValve::get(uint8_t* data)
{
  return pid->getPidsValve(data);
}
/*
 * returns the Level and some Status-infos of the Valve
 */
uint8_t HBWPids::getPidsValve(uint8_t* data)
{
	uint16_t retVal = 0;
  // uint8_t retVal = 0;
	uint8_t bits = 0;

	// map the Valve level, so it fits
	// into a standard FHEM slider (8bit)
	// and send some status bits (8bit)
	retVal = (uint16_t) mymap(pidConf.outputMap, MAPSIZE, 200.0);
  // retVal = mymap(pidConf.outputMap, MAPSIZE, 200.0);
	if (retVal % 2)	 retVal++;
	// state_flags
	bits = (pidConf.status << 2) | (pidConf.inAuto << 1) | pidConf.upDown;

//TODO: simplify code (value & statusbits...)

	//shift the 8bit value and the statusbits into 16bit
	retVal = (retVal << 8 | (bits << 4));

  // MSB first
  *data++ = (retVal >> 8);
  *data = retVal & 0xFF;
  // *data++ = retVal;
  // *data = (bits << 4);

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

//		pidConf[cnt].Input = temperature[cnt];
//pidConf.Input = 2000;
    
		// start the first time
		// can't sending everything at once to the bus.
		// so make a delay between channels
		if (pidConf.windowStartTime == 0)
		{
			pidConf.windowStartTime = now - ((channel + 1) * 2000);
      pidConf.lastPidTime = now - pidConf.sampleTime;
   
#ifdef DEBUG_OUTPUT
	hbwdebug(F("PID ch: ")); hbwdebug(channel);
	hbwdebug(F(" temp ")); hbwdebug(pidConf.Input);
	hbwdebug(F(" starting...\n"));
#endif
			return;
		}

		// error temp values comes back again
		if (pidConf.error) {
			if (pidConf.Input > ERROR_TEMP) {
				pidConf.error = 0;
				setMode(pidConf.oldInAuto);
			}
		}
		// check if we had a temperature and in automatic mode
		// in manual mode we can ignore the temp
		if (pidConf.inAuto == AUTOMATIC && (pidConf.Input == DEFAULT_TEMP || pidConf.Input == ERROR_TEMP)) {
			pidConf.error = 1;
			pidConf.oldInAuto = pidConf.inAuto;
			setErrorPosition();
		}
		// compute the PID Output
		if (computePid(channel))
		{
#ifdef DEBUG_OUTPUT
	hbwdebug(F("computePid ch: ")); hbwdebug(channel);
	hbwdebug(F(" returns new status: ")); hbwdebug(getPidsValveStatus()); hbwdebug(F("\n"));
#endif
//			state[cnt].valveStatus = getPidsValve(cnt, 0x78);
//			state[cnt].state = pidConf[cnt].status;
//			pidConf.counter >= 63 ? pidConf.counter = 0 : pidConf.counter++;  // TODO: needed where? key-press counter??
//			pidConf.lastSentTime = now;
		}
}


bool HBWPids::getPidsValveStatus()
{
  return pidConf.status;
}


/* standard public function - called by device main loop for every channel in sequential order */
void HBWPidsValve::loop(HBWDevice* device, uint8_t channel)
{
	uint32_t now = millis();

  if (pidValveConf.status != pid->getPidsValveStatus())  // check if pid status changed, turn ON or OFF
  {
    pidValveConf.status = pid->getPidsValveStatus();
    digitalWrite(pin, pidValveConf.status);  // turn ON or OFF
    
    // TODO: send key-event?
    hbwdebug(F("TODO:key-event ch: ")); hbwdebug(channel); pidValveConf.status ? hbwdebug(F(" long\n")) : hbwdebug(F(" short\n"));
  }

  if (pidValveConf.lastSentTime == 0 ) {
    pidValveConf.lastSentTime = now - ((channel + 1) * 3000);
    return;
  }
  

	if (config->send_max_interval && now - pidValveConf.lastSentTime >= (uint32_t) (config->send_max_interval) * 1000)
	{
    uint8_t level;
    get(&level);
  #ifdef DEBUG_OUTPUT
    hbwdebug(F("Valve ch: ")); hbwdebug(channel); hbwdebug(F(" send: ")); hbwdebug(level/2); hbwdebug(F("%\n"));
  #endif
    device->sendInfoMessage(channel, 2, &level);    // level has 2 byte here
    pidValveConf.lastSentTime = now;
	}
}


/*
 * on Error go to the error position stored in eeprom
 * and switch to manual
 */
void HBWPids::setErrorPosition()
{
	pidConf.Output = mymap(configValve->error_pos, 200.0, (uint32_t) config->windowSize * 1000);
	setMode(MANUAL);
#ifdef DEBUG_OUTPUT
  hbwdebug(F("setErrorPosition: ")); hbwdebug(pidConf.Output); hbwdebug(F("\n"));
#endif
}


int8_t HBWPids::computePid(uint8_t channel)
{
	uint32_t now = millis();
	uint8_t retVal = 0;
  
	// get Output from PID
	compute();
  
	//map Output from 0 to 65535
	uint16_t valveStatus = (uint16_t) mymap(pidConf.Output, config->windowSize *1000, MAPSIZE);

	// new window
	if (now - pidConf.windowStartTime > (uint32_t) config->windowSize * 1000) {
		// goes the Output up or down ?
		if (valveStatus != pidConf.outputMap) {
			valveStatus > pidConf.outputMap ? pidConf.upDown = 1 : pidConf.upDown = 0;
			retVal = 1;
		}
		pidConf.outputMap = valveStatus;
		pidConf.windowStartTime = now;
    ////pidConf.windowStartTime += config->windowSize * 1000;
#ifdef DEBUG_OUTPUT
  hbwdebug(F("computePid ch: ")); hbwdebug(channel);
  hbwdebug(F(" inAuto: ")); hbwdebug(pidConf.inAuto);
	hbwdebug(F(" windowSize: ")); hbwdebug(config->windowSize);
	hbwdebug(F(" output: ")); hbwdebug(pidConf.Output);
	hbwdebug(F(" outputMap: ")); hbwdebug(pidConf.outputMap);
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
	// TODO: check for % calculation based on windowSize?
	if ((unsigned long) pidConf.Output > now - pidConf.windowStartTime
      && ((unsigned long) pidConf.Output > 2000
			&& (unsigned long) pidConf.Output > (unsigned long) config->windowSize * 10)) {
		// ON
		if (pidConf.status == 0) {
			pidConf.status = 1;
			return 1;
		}
	}
	else if ((unsigned long) pidConf.Output < (unsigned long) (config->windowSize * 1000) - (unsigned long) (config->windowSize * 10)) {
		// OFF
		if (pidConf.status == 1) {
			pidConf.status = 0;
			return 1;
		}
	}
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
   
	unsigned long now = millis();
  
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
		pidConf.sampleTime = (unsigned long) NewSampleTime;
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


void HBWPids::setMode(bool Mode)
{
	bool newAuto = (Mode == AUTOMATIC);
	if (newAuto == !pidConf.inAuto) { /*we just went from manual to auto*/
		initialize();
	}
	pidConf.inAuto = newAuto;
}


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
long HBWPids::mymap(double x, double in_max, double out_max)
{
	return (x * out_max) / in_max;
}
