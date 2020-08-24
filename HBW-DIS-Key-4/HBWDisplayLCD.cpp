﻿/*
 * HBWDisplayLCD.cpp
 *
 * Created on: 16.07.2020
 * loetmeister.de
 * 
 */
 
#include "HBWDisplayLCD.h"
#include <EEPROM.h>


/* global */
boolean HBWDisplayDim::displayWakeUp = true;  // wakeup on start - if config->startup allows
byte HBWDisplayVChannel::numTotal = 0;
byte HBWDisplayLine::numTotal = 0;

/* contructors */
//HBWDisplayVChannel::HBWDisplayVChannel()
//{
//  num++;
//};

HBWDisplayVChNum::HBWDisplayVChNum(hbw_config_displayVChNum* _config)
{
  HBWDisplayVChannel::numTotal++;
  config = _config;
  currentValue = 0;
};


HBWDisplayVChBool::HBWDisplayVChBool(hbw_config_displayVChBool* _config)
{
  HBWDisplayVChannel::numTotal++;
  config = _config;
  currentValue = false;
  lastKeyNum = 0;
};


HBWDisplayDim::HBWDisplayDim(hbw_config_display_backlight* _config, uint8_t _backlight_pin, uint8_t _photoresistor_pin)
{
  config = _config;
  backlightPin = _backlight_pin;
  photoresistorPin = _photoresistor_pin;
  lastKeyNum = 0;
  initDone = false;
  currentValue = 0;
};


HBWDisplayLine::HBWDisplayLine(hbw_config_display_line* _config)
{
  config = _config;
  useDefault = true;
  lastKeyNum = 0;

  num = numTotal++;
};


HBWDisplay::HBWDisplay(LiquidCrystal* _display, HBWDisplayVChannel** _displayVChan, hbw_config_display* _config, HBWDisplayVChannel** _displayLines)
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
  if (config->num_characters == 7)  config->num_characters = 4; //4*4+4 = 20

/* temp */
//config->num_lines = 1; // +1 -> 2 lines
//config->num_characters = 2; // 2+1*4 -> 12
//config->refresh_rate = 4;
/* temp */
  
  if (!initDone) {
    // write custom characters to display
//    lcd->createChar(0, t_smiley);
    
    // set up the LCD's number of columns and rows:
    lcd->begin((config->num_characters *4) +4, config->num_lines +1);
    lcd->clear();
    // Print a message to the LCD.
    lcd->print(F("HBW-DISPLAY"));
    
//    lcd->write(byte(0));
    initDone = true;
  }
 #ifdef DEBUG_OUTPUT
  hbwdebug(F("display_cfg"));
  hbwdebug(F(" num_char: ")); hbwdebug((config->num_characters *4) +4);
  hbwdebug(F(" lines: ")); hbwdebug(config->num_lines +1);
  hbwdebug(F("\n"));
 #endif
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
    if (length == 8)  offset = 6;  // negative values might be send as QWORD
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
//  hbwdebug(F(" display_text: ")); hbwdebug(config->display_text);
//  hbwdebug(F(" sizeBoolVal: ")); hbwdebug(sizeof(t_displayVChVars{0}.b));
//  hbwdebug(F(" strlenBoolVal: ")); hbwdebug(strlen(displayVChBoolValue));
  hbwdebug(F("\n"));
  #endif
};

