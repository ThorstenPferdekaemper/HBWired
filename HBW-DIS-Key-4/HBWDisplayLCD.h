/*
 * HBWDisplayLCD.h
 *
 * Created on: 16.07.2020
 * loetmeister.de
 * No commercial use!
 *
 */
 
#ifndef HBWDISPLAYLCD_H_
#define HBWDISPLAYLCD_H_


#include "HBWired.h"
#include <LiquidCrystal.h>


#define DEBUG_OUTPUT   // debug output on serial/USB



#define MAX_DISPLAY_LINES 4   // number of lines the device will allow (class: HBWDisplayLine) - equal amount of channels must be created
#define MAX_DISPLAY_CHARACTER_PER_LINE 24 //28 possible (with current config..) limit to 20?

#define DISPLAY_CUSTOM_LINES_EESTART 0x320  // EEPROM start address (required space: MAX_DISPLAY_LINES * MAX_DISPLAY_CHARACTER_PER_LINE)

//static const uint8_t LCD_BACKLIGHT_MIN_DIM = 0;   // minimum dimming %


static const byte LINE_BUFF_LEN = MAX_DISPLAY_CHARACTER_PER_LINE + (MAX_DISPLAY_CHARACTER_PER_LINE /5);

/*****************************************************************************
 * Static text
*/
// pre-defined display lines. load with set command
// %2% or %02% calls default_lines[1] (n-1)
const char default_line1[] PROGMEM = {"Ofen: %1%" "\xDF" "C"};    //Ofen: 99.9°C
const char default_line2[] PROGMEM = {"S" "\xF0" ": %2%" "\xDF" "C %5%"};  //Sp: 99°C Ein (Status Ein/Aus für Ladepumpe)
const char default_line3[] PROGMEM = {"Au" "\xE2" "en %3%" "\xDF" "C"}; // Außen 99.9°C (symbole? | \x7E -99.9°C)
const char default_line4[] PROGMEM = {"Innen %4%" "\xDF" "C"};          // Innen 99.9°C (| \x7F 99.9°C)

const char * const default_lines[] PROGMEM = {
  default_line4, default_line3, default_line2, default_line1
};

const char undefined[] PROGMEM = {"undefined"};
const char * const default_line_out_of_range[] PROGMEM = {
  undefined
};

// pre-defined text (or symbols/custom chars?), access via %t0% ... %t9% (%s1%)? // TODO: implement...?
//const char text1[] PROGMEM = {""};
//const char text2[] PROGMEM = {"Zur\365ck"};//\xF5
//const char text3[] PROGMEM = {""};
//const char text4[] PROGMEM = {""};

//const char * const default_text[] PROGMEM = {
//  text1, text2, text3, text4
//};

// text for "bool_text[][]" array, used in bool channels
const char text_on[] PROGMEM = {"Ein"};
const char text_off[] PROGMEM = {"Aus"};
const char text_open[] PROGMEM = {"Auf"};
const char text_close[] PROGMEM = {"Zu"};
const char text_auto[] PROGMEM = {"Auto"};
const char text_manu[] PROGMEM = {"Manu"};
//const char text_on_sym[] PROGMEM = {"\xDB"};
//const char text_off_sym[] PROGMEM = {"\xF8"};
const char text_on_sym[] PROGMEM = {"\xF3"};
const char text_off_sym[] PROGMEM = {"-"};

// default, max. 4 characters will be displayed
#define MAX_LENGTH_FOR_BOOL_TEXT 4

// this array matches the "display_text" XML configuration: bool_text[display_text][current bool value (0 || 1)]
PROGMEM const char * const bool_text[][4] = {
  {text_off, text_on},
  {text_close, text_open},
  {text_manu, text_auto},
  {text_off_sym, text_on_sym}
};


//typedef struct {
//  const char *on;
//  const char *off;
//} t_text_on_off;
//
//const t_text_on_off bool_text[] PROGMEM = {
//  {text_on, text_off},
//  {text_open, text_close},
//  {text_auto, text_manu},
//  {text_on, text_off}
//};

//const char smiley[8] PROGMEM = {
//  B00000,
//  B10001,
//  B00000,
//  B00000,
//  B10001,
//  B01110,
//  B00000,
//};

/******************************************************************************/

// config of one Display Virtual Numeric (display number variable) channel, address step 1
struct hbw_config_displayVChNum {
  uint8_t factor:2;   // 1/1, 1/10, 1/100. 1/1000
  uint8_t digits:2;   // number of decimal places/digits (99999, 9999.9, 999.99, 99.999)
  uint8_t :4;     //fillup
};

