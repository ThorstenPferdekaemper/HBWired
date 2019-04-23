/*
 * HMWPids.cpp
 *
 * Created on: 15.04.2019
 * loetmeister.de
 * 
 * Author: Harald Glaser
 * some parts from the Arduino PID Library - Version 1.1.1
 * by Brett Beauregard <br3ttb@gmail.com> brettbeauregard.com
 */
 
#include "HMWPids.h"


HBWPids::HBWPids(hbw_config_pid_valve* _configValve, hbw_config_pid* _config)
{
  config = _config;
  configValve = _configValve;
  
  pidConf.windowStartTime = 0;
//  pidConf.lastSentTime = 0;
//  pidConf.setPoint = 2500; //TODO a default value ?
  // outputMap is a maped Value of the computed Output in uint16_t
  pidConf.outputMap = mymap(configValve->error_pos, 200.0, MAPSIZE);
  pidConf.upDown = 0; // 0 up; 1 down
  pidConf.inAuto = config->startMode; // 1 automatic ; 0 manual
  pidConf.oldInAuto = 0; // we switch to MANUAL in error Position. Store the old value here
  pidConf.status = 0;
  //pidConf.counter = 0;
  //pid lib
  pidConf.Input = 0;  // TODO: init with ERROR or DEFAULT temp?
  pidConf.Output = 0;
  pidConf.ITerm = 0;
  pidConf.lastInput = 0;
  pidConf.kp = 0;
  pidConf.ki = 0;
  pidConf.kd = 0;
  pidConf.sampleTime = 2000; // pid cumpute every 2 sec
  pidConf.outMax = 0;
//  pidConf.lastPidTime = millis() - pidConf.sampleTime;
  pidConf.setPoint = 2200;
}


HBWPidsValve::HBWPidsValve(uint8_t _pin, HBWPids* _pid, hbw_config_pid_valve* _config)
{
  config = _config;
  pin = _pin;
  pid = _pid; // linked PID channel

  pidConf.lastSentTime = 0;
  //pidConf.counter = 0;  // key press counter?
//  pidConf.Output = 0;
//  pidConf.outputMap = mymap(config->error_pos, 200.0, MAPSIZE);
  //pidConf.status = LOW;
  digitalWrite(pin, LOW);
  pinMode(pin, OUTPUT);
}

/*
 * set Pids Parameter on Start and when changed in EEProm
 */
// channel specific settings or defaults
void HBWPids::afterReadConfig() {
//todo look if we need such big sizes for parameters
  if (config->kp == 0xFFFF)  config->kp = 1000;
  if (config->ki == 0xFFFF)  config->ki = 50; //todo do i need size 2 for autotune 0,5
  if (config->kd == 0xFFFF)  config->kd = 10; //dito 0,1
  if (config->windowSize == 0xFFFF)  config->windowSize = 600; // 10min

	setOutputLimits(config->windowSize * 1000);
	setTunings((float) config->kp, (float) config->ki / 100, (float) config->kd / 100);
	
	pidConf.inAuto = config->startMode; // 1 automatic ; 0 manual
	pidConf.inAuto ? setMode(AUTOMATIC) : setMode(MANUAL);
  
	pidConf.Output = mymap(configValve->error_pos, 200.0, config->windowSize * 1000);
	pidConf.outputMap = mymap(configValve->error_pos, 200.0, MAPSIZE);
}


// channel specific settings or defaults
void HBWPidsValve::afterReadConfig() {
  if (config->send_max_interval == 0xFFFF)  config->send_max_interval = 150;
  if (config->error_pos == 0xFF)  config->error_pos = 30;
  //TODO: Pids access PidsValve config, how to make sure PidsValve are initialized before Pids? -> move PidsValve afterReadConfig to Pids afterReadConfig?


  pidConf.status = !pid->getPidsValveStatus();  // TODO: start with off or force update by negating?
}

//uint8_t HBWPids::getCounter()
//{
//	return (pidConf.counter);
//}


