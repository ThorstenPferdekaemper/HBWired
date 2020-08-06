/*
 * HBWDisplayLCD.cpp
 *
 * Created on: 16.07.2020
 * loetmeister.de
 * 
 */
 
#include "HBWDisplayLCD.h"


/* contructors */

//byte HBWDisplayVChannel::num = 0;
//HBWDisplayVChannel::HBWDisplayVChannel()
//{
//  num++;
//};

HBWDisplayVChNum::HBWDisplayVChNum(hbw_config_displayVChNum* _config)//, int16_t* _displayVChNumValue)
{
  config = _config;
  //displayVChNumValue = _displayVChNumValue;
  //*displayVChNumValue = 0;
  currentValue = 0;
};


//HBWDisplayVChBool::HBWDisplayVChBool(hbw_config_displayVChBool* _config, uint8_t* _displayVChBoolValue)
//HBWDisplayVChBool::HBWDisplayVChBool(hbw_config_displayVChBool* _config, char* _displayVChBoolValue)
HBWDisplayVChBool::HBWDisplayVChBool(hbw_config_displayVChBool* _config)//, t_displayVChanValues _displayVChBoolValue)
//HBWDisplayVChBool::HBWDisplayVChBool(hbw_config_displayVChBool* _config, pfunc_getStringFromValue _displayVChBool)
{
  config = _config;
//  displayVChBoolValue = _displayVChBoolValue;
  ////dispVChanVars[0].getStrFromVal = HBWDisplayVChBool::getStringFromValue;
  /////_displayVChBool = &HBWDisplayVChBool::getStringFromValue;
//  *displayVChBoolValue = {0};
  //memset(displayVChBoolValue, 0, sizeof(t_displayVChVars{0}.b));  // clear string from old value
  currentValue = false;
  lastKeyNum = 0;
};


HBWDisplayDim::HBWDisplayDim(hbw_config_display_backlight* _config, uint8_t _backlight_pin, uint8_t _photoresistor_pin)
{
  config = _config;
  backlightPin = _backlight_pin;
  photoresistorPin = _photoresistor_pin;

//  nextAction = MEASURE;
  //lastActionTime = BRIGTNESS_MEASURE_INTERVAL; // delay startup
  lastKeyNum = 0;
  initDone = false;
  //currentValue = 200;   // 0...200 -> 0...100%
};


HBWDisplayLine::HBWDisplayLine(hbw_config_display_line* _config)//, char* _displayLine)
{
  config = _config;
  //ptr_line = _displayLine;
  useDefault = true;
};


//HBWDisplay::HBWDisplay(LiquidCrystal* _display, t_displayVChVars* _displayVChVars, hbw_config_display* _config)
///HBWDisplay::HBWDisplay(LiquidCrystal* _display, HBWDisplayVChannel** _displayVChan, hbw_config_display* _config, t_displayLine* _displayLines)
HBWDisplay::HBWDisplay(LiquidCrystal* _display, HBWDisplayVChannel** _displayVChan, hbw_config_display* _config, HBWDisplayVChannel** _displayLines)
//HBWDisplay::HBWDisplay(LiquidCrystal* _display, pfunc_getStringFromValue _displayVChVars, hbw_config_display* _config)
{
  config = _config;
  lcd = _display;
  displayVChan = _displayVChan;
  displayLine = _displayLines;
  
  displayLastRefresh = 0;
  initDone = false;
};

/* class functions */

// channel specific settings or defaults
void HBWDisplay::afterReadConfig()
{
  if (!initDone) {
    // set up the LCD's number of columns and rows:
    lcd->begin(12, NUMBER_OF_DISPLAY_LINES);
    lcd->clear();
    // Print a message to the LCD.
    lcd->print(F("HBW-DISPLAY"));
    initDone = true;
  }
// #ifdef DEBUG_OUTPUT
//  hbwdebug(F("display_cfg"));
//  hbwdebug(F(" size displayVChan: ")); hbwdebug(sizeof(displayVChan));
//  hbwdebug(F(" num displayVChan: ")); hbwdebug(sizeof(**displayVChan) / sizeof(displayVChan[0])); // does not work....
//  hbwdebug(F("\n"));
// #endif
};