/* set special input value for a channel, via peering event. */
void HBWDisplayLine::set(HBWDevice* device, uint8_t length, uint8_t const * const data)
{
  HBWDisplayDim::displayWakeUp = true;
  if (length == 2)  // peering has 2 bytes
  {
    if (lastKeyNum == data[1])  // ignore repeated key press
      return;
    else
      lastKeyNum = data[1];
  }
  
  if (length > 2 )  // need to set min. 3 characters (should be ok, to set "foo" or even a single variable, like "%1%")
  {
    memset(line, 0, sizeof(line));
    memcpy(line, data, length < sizeof(line) ? length : sizeof(line));   // just copy what fits the buffer, discard the rest
    useDefault = false;   // use the data we just got, not the default
  }
  else {  // first byte should be a command (peering or FEHM)
    switch (data[0]) {
      case 0:
        useDefault = false;
        break;
      case 200:
        useDefault = true;
        break;
      // case 205: // toogle/activate auto cycle? (return default or set line, every other call of getStringFromValue())?
     #ifdef DISPLAY_CUSTOM_LINES_EESTART
      case 220:
      hbwdebug(F("save custom line")); hbwdebug(num);// hbwdebug(F(", numTotal: ")); hbwdebug(numTotal);
        if (strlen(line) > 0) {
//          hbwdebug(F(", eeAddr: ")); hbwdebug(DISPLAY_CUSTOM_LINES_EESTART + (MAX_DISPLAY_CHARACTER_PER_LINE * num));
          EEPROM.put(DISPLAY_CUSTOM_LINES_EESTART + (LINE_BUFF_LEN * num), line);
//          hbwdebug(F(", eePut line:")); hbwdebug(line);
        }
        hbwdebug(F("\n"));
      break;
     #endif
      case 255:
        useDefault = !useDefault;
      break;
    }
  }


 #ifdef DEBUG_OUTPUT
 char text[LINE_BUFF_LEN +1] = {0};
 hbwdebug(F("Line_set, len: ")); hbwdebug(length); hbwdebug(F(" default:"));hbwdebug(useDefault);hbwdebug(F(" "));
 if (length > 2 ) {
   memcpy(text, data, length < LINE_BUFF_LEN ? length : LINE_BUFF_LEN);
   hbwdebug(F("text: ")); hbwdebug(text);
 }
 hbwdebug(F("\n"));
 #endif
};

/* standard public function - set a channel, directly or via peering event. Data array contains new value or all peering details */
  // 0 - backlight off (0%)
  // 1...199 - 0.5% - 99.5% -> auto_brightness to 'off' when setting explicit value
  // 200 - backlight on (100%)
  // 202 - auto_brightness off
  // 204 - auto_brightness on
  // 255 - toggle backligh
void HBWDisplayDim::set(HBWDevice* device, uint8_t length, uint8_t const * const data)
{
  HBWDisplayDim::displayWakeUp = true;
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
  
  if (data[0] > 200)
  {
    switch (data[0]) {
      case 202:  // auto_brightness off
        config->auto_brightness = false;
       #ifdef DEBUG_OUTPUT
       hbwdebug(F("auto_brightness off"));
       #endif
        break;
      case 204:  // auto_brightness on
        config->auto_brightness = true;
       #ifdef DEBUG_OUTPUT
       hbwdebug(F("auto_brightness on"));
       #endif
        break;
      case 255:   // toggle
        currentValue = currentValue > 0 ? 0 : 200;
        config->auto_brightness = false;  // auto_brightness off
       #ifdef DEBUG_OUTPUT
       hbwdebug(F("toggle"));
       #endif
        break;
    }
  }
  else {
    currentValue = data[0];
    config->auto_brightness = false;  // auto_brightness off when setting value manually
  }
  
#ifdef DEBUG_OUTPUT
hbwdebug(F(", currentValue: ")); hbwdebug(currentValue);
hbwdebug(F("\n"));
#endif 
};


/* set special input value for a channel, via peering event. */
void HBWDisplay::set(HBWDevice* device, uint8_t length, uint8_t const * const data)
{
  //displayWakeUp = true;
// TODO: allow key peerig, to change entire screens?
// or use peering of one key with all lines, to get alle changed?
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
  *data = currentValue ? 200 : 0;

  return 1;
};


/* standard public function - returns length of data array. Data array contains current channel reading */
uint8_t HBWDisplayVChNum::get(uint8_t* data)
{
  // MSB first
  *data++ = (currentValue >> 8);
  *data = currentValue & 0xFF;

  return sizeof(data);
};


/* default function for HBWDisplayVChannel base class */
uint8_t HBWDisplayVChannel::getStringFromValue(char* _buffer) { return 0; };


