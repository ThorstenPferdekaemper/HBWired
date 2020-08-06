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
//#include <Wire.h> // TWI lib
#include <LiquidCrystal.h>


#define DEBUG_OUTPUT   // debug output on serial/USB

// defined here to use in functions // TODO: replace with proper/flexible solution
#define NUMBER_OF_V_CHAN 8    // total number of virtual channels (class: HBWDisplayVChNum, HBWDisplayVChBool)
#define NUMBER_OF_DISPLAY_LINES 2   // number of lines the display has (class: HBWDisplayLine)
#define CHARACTER_PER_LINE 12


static const byte LINE_BUFF_LEN = 14;  // TOOD: use consistent sizes.. what value? CHARACTER_PER_LINE + (CHARACTER_PER_LINE/4)

// pre-defined display lines. load with set command
// %2% or %02% calls default_lines[1] (n-1)
const char default_line1[] PROGMEM = {"Ofen: %1%" "\xDF" "C"};    //Ofen: 99.9°C
const char default_line2[] PROGMEM = {"S" "\xF0" ": %2%" "\xDF" "C %5%"};  //Sp: 99°C Ein (Status Ein/Aus für Ladepumpe)
const char default_line3[] PROGMEM = {"Au" "\xE2" "en %3%" "\xDF" "C"}; // Außen 99.9°C (symbole? | \x7E -99.9°C)
const char default_line4[] PROGMEM = {"Innen %4%" "\xDF" "C"};          // Innen 99.9°C (| \x7F 99.9°C)

const char * const default_lines[] PROGMEM = {
  default_line4, default_line3, default_line2, default_line1
};

const char default_line_out_of_range[] PROGMEM = {"not defined"};


// pre-defined text (or symbols/custom chars?), access via %t0% ... %t9% (%s1%)? // TODO: implement...
//const char text1[] PROGMEM = {"\343C"}; //°
//const char text2[] PROGMEM = {"Zur\365ck"};//\xF5
//const char text3[] PROGMEM = {"Ofen "};
//const char text4[] PROGMEM = {"Fenster "};

//const char * const default_text[] PROGMEM = {
//  text1, text2, text3, text4
//};

// text for bool channels
const char text_on[] PROGMEM = {"Ein"};
const char text_off[] PROGMEM = {"Aus"};
const char text_open[] PROGMEM = {"Auf"};
const char text_close[] PROGMEM = {"Zu"};
const char text_auto[] PROGMEM = {"Aut"};
const char text_manu[] PROGMEM = {"Man"};
//const char text_on_sym[] PROGMEM = {"\xDB"};
//const char text_off_sym[] PROGMEM = {"\xF8"};
const char text_on_sym[] PROGMEM = {"\xF3"};
const char text_off_sym[] PROGMEM = {"-"};

#define MAX_LENGTH_FOR_BOOL_TEXT 3

// this array matches the "display_text" XML configuration: bool_text[display_text][current bool value (0 || 1)]
PROGMEM const char * const bool_text[][4] = {
  {text_off, text_on},
  {text_close, text_open},
  {text_manu, text_auto},
  {text_off_sym, text_on_sym}
};


//const uint16_t factor_map[] PROGMEM = {1, 10, 100, 100}; //1000


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


// config of one Display Virtual Numeric (display number variable) channel, address step 1
struct hbw_config_displayVChNum {
  uint8_t factor:2;   // 1/1, 1/10, 1/100. 1/1000
  uint8_t digits:2;   // number of decimal places/digits (999, 999.9, 999.99, 99.999) ////, 99999)
  uint8_t :4;     //fillup
};

// config of one Display Virtual Bool (display bool variable) channel, address step 1
struct hbw_config_displayVChBool {
  uint8_t display_text:2;   // On/Off, Open/Closed, Symbol X/Y, Auto/Manu
  uint8_t :6;     //fillup
};

// config of one Display line channel, address step 1
struct hbw_config_display_line {
//  uint8_t num_characters;  // number of characters per line //TODO: needed? static? -> use to allocate correct "buffer" size?
  uint8_t no_auto_cycle:1;  // default 1=auto_cycle disabled, to rotate different lines //TODO: implement
  uint8_t default_text:2;  // or less pre-defined options, if one line can be set and saved at runtime?
  uint8_t :5;     //fillup
};