void HBWPids::setInfo(HBWDevice* device, uint8_t length, uint8_t const * const data)
{
  if (length == 2)
  {
    pidConf.Input = ((data[0] << 8) | data[1]);
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
  hbwdebug(" setPidsTemp autotune ");
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

  //TODO: ?add state_flags to get()?
}


//getPidsTemp
uint8_t HBWPids::get(uint8_t* data)
{
// todo which commands do i really need ?
//	case 0x73: //s -- level set
//		retVal = (pidConf[channel].setPoint << 8 | (pidConf[channel].autoTune << 4));
//	case 0x78: //x -- level set
//		retVal = pidConf[channel].setPoint;
//	case 0x53: //S -- LEVEL_GET
//		retVal = pidConf[channel].setPoint;


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
    pidConf.inAuto ? setMode(AUTOMATIC) : setMode(MANUAL);
    pidConf.inAuto ? pidConf.outputMap = mymap(configValve->error_pos, 200.0, MAPSIZE) : pidConf.outputMap;
    pidConf.status = 0;
  }
  else if ((*(data) >= 0 && *(data) <= 200) && (pidConf.inAuto == MANUAL)) {  // right limits only if manual
    // map to PID's WindowSize
    pidConf.outputMap = mymap(*(data), 200.0, MAPSIZE);
    pidConf.Output = mymap(*(data), 200.0, config->windowSize * 1000);
  }
  
//	case 0x73:
//#if PIDDEBUG
//		hmwdebug("   setPidsValve manual level: "); hmwdebugln(level);
//#endif
//		if (level == 0) {
//			// toogle PID mode
//			pidConf[channel].inAuto = !pidConf[channel].inAuto;
//			pidConf[channel].inAuto ? setMode(channel, AUTOMATIC) : setMode(channel, MANUAL);
//			pidConf[channel].inAuto ?
//					pidConf[channel].outputMap = mymap(config->valves[channel].error_pos, 200.0, MAPSIZE) :
//					pidConf[channel].outputMap;
//			pidConf[channel].status = 0;
//		}
//	case 0x78:
//		// right limits only if manual
//		if ((level >= 0 && level <= 200) && (pidConf[channel].inAuto == MANUAL)) {
//#if PIDDEBUG
//			hmwdebug ("ok manualmode");
//#endif
//			// map to PID's WindowSize
//			pidConf[channel].outputMap = mymap(level, 200.0, MAPSIZE);
//			pidConf[channel].Output = mymap(level, 200.0, window_size * 1000);
//			//pidConf[channel].Output = pidConf[channel].outputMap * 1000;
//		}
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
	uint8_t bits = 0;

	// map the Valve level, so it fits
	// into a standard FHEM slider (8bit)
	// and send some status bits (8bit)
	retVal = (uint16_t) mymap(pidConf.outputMap, MAPSIZE, 200.0);
	if (retVal % 2)
		retVal++;
	bits = (pidConf.status << 2) | (pidConf.inAuto << 1) | pidConf.upDown;

//TODO: simplify code (value & statusbits...)

	//shift the 8bit value and the statusbits into 16bit
	retVal = (retVal << 8 | (bits << 4));

  // MSB first
  *data++ = (retVal >> 8);
  *data = retVal & 0xFF;

  return 2;
}


void HBWPids::autoTune()
{
	// TODO: autotune
}