/* read string from Progmem or RAM for each line of the display */
uint8_t HBWDisplayLine::getStringFromValue(char* _buffer)
{
  char cstr[LINE_BUFF_LEN];
  uint8_t defaultText = config->default_text;
  
  if (useDefault)
  {
    if (defaultText < sizeof(default_lines)/sizeof(*default_lines)) { // validate selection with available array elements
      strcpy_P(cstr, (char *)pgm_read_word(&(default_lines[defaultText])));   // load configured default text
    }
    else {
     #ifdef DISPLAY_CUSTOM_LINES_EESTART
      EEPROM.get(DISPLAY_CUSTOM_LINES_EESTART + (LINE_BUFF_LEN * num), cstr);
      if ((unsigned char)cstr[0] == 0xFF) {
        strcpy_P(cstr, (char *)pgm_read_word(&(default_line_out_of_range)));
      }
//      hbwdebug(F("eeGet line")); hbwdebug(num); hbwdebug(F(":")); hbwdebug(cstr);hbwdebug(F("\n"));
     #else
      strcpy_P(cstr, (char *)pgm_read_word(&(default_line_out_of_range)));
     #endif
    }
    strcpy(_buffer, cstr);
  }
  else {
    strcpy(_buffer, line);   // load text, that has been "set()"
  }

  return 0; // value not relevant for HBWDisplayLine
};


/* concatenate text string for bool channel */
uint8_t HBWDisplayVChBool::getStringFromValue(char* _buffer)
{
  if (config->display_text < sizeof(bool_text)/sizeof(*bool_text))  // validate selection with available array elements
  {
    char cstr[MAX_LENGTH_FOR_BOOL_TEXT +1];
    strcpy_P(cstr, (char *)pgm_read_word(&(bool_text[config->display_text][currentValue])));
    strcat(_buffer, cstr);
    
    return strlen(cstr);
  }
  return 0;
};


/* concatenate text string for numeric channel */
uint8_t HBWDisplayVChNum::getStringFromValue(char* _buffer)
{
  uint16_t factor = 1;
  
  for (uint8_t i = 0; i < config->factor; i++)
    factor *= 10;
  
  return formatFixpoint(_buffer, currentValue, config->digits, factor);
};


/*  format an int as fixed point value and create a sting/char array
 *  return length of this string
 *  (e.g. -1234 is retured as -12.34 string, using precicion 2 and factor 100)
 *  params: factor (1...1000)
 *          precision (0...3)
 */
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
    // remove the last digit, for precision < 3
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
  return strlen(cstr);
};


/* parse input string (_input_line) and replace %variable% with values
 * write directly to display
 */
void HBWDisplay::parseLine(char* _input_line, uint8_t length)
{
  char outBuff[MAX_DISPLAY_CHARACTER_PER_LINE +1] = {0};  // TODO: use actual display size? (config->num_characters +4*4)
  
  boolean isVar = false;
  unsigned char skipChar = 0;
  unsigned char varNum = 0;
  uint8_t outBuff_pos = 0;
  
  for (uint8_t i = 0; i< length; i++)
  {
    if (isVar && !skipChar)
    {
      if (_input_line[i] == '%')   // display '%%' as '%'
      {
        isVar = false;
        skipChar = 1;
      }
      else if (_input_line[i +1] == '%')   // variable %n%
      {
        varNum = _input_line[i] -'0';
        skipChar = 2;
  //hbwdebug(F("varNum: %")); hbwdebug(varNum); hbwdebug(F("% len_1\n"));
      }
      else if (_input_line[i +2] == '%')   // variable %nn%
      {
    // TODO: add option to use %t0...9% for predefined text/or symbols?
        varNum = _input_line[i] -'0';
        varNum *= 10;   // if second digit was found, the first one must be tens digit
        varNum += _input_line[i +1] -'0';
        skipChar = 3;
  //hbwdebug(F("varNum: %")); hbwdebug(varNum); hbwdebug(F("% len_2\n"));
      }
      else {
        isVar = false;
      }

      if (varNum > 0 && varNum <= HBWDisplayVChannel::getNumVChannel())   // check amount of virtual channels, not to call non-existing function
      {
        outBuff_pos += displayVChan[varNum -1]->getStringFromValue((char *)outBuff);
        varNum = 0;
      }
    }
    
    if (_input_line[i] == '%' && !isVar && !skipChar) {
      isVar = true;   // detected first '%'
    }

    if (!isVar) {
// hbwdebug(F("currentChar:")); hbwdebug(_input_line[i]);
// hbwdebug(F(" i:")); hbwdebug(i);
// hbwdebug(F(" shift:")); hbwdebug(shift);
// hbwdebug(F(" outBuff_pos:")); hbwdebug(outBuff_pos); hbwdebug(F("\n"));
      
      outBuff[outBuff_pos++] = _input_line[i];
      skipChar = 0;
      if (outBuff_pos > sizeof(outBuff) -1) break;  // stop if chars (per line) exeed buffer
    }
    else {
      if (skipChar > 0) {
        skipChar--;
        if (skipChar == 0)  isVar = false;
      }
    }
  }
  
 #ifdef DEBUG_OUTPUT
 hbwdebug(F("dis_outBuff#")); hbwdebug(outBuff); hbwdebug(F("#\n"));
 #endif
  
  lcd->print(outBuff);
  // TODO: use serial.print instead, to send data to other displays?
};