/* set special input value for a channel, via peering event. */
void HBWDisplayVChNum::setInfo(HBWDevice* device, uint8_t length, uint8_t const * const data)
{
  set(device, length, data);
};

/* standard public function - set a channel, directly or via peering event. Data array contains new value or all peering details */
void HBWDisplayVChNum::set(HBWDevice* device, uint8_t length, uint8_t const * const data)
{
  /* just store the new value. The display loop can fetch it, when needed */
  
  if (length == 1) {
    currentValue = (int16_t)data[0];
  }
  //else if == 2 (WORD)
  //else if == 8 (QWORD)
  else {
    uint8_t offset = 0;
    //*displayVChNumValue = (int16_t)((data[0] << 8) | data[1]);  // handle 2 byte values, skip the rest
    if (length == 8)  offset = 6;  // negative values might be send as QWORD
      //*displayVChNumValue = (int16_t)((data[6] << 8) | data[7]);  // negative values might be send as QWORD
    currentValue = (int16_t)((data[0 + offset]) << 8 | data[1 + offset]);  // handle 2 byte values, skip the rest
  }

 #ifdef DEBUG_OUTPUT
 hbwdebug(F("setVChNum: ")); hbwdebug(currentValue); hbwdebug(F(" len: ")); hbwdebug(length);
 hbwdebug(F("\n"));
 #endif
};


/* set special input value for a channel, via peering event. */
void HBWDisplayVChBool::setInfo(HBWDevice* device, uint8_t length, uint8_t const * const data)
{
  set(device, length, data);
};

/* standard public function - set a channel, directly or via peering event. Data array contains new value or all peering details */
void HBWDisplayVChBool::set(HBWDevice* device, uint8_t length, uint8_t const * const data)
{
  /* just store the new value. The display loop can fetch it, when needed */
  //*displayVChBoolValue = data[0];

  if (length == 2)
  {
    if (lastKeyNum == data[1])  // ignore repeated key press
      return;
    else
      lastKeyNum = data[1];
  }
    
  if (data[0] > 200)    // toggle
    currentValue = !currentValue;
  else
    currentValue = data[0] > 0 ? true : false;

  #ifdef DEBUG_OUTPUT
  hbwdebug(F("setVChBool: ")); hbwdebug(data[0]);
  hbwdebug(F(" display_text: ")); hbwdebug(config->display_text);
//  hbwdebug(F(" sizeBoolVal: ")); hbwdebug(sizeof(t_displayVChVars{0}.b));
//  hbwdebug(F(" strlenBoolVal: ")); hbwdebug(strlen(displayVChBoolValue));
  hbwdebug(F("\n"));
  #endif
};

/* set special input value for a channel, via peering event. */
void HBWDisplayLine::set(HBWDevice* device, uint8_t length, uint8_t const * const data)
{
  if (length > 1 )
  {
    memset(line, 0, sizeof(line));
    memcpy(line, data, length < sizeof(line) ? length : sizeof(line));   // just copy what fits the buffer, discard the rest
    useDefault = false;   // use the data we got, not the default
  }
//TODO: check how to handle repeated key press in peering (length == 2 & lastKeyNum = data[1])
  else if (length == 1) {  // just one byte. This should be a command
    switch (data[0]) {
      case 204:
        useDefault = true;
        break;
      case 202:
        useDefault = false;
        break;
      case 255:
        useDefault = !useDefault;
      break;
      // case 205: // toogle/activate auto cycle? (return default or set line, every other call of getStringFromValue())
    }
  }
  

 #ifdef DEBUG_OUTPUT
 char text[LINE_BUFF_LEN +1] = {0};
 hbwdebug(F("Line_set, len: ")); hbwdebug(length); hbwdebug(F(" "));
////  for(uint8_t i=0; i<length; i++) {  // only read what fits the buffer
////    text[i] = data[i];
////   //hbwdebug(data[i]); hbwdebug(F("-"));
////  }
 if (length > 1 ) {
   memcpy(text, data, length < LINE_BUFF_LEN ? length : LINE_BUFF_LEN);
   hbwdebug(F("text: ")); hbwdebug(text);
 }
 hbwdebug(F("\n"));
//
// // echo to console
// parseLine(text, length);
 #endif

  //memcpy(ptr_line, data, length < LINE_BUFF_LEN ? length : LINE_BUFF_LEN);
////  for(uint8_t i=1; i<length; i++) {  // only read what fits the buffer
////    line1[i] = data[i];
////  }

};

