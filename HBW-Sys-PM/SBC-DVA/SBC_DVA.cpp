/*
    SBC-DVA.h - Library for SBC-DVA with the TI INA236 chip.
    https://joy-it.net/de/products/SBC-DVA
    Created by L0L0-tack, 24 October, 2023.
    Released for Joy-IT.
    Last modified, 24 October, 2023.
*/

#include <Arduino.h>
#include "SBC_DVA.h"

// Constructor //

SBCDVA::SBCDVA(uint8_t _address, TwoWire* _wire) {
  wire = _wire;
  Address = _address;
  Current_lsb = 0;
  Lsb = _LSB[0];
  // Mode = _MODE[7];
  // Vshct = _VSHCT[4];
  // Vbusct = _VBUSCT[4];
  // Avg = _AVG[0];
  Adcrange = _ADCRANGE[0];
}

// Private //

// Write the required 16-bit value into the register
void SBCDVA::_write_register(byte reg, int val) {
  wire->beginTransmission(Address);
  wire->write(reg);
  wire->write((val >> 8) & 0xFF);
  wire->write(val & 0xFF);
  wire->endTransmission();
  // if (wire->endTransmission() != 0)  // bus error
    // return BUS_ERROR;  // TODO: error handling? rely on read() error handling only?
}

// Read the 16-bit register value that will be returned
int SBCDVA::_read_register(byte reg) {
  wire->beginTransmission(Address);
  wire->write(reg);
  // wire->endTransmission();
  if (wire->endTransmission() != 0)  // bus error
    return BUS_READ_ERROR;
  // wire->requestFrom(Address, (uint8_t) 2);
  if (wire->requestFrom(Address, (uint8_t) 2) == 0)  // size 0
    return BUS_READ_ERROR;
  int value = (wire->read() << 8) | wire->read();
  return value;
}

// Public //

// Initialize the INA236 with the user specified parameters
void SBCDVA::init_ina236(int mode, int vshct, int vbusct, int avg, int adcrange, uint8_t _address) {
  if (_address != 0x0) Address = _address;  // use address set with constructor already, if not provided here
  _write_register(Config_Reg, RST);  // Reset the INA236 registers
  // Mode = _MODE[mode];
  // Vshct = _VSHCT[vshct];
  // Vbusct = _VBUSCT[vbusct];
  // Avg = _AVG[avg];
  Adcrange = _ADCRANGE[adcrange];
  Lsb = (Adcrange == ADCRANGE_1) ? _LSB[0] : _LSB[1];
  int config = 0x0000;
  // config |= Mode;
  // config |= Vshct;
  // config |= Vbusct;
  // config |= Avg;
  config |= _MODE[mode];
  config |= _VSHCT[vshct];
  config |= _VBUSCT[vbusct];
  config |= _AVG[avg];
  config |= Adcrange;
  _write_register(Config_Reg, config);
}

// Reset the INA236 registers
void SBCDVA::reset_ina236(byte _address) {
  Address = _address;
  _write_register(Config_Reg, RST);
}

// Calibrate the INA236 for the correct Shunt measurement
void SBCDVA::calibrate_ina236() {
  static const float current_lsb_min = 10 / pow(2, 15);
  Current_lsb = current_lsb_min + 0.0018;
  int SHUNT_CAL = 0.00512 / (Current_lsb * 0.008);
  SHUNT_CAL = static_cast<int>(SHUNT_CAL);
  if (Adcrange == ADCRANGE_2) SHUNT_CAL /= 4;
  _write_register(Calibration_Reg, SHUNT_CAL);
}

// Read the current across the Shunt resistor
float SBCDVA::read_current() {
  int CURRENT = _read_register(Current_Reg);
  return (Current_lsb * CURRENT);
}

// Read the power across the Shunt resistor
float SBCDVA::read_power() {
  int POWER = _read_register(Power_Reg);
  return fabs(32 * Current_lsb * POWER);
}

// Read the voltage across the Shunt resistor
float SBCDVA::read_shunt_voltage() {
  int value_raw = _read_register(Shunt_Volt_Reg);
  int value_comp = ~value_raw;
  int value = value_comp + 1;
  return fabs(value * Lsb);
}

// Read the bus voltage
float SBCDVA::read_bus_voltage() {
  int value = _read_register(Bus_Volt_Reg);
  return (value * _LSB[2]);
}

// Set the alert register
void SBCDVA::mask_enable(byte val) {
  if (val > sizeof(_MASK_ENABLE)) return;
  unsigned int out = _MASK_ENABLE[val];
  _write_register(Mask_Enable_Reg, out);
}

// Write a value into the alert register as reference
void SBCDVA::write_alert_limit(int val) {
  _write_register(Alert_Limit_Reg, val);
}

// print the value from the alert register
void SBCDVA::print_alert_limit(Stream* _serial) {
  if (_serial == NULL) return;
  int raw = _read_register(Alert_Limit_Reg);
  _serial->print(F("Mask/Alert register value: "));
  _serial->println(String(raw));
}

// print the Manufacturer ID
void SBCDVA::print_manufacturer_ID(Stream* _serial) {
  if (_serial == NULL) return;
  _serial->print(F("Manufacturer ID: "));
  if (_read_register(Manufacturer_ID_Reg) == 21577) {
    _serial->println(F("TI"));
  } else {
    _serial->println(F("nan"));
  }
}

// print the Device ID
void SBCDVA::print_device_ID(Stream* _serial) {
  if (_serial == NULL) return;
  _serial->print(F("Device ID: "));
  _serial->println(String(_read_register(Device_ID_Reg)));
}

// Read the Device ID
int SBCDVA::read_device_ID() {
  int value = _read_register(Device_ID_Reg);
  return (value == BUS_READ_ERROR) ? 0 : value;
}