// config of one Display Virtual Bool (display bool variable) channel, address step 1
struct hbw_config_displayVChBool {
  uint8_t display_text:2;   // On/Off, Open/Closed, Symbol X/Y, Auto/Manu
  uint8_t :6;     //fillup
};

// config of one Display line channel, address step 1
struct hbw_config_display_line {
  uint8_t no_auto_cycle:1;  // default 1=auto_cycle disabled, to rotate different lines //TODO: implement
  //uint8_t default_text:2;  // TODO: or less pre-defined options, if one line can be set and saved at runtime? use 3 bit? add custom line 1 - 4? (100 byte @eeprom)
  uint8_t default_text:3;  // pre-defined text (0-3) or custom line (>3)
  uint8_t :4;     //fillup
};

// config of one Display channel, address step 2
struct hbw_config_display {
  uint8_t startup:1;  // initial state, 1=ON
  uint8_t auto_cycle:1;  // default 1=auto_cycle enabled, to rotate different screens //TODO: implement
  uint8_t n_invert_display:1;  // default 1=not inverted //TODO: implement
  uint8_t refresh_rate:5;  // 1-32 seconds (default 32) (0-31 +1)
  //uint8_t :1;     //fillup
  uint8_t num_lines:2;  // number of lines (2bit 0-3 +1 --> 1-4)
  uint8_t num_characters:3;  // number of characters per line (0-6 *4 +4 --> 4-28)
  uint8_t :3;     //fillup
//  uint8_t dummy;
};
//typedef struct {
//  uint8_t startup:1;  // initial state, 1=ON
//  uint8_t auto_cycle:1;  // default 1=auto_cycle enabled, to rotate different screens //TODO: implement
//  uint8_t n_invert_display:1;  // default 1=not inverted //TODO: implement
//  uint8_t refresh_rate:5;  // 1-32 seconds (default 32) (0-31 +1)
//  //uint8_t :1;     //fillup
//  uint8_t num_lines:2;  // number of lines (2bit 0-3 +1 --> 1-4)
//  uint8_t num_characters:3;  // number of characters per line (0-6 +1 *4 --> 4-28)
//  uint8_t :3;     //fillup
//} t_hbw_config_display;


//// use this to create an array, matching the ammount of display lines
//// it should be referenced by a global pointer to be accessed by the main display class as well as the individual lines
//typedef struct {
//  char line[LINE_BUFF_LEN];  // TODO: define correct size (NUM_CHARS + x?)
//} t_displayLine;


// config of Display backlight (dimmer) channel, address step 2
struct hbw_config_display_backlight {
  uint8_t startup:1;  // initial state, 1=ON
  uint8_t auto_brightness:1;  // default 1=auto_brightness enabled
  uint8_t auto_off:4;  // 1..15 minutes standby delay. 0 = always on ( 0 = "special value", NOT_USED)
  uint8_t :2;     //fillup
  uint8_t dummy;
};


//typedef struct {
//  uint8_t b[2];
//  int16_t t[2];
//} t_displayVChVars;
//struct t_displayVChVars {
//  char b[4];  // bool representation. Contains string like 'On' or 'Off'
//  //char n[8];  // numeric (int)
//  int n;
//};

//TODO: not working... fix or remove.
// create a function pointer, that will point to getStringFromValue() in bool or numValue virtual channels
////typedef boolean (*pfunc_getStringFromValue)(char* _buffer);
//typedef void (*pfunc_getStringFromValue)(char* _buffer);
//
//typedef struct s_displayVChanValues {
//  pfunc_getStringFromValue getStrFromValBool;
//  pfunc_getStringFromValue getStrFromValNum;
//} t_displayVChanValues;


// new base class to declare common function 'getStringFromValue()'
class HBWDisplayVChannel : public HBWChannel {
  public:
//    virtual uint8_t get(uint8_t* data);
//    virtual void setInfo(HBWDevice*, uint8_t length, uint8_t const * const data);
//    virtual void set(HBWDevice*, uint8_t length, uint8_t const * const data);
    virtual uint8_t getStringFromValue(char* _buffer);
    
  private:
    static byte numTotal;    // count total number of virtual input channels (class: HBWDisplayVChNum, HBWDisplayVChBool)

  public:
    static inline void addNumVChannel()
    {
      numTotal++;
    }
    static inline uint8_t getNumVChannel()
    {
      return numTotal;
    }
};

// Class HBWDisplayVChNum (input channel, to peer with external temperatur sensor or set any 16bit value - signed!)
class HBWDisplayVChNum : public HBWDisplayVChannel {
  public:
    HBWDisplayVChNum(hbw_config_displayVChNum* _config);
    virtual uint8_t get(uint8_t* data);
    virtual void setInfo(HBWDevice*, uint8_t length, uint8_t const * const data);
    virtual void set(HBWDevice*, uint8_t length, uint8_t const * const data);
    
