/*
    SBC-DVA.h - Library for SBC-DVA with the TI INA236 chip.
    https://joy-it.net/de/products/SBC-DVA
    Created by L0L0-tack, 24 October, 2023.
    Released for Joy-IT.
    Last modified, 24 October, 2023.
*/

#ifndef sbcdva
#define sbcdva

static const int READING_INVALID = -1024;

// Address variables ina236A
const byte GND_A = 0x40;
const byte VS_A = 0x41;
const byte SDA_A = 0x42;
const byte SCL_A = 0x43;

// Address variables ina236B
const byte GND_B = 0x48;
const byte VS_B = 0x49;
const byte SDA_B = 0x4A;
const byte SCL_B = 0x4B;

// Register variables
const byte Config_Reg = 0x00;
const byte Shunt_Volt_Reg = 0x01;
const byte Bus_Volt_Reg = 0x02;
const byte Power_Reg = 0x03;
const byte Current_Reg = 0x04;
const byte Calibration_Reg = 0x05;
const byte Mask_Enable_Reg = 0x06;
const byte Alert_Limit_Reg = 0x07;
const byte Manufacturer_ID_Reg = 0x3E;
const byte Device_ID_Reg = 0x3F;

// Reset
const int RST = 0x8000;

// Range of the ADC
const int ADCRANGE_1 = 0x0000;
const int ADCRANGE_2 = 0x1000;
 
// ADC values to be averaged
const int AVG_1 = 0x0000;
const int AVG_2 = 0x0200;
const int AVG_3 = 0x0400;
const int AVG_4 = 0x0600;
const int AVG_5 = 0x0800;
const int AVG_6 = 0x0A00;
const int AVG_7 = 0x0C00;
const int AVG_8 = 0x0E00;

// Conversion time for the VBUS measurement
const int VBUSCT_1 = 0x0000;
const int VBUSCT_2 = 0x0040;
const int VBUSCT_3 = 0x0080;
const int VBUSCT_4 = 0x00C0;
const int VBUSCT_5 = 0x0100;
const int VBUSCT_6 = 0x0140;
const int VBUSCT_7 = 0x0180;
const int VBUSCT_8 = 0x01C0;

// Conversion time for the SHUNT measurement
const int VSHCT_1 = 0x0000;
const int VSHCT_2 = 0x0008;
const int VSHCT_3 = 0x0010;
const int VSHCT_4 = 0x0018;
const int VSHCT_5 = 0x0020;
const int VSHCT_6 = 0x0028;
const int VSHCT_7 = 0x0030;
const int VSHCT_8 = 0x0038;

// Operating Mode
const int MODE_1 = 0x0000;
const int MODE_2 = 0x0001;
const int MODE_3 = 0x0002;
const int MODE_4 = 0x0003;
const int MODE_5 = 0x0004;
const int MODE_6 = 0x0005;
const int MODE_7 = 0x0006;
const int MODE_8 = 0x0007;

#include <Arduino.h>
#include <Wire.h>
#include <math.h>


class SBCDVA {
public:
  SBCDVA(uint8_t _address, TwoWire* _wire = &Wire);
  void init_ina236(int mode, int vshct, int vbusct, int avg, int adcrange, uint8_t _address = 0x0);
  void reset_ina236(uint8_t _address);
  void calibrate_ina236();
  float read_current();
  float read_power();
  float read_shunt_voltage();
  float read_bus_voltage();
  int read_device_ID();
  void mask_enable(byte val = 1);
  void write_alert_limit(int val = 0x294);
  void print_alert_limit(Stream* serial = &Serial);
  void print_manufacturer_ID(Stream* serial = &Serial);
  void print_device_ID(Stream* serial = &Serial);
  float Current_lsb;
  float Lsb;
  uint8_t Address;
  // int Mode;
  // int Vshct;
  // int Vbusct;
  // int Avg;
  int Adcrange;
  
private:
  void _write_register(byte reg, int val);
  int _read_register(byte reg);
  TwoWire* wire;
  static constexpr int _ADDRESS_A[4] = { GND_A, VS_A, SDA_A, SCL_A };
  static constexpr int _ADDRESS_B[4] = { GND_B, VS_B, SDA_B, SCL_B };
  static constexpr int _ADCRANGE[2] = { ADCRANGE_1, ADCRANGE_2 };
  static constexpr float _LSB[3] = { 0.0000025, 0.000000625, 0.0016 };
  static constexpr int _AVG[8] = { AVG_1, AVG_2, AVG_3, AVG_4, AVG_5, AVG_6, AVG_7, AVG_8 };
  static constexpr int _VBUSCT[8] = { VBUSCT_1, VBUSCT_2, VBUSCT_3, VBUSCT_4, VBUSCT_5, VBUSCT_6, VBUSCT_7, VBUSCT_8 };
  static constexpr int _VSHCT[8] = { VSHCT_1, VSHCT_2, VSHCT_3, VSHCT_4, VSHCT_5, VSHCT_6, VSHCT_7, VSHCT_8 };
  static constexpr int _MODE[8] = { MODE_1, MODE_2, MODE_3, MODE_4, MODE_5, MODE_6, MODE_7, MODE_8 };
  static constexpr unsigned int _MASK_ENABLE[10] = { 0x8000, 0x4000, 0x2000, 0x1000, 0x0800, 0x0400, 0x0020, 0x0010, 0x0008, 0x0004 };
  
  static const int BUS_READ_ERROR = 0;//0x8000;
};

#endif