/* standard public function - set a channel, directly or via peering event. Data array contains new value or all peering details */
void HBWDisplayDim::set(HBWDevice* device, uint8_t length, uint8_t const * const data)
{
 #ifdef DEBUG_OUTPUT
 hbwdebug(F("setDim: "));
 #endif

  if (length == 2)
  {
    if (lastKeyNum == data[1])  // ignore repeated key press
      return;
    else
      lastKeyNum = data[1];
  }
  
  // set 202 - auto_brightness off? -> auto_brightness to 'off' when setting explicit value?
  // set 203 - toggle auto_brightness?
  // set 204 - auto_brightness on?
  // set 0 - backlight off
  // set 255 - toggle backligh
  // set 200 - backlight on

  if (data[0] > 200)
  {
    switch (data[0]) {
      case 202:  // auto_brightness off
        config->auto_brightness = false;
       #ifdef DEBUG_OUTPUT
       hbwdebug(F("auto_brightness off, "));
       #endif
        break;
      case 204:  // auto_brightness on
        config->auto_brightness = true;
       #ifdef DEBUG_OUTPUT
       hbwdebug(F("auto_brightness on, "));
       #endif
        break;
      case 255:   // toggle
        currentValue = currentValue > 0 ? 0 : 200;
        if (currentValue) config->auto_brightness = true;  // auto_brightness on
       #ifdef DEBUG_OUTPUT
       hbwdebug(F("toggle, "));
       #endif
        break;
    }
  }
  else {
    currentValue = data[0];
    config->auto_brightness = false;  // auto_brightness off
  }
  
#ifdef DEBUG_OUTPUT
hbwdebug(currentValue);
hbwdebug(F("\n"));
#endif 
};
  

/* standard public function - returns length of data array. Data array contains current channel reading */
uint8_t HBWDisplayDim::get(uint8_t* data)
{
  *data = currentValue;

  return 1;
};


/* standard public function - returns length of data array. Data array contains current channel reading */
uint8_t HBWDisplayVChBool::get(uint8_t* data)
{
  //*data = currentValue;
  *data = currentValue ? 200 : 0;

  return 1;
};


/* standard public function - returns length of data array. Data array contains current channel reading */
uint8_t HBWDisplayVChNum::get(uint8_t* data)
{
  // MSB first
//  *data++ = (*displayVChNumValue >> 8);
//  *data = *displayVChNumValue & 0xFF;

  *data++ = (currentValue >> 8);
  *data = currentValue & 0xFF;

  return sizeof(data);
};


// default function for HBWDisplayVChannel base class
uint8_t HBWDisplayVChannel::getStringFromValue(char* _buffer) { return 0; };


// read string from Progmem or RAM for each line of the display
uint8_t HBWDisplayLine::getStringFromValue(char* _buffer)
{
hbwdebug(F("getLineVal, def_lines:")); hbwdebug(sizeof(default_lines)/sizeof(*default_lines)); hbwdebug(F("\n"));

  char cstr[LINE_BUFF_LEN];
  uint8_t defaultText = config->default_text;
  
  if (useDefault)
  {
    if (defaultText < sizeof(default_lines)/sizeof(*default_lines)) { // validate selection with available array elements
      strcpy_P(cstr, (char *)pgm_read_word(&(default_lines[defaultText])));
    }
    else {
      strcpy_P(cstr, (char *)pgm_read_word(&(default_line_out_of_range)));
    }
    strcpy(_buffer, cstr);
  }
  else {
    strcpy(_buffer, line);
  }

  return 0; // value not relevant for HBWDisplayLine
};