//pidRetState* HBWPids::handlePids(int16_t *temperature)
void HBWPids::loop(HBWDevice* device, uint8_t channel)
{
	uint32_t now = millis();
	//static pidRetState state[HMW_CONFIG_NUM_PIDS]; //NULL or -1 do nothing; 0=off 1=on 2=send status

	if (now < 15000)
		return NULL; // wait 15 sec on start
	
//	if (check_max_interval(state))
//		return state; // send on interval

//		state[cnt].valveStatus = 0;
//		state[cnt].state = -1;
//		pidConf[cnt].Input = temperature[cnt];
    //int16_t temperature = 2000;
    //pidConf.Input = temperature;
    
		// start the first time
		// can't sending everything at once to the bus.
		// so make a delay between channels
		if (pidConf.windowStartTime == 0)
		{
			pidConf.windowStartTime = now - ((channel + 1) * 2000);
			//pidConf.lastSentTime = now - ((channel + 1) * 3000);
      pidConf.lastPidTime = now - pidConf.sampleTime;
   
#ifdef DEBUG_OUTPUT
	hbwdebug("PID ch: "); hbwdebug(channel);
	hbwdebug(" temp "); hbwdebug(pidConf.Input);
	hbwdebug(" starting...\n");
#endif
			return NULL;
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
	hbwdebug("computePid ch: "); hbwdebug(channel);
	hbwdebug(" returns new status: "); hbwdebug(pidConf.status); hbwdebug("\n");
#endif
//			state[cnt].valveStatus = getPidsValve(cnt, 0x78);
//			state[cnt].state = pidConf[cnt].status;
//			pidConf.counter >= 63 ? pidConf.counter = 0 : pidConf.counter++;  // TODO: needed where? key-press counter??
//			pidConf.lastSentTime = now;
		}

	//return state;
}

bool HBWPids::getPidsValveStatus() {
  return pidConf.status;
}

/*
 * check if we reach the send_max_interval time
 * and return the state values if so
 */
//uint8_t HBWPidsValve::check_max_interval(pidRetState *state_a)
void HBWPidsValve::loop(HBWDevice* device, uint8_t channel)
{
	uint32_t now = millis();

  if (pidConf.status != pid->getPidsValveStatus())  // check if pid status changed, turn ON or OFF
  {
    pidConf.status = pid->getPidsValveStatus();
    digitalWrite(pin, pidConf.status);  // turn ON or OFF
    // TODO: send key-event?
    hbwdebug("key-event ch: "); hbwdebug(channel); pidConf.status ? hbwdebug(" long\n") : hbwdebug(" short\n");
  }

  if (pidConf.lastSentTime == 0 ) {
    pidConf.lastSentTime = now - ((channel + 1) * 3000);
    return;
  }
  

	if (config->send_max_interval && now - pidConf.lastSentTime >= (uint32_t) (config->send_max_interval) * 1000)
	{
    uint8_t level;
    get(&level);
    device->sendInfoMessage(channel, 2, &level);    // level has 2 byte here
    pidConf.lastSentTime = now;
    
#ifdef DEBUG_OUTPUT
  hbwdebug(" Valve ch: "); hbwdebug(channel); hbwdebug(" send "); hbwdebug(level); hbwdebug("\n");
#endif
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
	if (now - pidConf.windowStartTime >= (uint32_t) config->windowSize * 1000) {
		// goes the Output up or down ?
		if (valveStatus != pidConf.outputMap) {
			valveStatus > pidConf.outputMap ? pidConf.upDown = 1 : pidConf.upDown = 0;
			retVal = 1;
		}
		pidConf.outputMap = valveStatus;
		pidConf.windowStartTime = now;
#ifdef DEBUG_OUTPUT
  hbwdebug("computePid ch: "); hbwdebug(channel);
	hbwdebug(" windowSize: "); hbwdebug(config->windowSize);
	hbwdebug(" outPut: "); hbwdebug(pidConf.Output);
	hbwdebug(" outputMap: "); hbwdebug(pidConf.outputMap);
	hbwdebug(" input: "); hbwdebug(pidConf.Input);
	hbwdebug(" setpoint: "); hbwdebug(pidConf.setPoint);
	hbwdebug("   outMax: ");hbwdebug(pidConf.outMax);
	hbwdebug(" Kp: ");hbwdebug(pidConf.kp);
	hbwdebug(" Ki: ");hbwdebug(pidConf.ki);
	hbwdebug(" Kd: ");hbwdebug(pidConf.kd);
	hbwdebug("\n");
#endif
	}
	// under 2 seconds or under 1% of windowsize we do nothing.
	// it makes no sense to send on's and off's over the bus in
	// such a short time
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

/*
 * compute the pid output
 */
void HBWPids::compute()
{
	if (!pidConf.inAuto)
		return;
   
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
		pidConf.Output = (pidConf.kp * error + pidConf.ITerm - pidConf.kd * dInput);
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