// config of one Display channel, address step 2?
struct hbw_config_display {
  uint8_t startup:1;  // initial state, 1=ON
  uint8_t auto_cycle:1;  // default 1=auto_cycle enabled, to rotate different screens //TODO: implement
  uint8_t n_invert_display:1;  // default 1=not inverted //TODO: implement
  uint8_t refresh_rate:4;  //??? dealy or rate? (1-15 seconds?) // TODO: needed? or just an option "auto_refresh"? //TODO: implement?
  uint8_t :1;     //fillup
//  uint8_t num_characters;  // number of characters per line //TODO: needed? static? -> use to allocate correct "buffer" size?
//  uint8_t num_lines;  // number of lines //TODO: needed? static? -> configure by number of display line channels?
//  uint8_t :2;     //fillup
  uint8_t dummy;
};
//typedef struct {
//  uint8_t startup:1;  // initial state, 1=ON
//  uint8_t auto_cycle:1;
//  uint8_t n_invert_display:1;  // default 1=not inverted
//  uint8_t refresh_delay:4;  //??? dealy or rate? (0-15 seconds?) // TODO: needed? or just an option "auto_refresh"?
//  uint8_t :2;     //fillup
//  uint8_t num_characters;  // number of characters per line //TODO: static? -> use to allocate correct "buffer" size?
//  uint8_t num_lines;  // number of characters per line //TODO: static?
//  uint8_t dummy;
//  char line1[21];
//  char line2[21];
//  char line3[21];
//  char line4[21];
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
  uint8_t auto_off:5;  // 1..31 minutes standby delay. 0 = always on ( 0 = "special value", not used) //TODO: implement
  uint8_t :1;     //fillup
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
    
//  private:
//    boolean newData // TODO: add a flag, to indicate new value has been received by set or setInfo (peering)? How to check? Must be tracked for every variable
//    static byte num;
//  protected:
//    HBWDisplayVChannel();
//
//  public:
//    static inline uint8_t getNum()
//    {
//      return num;
//    }
};

// Class HBWDisplayVChNum (input channel, to peer with external temperatur sensor or set any 16bit value - signed!)
//class HBWDisplayVChNum : public HBWChannel {
class HBWDisplayVChNum : public HBWDisplayVChannel {
  public:
    //HBWDisplayVChNum(hbw_config_displayVChNum* _config, int16_t* _displayVChNumValue);
    HBWDisplayVChNum(hbw_config_displayVChNum* _config);
    virtual uint8_t get(uint8_t* data);
    virtual void setInfo(HBWDevice*, uint8_t length, uint8_t const * const data);
    virtual void set(HBWDevice*, uint8_t length, uint8_t const * const data);
    
    uint8_t getStringFromValue(char* _buffer);  // format the value and write the string to "_buffer"
    
  private:
    hbw_config_displayVChNum* config;
    int16_t currentValue; // "raw" value
    //int16_t* displayVChNumValue;
    
    uint8_t formatFixpoint(char* _buf, int16_t intVal, uint8_t precision, uint16_t factor);
};

// Class HBWDisplayVChBool (input channel, to peer with external key channel or set bool value)
//class HBWDisplayVChBool : public HBWChannel {
class HBWDisplayVChBool : public HBWDisplayVChannel {
  public:
    //HBWDisplayVChBool(hbw_config_displayVChBool* _config, char* _displayVChBoolValue);
    //HBWDisplayVChBool(hbw_config_displayVChBool* _config, t_displayVChanValues _displayVChBoolValue);
    HBWDisplayVChBool(hbw_config_displayVChBool* _config);
    virtual uint8_t get(uint8_t* data);
    virtual void setInfo(HBWDevice*, uint8_t length, uint8_t const * const data);
    virtual void set(HBWDevice*, uint8_t length, uint8_t const * const data);
    
    uint8_t getStringFromValue(char* _buffer);  // write the string defined in the channel config to "_buffer"
    
  private:
    hbw_config_displayVChBool* config;
    //uint8_t* displayVChBoolValue;
    //char* displayVChBoolValue;    // point to char array (struct) accessible to the display loop()
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
    //char* ptr_line;
    char line[LINE_BUFF_LEN];   // value received by set command (like "foo: %1%"). Will be displayed when set or called by command value 201
};