// concatenate text string for bool channel
uint8_t HBWDisplayVChBool::getStringFromValue(char* _buffer)
{
  if (config->display_text < sizeof(bool_text)/sizeof(*bool_text))  // validate selection with available array elements
  {
    char cstr[MAX_LENGTH_FOR_BOOL_TEXT +1];
    strcpy_P(cstr, (char *)pgm_read_word(&(bool_text[config->display_text][currentValue])));
    strcat(_buffer, cstr);
    ///strcat_P(_buffer, (char *)pgm_read_word(&(bool_text[displayType][currentValue])));

//   hbwdebug(F("getBoolStr len:")); hbwdebug(strlen(cstr)); hbwdebug(F("\n"));
    
// currentValue = !currentValue; // only for testing
 
    return strlen(cstr);
  }
  
  return 0;
};


// concatenate text string for numeric channel
uint8_t HBWDisplayVChNum::getStringFromValue(char* _buffer)
{
  uint16_t factor = 1;
  for (uint8_t i = 0; i < config->factor; i++)  factor *= 10;
  
//  switch (config->factor) {
//    case 3:
//        factor = 1000;
//      break;
//    case 2:
//    factor = 100;
//      break;
//    case 1:
//    factor = 10;
//      break;
//  }
  //static const uint16_t factor_map[] = {1, 10, 100, 1000};
  //formatFixpoint(_buffer, currentValue, config->digits, factor_map[config->factor]);
  //formatFixpoint(_buffer, currentValue, config->digits, pgm_read_word(factor_map[config->factor]));

//currentValue -= 283; // only for testing
  
  return formatFixpoint(_buffer, currentValue, config->digits, factor);
  
  //return true;  // TODO: check if the value has changed since last call? only return true if it did?
};


// format an int as fixed point value and create a sting/char array
// return length of this string
// (e.g. -1234 is retured as -12.34 string, using precicion 2 and factor 100)
// params: factor (1...1000)
//         precision (0...3)
uint8_t HBWDisplayVChNum::formatFixpoint(char* _buf, int16_t intVal, uint8_t precision, uint16_t factor) {
 #ifdef DEBUG_OUTPUT
 hbwdebug(F("formatFixp, Val:")); hbwdebug(intVal);
 hbwdebug(F(" factor:")); hbwdebug(factor);
 hbwdebug(F(" precision:")); hbwdebug(precision);
 #endif
  
  unsigned int lval, rval;
  char sign[2] = "";
  char cstr[9];   // need to fit 8 characters + end of string '/0'
  
  if (intVal < 0) {
    sign[0] = '-';
    intVal *= -1;
  }
  
  lval = intVal / factor;
  rval = intVal - (lval * factor);

  if (precision == 0) {
    //sprintf_P(cstr, PSTR("%s%u"), sign, lval);
    sprintf(cstr, "%s%u", sign, lval);
  }
  else {
    // drop the last digit
    if (precision == 1 && rval > 9)  rval /= 10;
    else if (precision == 2 && rval > 99)  rval /= 10;
    
    char formatstr[10]="%s%u.%01u";
    formatstr[7]='0'+precision;
    
    sprintf(cstr, formatstr, sign, lval, rval);
//    sprintf(cstr, "%s%u.%.*u", sign, lval, precision, rval);  // seem to be not supported (precision as argument, with .*)
  }

 #ifdef DEBUG_OUTPUT
// hbwdebug(F(" str_len:")); hbwdebug(strlen(cstr));
 hbwdebug(F(" str:")); hbwdebug(cstr);
 hbwdebug(F("\n"));
 #endif

  strcat(_buf, cstr);
  return strlen(cstr);  // TODO: make void function, if len not needed?
}


/* set special input value for a channel, via peering event. */
void HBWDisplay::set(HBWDevice* device, uint8_t length, uint8_t const * const data)
{
//  static const byte BUFF_LEN = 13;  // TOOD: use consistent sizes.. what value?

//  char text[BUFF_LEN +1] = {0};
//  
// #ifdef DEBUG_OUTPUT
// hbwdebug(F("set, len: ")); hbwdebug(length); hbwdebug(F(" "));
////  for(uint8_t i=0; i<length; i++) {  // only read what fits the buffer
////    text[i] = data[i];
////   //hbwdebug(data[i]); hbwdebug(F("-"));
////  }
//  memcpy(text, data, length < BUFF_LEN ? length : BUFF_LEN);
// hbwdebug(F("text: ")); hbwdebug(text);
// hbwdebug(F("\n"));
//
// // echo to console
// parseLine(text, length);
// #endif
};