/* standard public function - called by device main loop for every channel in sequential order */
void HBWDisplay::loop(HBWDevice* device, uint8_t channel)
{
  static byte currentLine = 0;
  
  if (millis() - displayLastRefresh >= (uint32_t)(config->refresh_rate +1)*1000)
  {
    if (currentLine >= config->num_lines +1) {
      currentLine = 0;
      displayLastRefresh = millis();  // wait, after all lines have been updated
      return;
    }
    
    if (currentLine == 0) {
      lcd->clear();
    }
    
//    char lineBuffer[23];
//    for (byte l=0; l < NUMBER_OF_DISPLAY_LINES; l++) //(sizeof(**displayLine) / sizeof(displayLine[0]))..not working!
//    {
//      lcd->setCursor(0, l);
//      memset(lineBuffer, 0, sizeof(lineBuffer));
//      displayLine[l]->getStringFromValue((char*)lineBuffer);
//      parseLine(lineBuffer, sizeof(lineBuffer));
//    }

    //char lineBuffer[23] = {0};
    char lineBuffer[LINE_BUFF_LEN] = {0};
    // writing the display takes quite some time, so only update one line of the display each loop. Allowing to handle other stuff that is going on
    // TODO: check if this is still ok with 4 lines
    lcd->setCursor(0, currentLine);
    //memset(lineBuffer, 0, sizeof(lineBuffer));
    displayLine[currentLine]->getStringFromValue((char*)lineBuffer);  // copy (raw) string of current line to buffer
    parseLine(lineBuffer, sizeof(lineBuffer));    // parse (check for variables to expand) and send to display
    currentLine++;
  }
};


/* standard public function - called by device main loop for every channel in sequential order */
void HBWDisplayDim::loop(HBWDevice* device, uint8_t channel)
{
  if (!initDone) {
    if (config->startup)  currentValue = 200; // set startup value (200 = on, 0 = off)
    initDone = true;
  }

  // fixed intervall and not critical... so use millis() with modulo...
  if (millis() % LCD_BACKLIGHT_UPDATE_INTERVAL == 0)
  {
    if (displayWakeUp) {
      displayWakeUp = false;
      powerOnTime = millis();   // reset timer for "auto_off"
    }

    if (config->auto_brightness && photoresistorPin != NOT_A_PIN)
    {
      int16_t brightnessDiff = (analogRead(photoresistorPin) >> 2) - brightness;
      
      // smooth ADC reading, but allow larger steps if backlight is bright, or the measured brightness changed a lot
      if (brightnessDiff > 4) {
        brightness += 1 + (currentValue/32) + (brightnessDiff/50);  // if possible divider with power of 2, to use bitshift
      }
      else if (brightnessDiff < -4) {
        brightness -= 1 + (currentValue/32) + ((brightnessDiff * -1)/50);
      }
      
      currentValue = map(brightness, 0, 255, 0, 200);
    }
//hbwdebug(F("Dim reading:")); hbwdebug(reading);
//hbwdebug(F(" brightness:")); hbwdebug(brightness);
//hbwdebug(F(" currentValue:")); hbwdebug(currentValue);
//hbwdebug(F("\n"));

    if (millis() - powerOnTime >= (uint32_t)config->auto_off *60000 && config->auto_off)
      currentValue = 0;
    
    analogWrite(backlightPin, map(currentValue, 0, 200, 0, 255));
  }
};


// TODO: specialize template to keep separate .cpp and .h files.... if possible
//template class HBWDisplay<uint8_t>;