// dimmer channel for display backlight (or overall brighness, e.g. for OLED?)
// Class HBWDisplayDim
class HBWDisplayDim : public HBWChannel {
  public:
    HBWDisplayDim(hbw_config_display_backlight* _config, uint8_t _backlight_pin, uint8_t _photoresistor_pin = NOT_A_PIN);
    virtual void loop(HBWDevice*, uint8_t channel);
    virtual uint8_t get(uint8_t* data);
    virtual void set(HBWDevice*, uint8_t length, uint8_t const * const data);

//    enum Actions
//    {
//       MEASURE,
//       CALC_AVG,
//       SET_OUTPUT
//    };
  
  private:
    hbw_config_display_backlight* config;
    uint8_t backlightPin;  // (PWM) pin for backlight
    uint8_t photoresistorPin;  // light resistor (mabye not used == NOT_A_PIN)
    uint8_t brightness;   // read ADC and save average here
    uint8_t currentValue;   // current dimmer level (0...200 -> 0...100%)
    //uint32_t lastActionTime;
    //uint8_t nextAction;
    uint8_t lastKeyNum;
    boolean initDone;
    
    static const uint32_t BRIGTNESS_MEASURE_INTERVAL = 1000;
    static const uint32_t SET_BACKLIGHT_INTERVAL = 3000;

    // TODO: add notify/logging? Not really needed, it would only help to update the state in FHEM (just use get state!!)
};

// Class HBWDisplay (master channel)
//TODO: Check if this can be used // template <uint8_t num_HBWDisplayVChannel>
class HBWDisplay : public HBWChannel {
  public:
    //HBWDisplay(LiquidCrystal* _display, t_displayVChVars* _displayVChVars, hbw_config_display* _config, uint8_t _backlight_pin = NOT_A_PIN);
    //HBWDisplay(LiquidCrystal* _display, pfunc_getStringFromValue _displayVChVars, hbw_config_display* _config);
    HBWDisplay(LiquidCrystal* _display, HBWDisplayVChannel** _displayVChan, hbw_config_display* _config, HBWDisplayVChannel** _displayLines);
    virtual void loop(HBWDevice*, uint8_t channel);
    //virtual uint8_t get(uint8_t* data);
    virtual void set(HBWDevice*, uint8_t length, uint8_t const * const data);
    virtual void afterReadConfig();
    
	// set 1-10 + data = set display lines (update display immediately?) - additional channel per line? allow peer to load line? -> own channel!
  // set 11-20 + data = set display lines and save to EEPROM? -> own channel!
	// set 0 - off?
	// set 200 (or anything !=0?) - display on?
  // set 255 "toogle" to switch to next screen
  // set 211...215 - load default text? -> own channel!
  // set 250 - write buffer to display (refresh)?
  
    
    static const uint32_t DISPLAY_REFRESH_INTERVAL = 3000; // 3 seconds // TODO: needed? useful? can virtual channels trigger an update?


  private:
    hbw_config_display* config;
    LiquidCrystal* lcd;
    //t_displayVChVars* displayVChVars;
    HBWDisplayVChannel** displayVChan;  // pointer to array of linked VChannels
    //pfunc_getStringFromValue displayVChan;
    uint32_t displayLastRefresh;    // last time of update
    boolean initDone;

//    char line1[15];  // TODO: make global and just define pointer here?
//    char line2[15];
    //t_displayLine* displayLines;
    HBWDisplayVChannel** displayLine;

    void parseLine(char* _line, uint8_t length);
    
};

// Class HBWDisplayLine (give access to one display line via dedicated channel)
//class HBWDisplayLine : public HBWChannel {
//  public:
//    HBWDisplayLine(displayBuf* _displayBuf, uint8_t _line_num, uint8_t _max_char);
//    // HBWDisplayLine(LiquidCrystal* _display, uint8_t _line_num, uint8_t _max_char);
//    // virtual void loop(HBWDevice*, uint8_t channel);
//    //virtual uint8_t get(uint8_t* data);
//    virtual void set(HBWDevice*, uint8_t length, uint8_t const * const data);
//    //virtual void afterReadConfig();
//
//  private:
//    
//
//};

#endif /* HBWDISPLAYLCD_H_ */