// parse input string (_input_line) and replace %variable% with values
// write directly to display
void HBWDisplay::parseLine(char* _input_line, uint8_t length)
{
  static const byte BUFF_LEN = 25;
  char outBuff[BUFF_LEN +1] = {0};
  
  boolean isVar = false;
  unsigned char skipChar = 0;
  unsigned char varNum = 0;
  uint8_t outBuff_pos = 0;
  
  for (uint8_t i = 0; i< length; i++) {
//  for (uint8_t i = 0; i< strlen_P(text1); i++) {
//    currentChar = pgm_read_byte_near(text1 +i);
    //currentChar = _input_line[i]; /// TODO: just use _input_line[i] !! remove currentChar
    
    //if (isVar && _input_line[i] != '%') {
    if (isVar && !skipChar)
    {
      if (_input_line[i] == '%') {  // display '%%' as '%'
        isVar = false;
        skipChar = 1;
      }
      else if (_input_line[i +1] == '%') {  // variable %n%
        varNum = _input_line[i] -'0';
        skipChar = 2;
  //hbwdebug(F("varNum: %")); hbwdebug(varNum); hbwdebug(F("% len_1\n"));
      }
      else if (_input_line[i +2] == '%') {  // variable %nn%
      // TODO add option to use %t0...9% for pre-defined text/or symbols?
        varNum = _input_line[i] -'0';
        varNum *= 10;   // if second digit was found, the first one must be tens digit
        varNum += _input_line[i +1] -'0';
        skipChar = 3;
  //hbwdebug(F("varNum: %")); hbwdebug(varNum); hbwdebug(F("% len_2\n"));
      }
      else {
        isVar = false;
      }

      if (varNum > 0 && varNum <= NUMBER_OF_V_CHAN )//(sizeof(**displayVChan) / sizeof(displayVChan[0]))) // TODO: validate if this works!!
      {
        outBuff_pos += displayVChan[varNum -1]->getStringFromValue((char *)outBuff);
        varNum = 0;
      }
    }
    
    if (_input_line[i] == '%' && !isVar && !skipChar) {
      isVar = true;   // detected first '%'
    //if (_input_line[i] == '%') {
//      if (isVar) {  //display '%%' as '%'
//        isVar = false;
//        varNum = 0;
//        ///shift -= 1;
//        skipChar = 1;
//      }
//      else {
//        isVar = true;
//      }
    }

    //if (!isVar && !skipChar) {
    if (!isVar) {
// hbwdebug(F("currentChar:")); hbwdebug(_input_line[i]);
// hbwdebug(F(" i:")); hbwdebug(i);
// hbwdebug(F(" shift:")); hbwdebug(shift);
// hbwdebug(F(" outBuff_pos:")); hbwdebug(outBuff_pos); hbwdebug(F("\n"));
      
      //outBuff[i+shift] = _input_line[i];  // use? lcd->print(_input_line[i+shift]);
      outBuff[outBuff_pos++] = _input_line[i];  // use? lcd->print(_input_line[i+shift]);
      skipChar = 0;
      if (outBuff_pos > BUFF_LEN) break;  //TODO: stop if chars (per line) exeeded (or BUFF_LEN)
    }
    else {
      if (skipChar > 0){
        skipChar--;
        if (skipChar == 0) {
          isVar = false;
        }
      }
    }
  }
  
 #ifdef DEBUG_OUTPUT
 hbwdebug(F("dis_outBuff#")); hbwdebug(outBuff); hbwdebug(F("#\n"));
 #endif
  
  lcd->print(outBuff);
};