    uint8_t getStringFromValue(char* _buffer);  // format the value and write the string to "_buffer"
    
  private:
    hbw_config_displayVChNum* config;
    int16_t currentValue; // "raw" value
    
    uint8_t formatFixpoint(char* _buf, int16_t intVal, uint8_t precision, uint16_t factor);
};

// Class HBWDisplayVChBool (input channel, to peer with external key channel or set bool value)
class HBWDisplayVChBool : public HBWDisplayVChannel {
  public:
    HBWDisplayVChBool(hbw_config_displayVChBool* _config);
    virtual uint8_t get(uint8_t* data);
    virtual void setInfo(HBWDevice*, uint8_t length, uint8_t const * const data);
    virtual void set(HBWDevice*, uint8_t length, uint8_t const * const data);
    
    uint8_t getStringFromValue(char* _buffer);  // write the string defined in the channel config to "_buffer"
    
  private:
    hbw_config_displayVChBool* config;
    boolean currentValue;   // store current value
    uint8_t lastKeyNum;

    // TODO: add notify/logging? Not really needed, it would only help to update the state in FHEM (just use get state!!)
};


// Class HBWDisplayLine, represents line buffer
//class HBWDisplayLine : public HBWChannel {
class HBWDisplayLine : public HBWDisplayVChannel {
  public:
    HBWDisplayLine(hbw_config_display_line* _config);//, char* _displayLine);
    //virtual void loop(HBWDevice*, uint8_t channel);
    //virtual uint8_t get(uint8_t* data);
    virtual void set(HBWDevice*, uint8_t length, uint8_t const * const data);
    
    virtual uint8_t getStringFromValue(char* _buffer);  // write current line or default text to "_buffer"

  private:
    hbw_config_display_line* config;
    boolean useDefault;
    char line[LINE_BUFF_LEN];   // value received by set command (like "foo: %1%"). Will be displayed when set or called by command value 0
    uint8_t lastKeyNum;

    static byte numTotal;
    byte num;
    
  public:
    static inline uint8_t getNumVChannel()
    {
      return numTotal;
    }
};


// dimmer channel for display backlight (or overall brighness, e.g. for OLED?)
// Class HBWDisplayDim
class HBWDisplayDim : public HBWChannel {
  public:
    HBWDisplayDim(hbw_config_display_backlight* _config, uint8_t _backlight_pin, uint8_t _photoresistor_pin = NOT_A_PIN);
    virtual void loop(HBWDevice*, uint8_t channel);
    virtual uint8_t get(uint8_t* data);
    virtual void set(HBWDevice*, uint8_t length, uint8_t const * const data);

    static boolean displayWakeUp;   // allow other channels to "wake up" the backlight
  
  private:
    hbw_config_display_backlight* config;
    uint8_t backlightPin;  // (PWM!) pin for backlight
    uint8_t photoresistorPin;  // light resistor (mabye not used == NOT_A_PIN) - pin must be ADC!
    uint8_t brightness;   // read ADC and save average here
    uint8_t currentValue;   // current dimmer level (0...200 -> 0...100%)
    uint32_t powerOnTime;
    uint8_t lastKeyNum;
    boolean initDone;
    
    static const uint32_t LCD_BACKLIGHT_UPDATE_INTERVAL = 1660;

    // TODO: add notify/logging? Not really needed, should not be enabled in auto_brightness mode anyway
};

// Class HBWDisplay (master channel)
//TODO: Check if this can be used // template <uint8_t num_HBWDisplayVChannel>
class HBWDisplay : public HBWChannel {
  public:
    HBWDisplay(LiquidCrystal* _display, HBWDisplayVChannel** _displayVChan, hbw_config_display* _config, HBWDisplayVChannel** _displayLines);
    virtual void loop(HBWDevice*, uint8_t channel);
    //virtual uint8_t get(uint8_t* data);
    virtual void set(HBWDevice*, uint8_t length, uint8_t const * const data);
    virtual void afterReadConfig();
    
	// set 0 - off?
	// set 200 (or anything !=0?) - display on? / wakeup?
  // set 255 "toogle" to switch to next screen
  

  private:
    hbw_config_display* config;
    LiquidCrystal* lcd;
    HBWDisplayVChannel** displayVChan;  // pointer to array of linked VChannels (input values)
    HBWDisplayVChannel** displayLine;  // pointer to array of linked VChannels (lines of the display)
    uint32_t displayLastRefresh;    // last time of update
    boolean initDone;
    
    void parseLine(char* _line, uint8_t length);
};

#endif /* HBWDISPLAYLCD_H_ */
