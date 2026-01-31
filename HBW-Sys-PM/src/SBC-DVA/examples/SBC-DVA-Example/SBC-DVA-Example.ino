#include <SBC_DVA.h>

SBCDVA INA236(0x40);

void setup() {
  Serial.begin(115200);
  Wire.begin();
  Wire.setClock(100000);
  delay(10);

  // Initialization of the INA236
  INA236.reset_ina236(0x40);
  INA236.init_ina236(0, 7, 4, 4, 0, 0);
  INA236.calibrate_ina236();
  INA236.mask_enable(1);
  INA236.write_alert_limit();
  INA236.print_alert_limit();
  INA236.print_manufacturer_ID();
  INA236.print_device_ID();
  delay(5000);  // Sleep for 5 seconds
  Serial.println("start");
}

void loop() {
  // Main loop
  Serial.print("Current [A]: ");
  Serial.print(INA236.read_current(), 4);
  Serial.print(", Power [W]: ");
  Serial.print(INA236.read_power(), 4);
  Serial.print(", Shunt [V]: ");
  Serial.print(INA236.read_shunt_voltage(), 4);
  Serial.print(", Bus [V]: ");
  Serial.println(INA236.read_bus_voltage(), 4);
  delay(1000);  // Sleep for 1 second
}