/* standard public function - called by device main loop for every channel in sequential order */
void HBWDisplay::loop(HBWDevice* device, uint8_t channel)
{
  //if (!initDone) return;
  
  if (millis() - displayLastRefresh > DISPLAY_REFRESH_INTERVAL)
  {
    displayLastRefresh = millis();
    lcd->clear();
    //lcd->setCursor(0, 0);

    
//    char lineBuffer[23] = {0};
//    displayLine[0]->getStringFromValue((char*)lineBuffer);
    // TODO: sync with lcd->setCursor(0, 0 || 1);    
    char lineBuffer[23];
    for (byte l=0; l < NUMBER_OF_DISPLAY_LINES; l++) //(sizeof(**displayLine) / sizeof(displayLine[0]))..not working!
    {
      lcd->setCursor(0, l);
      memset(lineBuffer, 0, sizeof(lineBuffer));
      displayLine[l]->getStringFromValue((char*)lineBuffer);
      parseLine(lineBuffer, sizeof(lineBuffer));
    }
    
//    strcpy_P(lineBuffer, (char *)pgm_read_word(&(default_lines[0])));
//    parseLine(lineBuffer, sizeof(lineBuffer));
//strcpy_P(displayLines[0].line, (char *)pgm_read_word(&(default_lines[0])));
//  parseLine(displayLines[0].line, sizeof(displayLines[0].line)); // sizeof or strlen?  
//  lcd->setCursor(0, 1);
//    memset(lineBuffer, 0, sizeof(lineBuffer));
//    strcpy_P(lineBuffer, (char *)pgm_read_word(&(default_lines[1])));
//    parseLine(lineBuffer, sizeof(lineBuffer));
    //lcd->print(lineBuffer);
    
    
//  lcd->setCursor(0, 1);
  // print the number of seconds since reset:
//  lcd->print(displayLastRefresh / 1000);
  
//  lcd->print(" t1:");
//  lcd->print(displayVChVars[0].n);
  
  //displayVChVars->t[0]-=1;
//  lcd->print(displayVChVars[1].b);


  #ifdef DEBUG_OUTPUT
//  hbwdebug(F("update LCD")); 

//  memset(lineBuffer, 0, sizeof(lineBuffer));
//  byte b=255;
//  displayVChan[3]->set(device, 1, &b);
//  displayVChan[3]->getStringFromValue((char *)lineBuffer);
//  hbwdebug(F(" b1:")); hbwdebug(lineBuffer);
  
//  hbwdebug(F(" t1:")); hbwdebug(displayVChVars[0].n); hbwdebug(F(" t2:")); hbwdebug(displayVChVars[1].n);
//  hbwdebug(F(" b1:")); hbwdebug(displayVChVars[0].b); hbwdebug(F(" b2:")); hbwdebug(displayVChVars[1].b);
//  hbwdebug(F("\n"));
  
  #endif
  }
};

/* standard public function - called by device main loop for every channel in sequential order */
void HBWDisplayDim::loop(HBWDevice* device, uint8_t channel)
{
  if (!initDone) {
    currentValue = config->startup ? 200 : 0; // set startup value (200 = on, 0 = off)
    initDone = true;
  }
    // switch (nextAction) {
    //  case MEASURE
    //if (millis() - lastActionTime >= BRIGTNESS_MEASURE_INTERVAL) {
    //  case SET_OUTPUT?
    //if (millis() - lastActionTime >= SET_INTERVAL) {
    //  case CALC_AVG?

    // not critical... so use millis() with modulo...
  if (millis() % BRIGTNESS_MEASURE_INTERVAL == 0)
  {
    brightness = (brightness + (uint8_t) (analogRead(photoresistorPin) >> 2)) /2;  // drop last two bits (10bit ADC)
  }
  
  if (millis() % SET_BACKLIGHT_INTERVAL == 0)
  {
    //lastActionTime = millis();

    
    if (config->auto_brightness && photoresistorPin != NOT_A_PIN) {
      //brightness = (uint8_t) (analogRead(photoresistorPin) >> 2);  // drop last two bits (10bit ADC)
      currentValue = map(brightness, 0, 255, 0, 200);
      // TODO: calculate AVG
      // TODO: use logaritmic value?
    }
//currentValue -=10;
//hbwdebug(F("Dim cVal:")); hbwdebug(currentValue);hbwdebug(F("\n"));
    analogWrite(backlightPin, map(currentValue, 0, 200, 0, 255));
  }
};


// TODO: specialize template to keep separate .cpp and .h files.... if possible
//template class HBWDisplay<uint8_t>;
