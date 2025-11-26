# C++ Package for the ina236 High-precision digital I²C monitoring module for current (48 V, 8 A, 16 bit)

This library provides a C++ Package for the ina236 High-precision digital I²C monitoring module for current.
See **https://joy-it.net/products/SBC_DVA** for more details.

## Behaviour considered to be a pass
As long as the microcontroller or single board computer still finds the I2C addresses of the INA236 (0x40 is the I2C address, can be changed on the board).
If the INA236 still returns accurate Current and Voltage readings.

## Behaviour considered to be a fail
As soon as the I2C address can no longer be found or the Current and Voltage readings are no longer accurate.

# Basic functionality

## Initialize the INA236
The `init_ina236` function allows to set up the INA236 either with default values or with user defined values.
```python
# First parameter
# Change the mode of operation.
# Choose between 8 different modes of operation
# 1. Shutdown
# 2. Shunt triggered, single shot
# 3. Bus triggered, single shot
# 4. Shunt and Bus triggered, single shot
# 5. Shutdown
# 6. Continuous Shunt voltage
# 7. Continuous Bus voltage
# 8. Continous Shunt and Bus voltage
# Default initialization with 'Continous Shunt and Bus voltage'
# Second parameter
# Change Vshct of the INA236.
# Choose between 8 different conversion times for the shunt voltage measurment
# 0. 140us
# 1. 204us
# 2. 332us
# 3. 588us
# 4. 1100us
# 5. 2116us
# 6. 4156us
# 7. 8244us
# Default initialization with '1100us'
# Third parameter
# Change Vbusct of the INA236.
# Choose between 8 different conversion times for the shunt voltage measurment
# 0. 140us
# 1. 204us
# 2. 332us
# 3. 588us
# 4. 1100us
# 5. 2116us
# 6. 4156us
# 7. 8244us
# Default initialization with '1100us'
# Fourth parameter
# Change Avg of the INA236.
# Choose between 8 different values to define the amount that is to be averaged
# 0. 1
# 1. 4
# 2. 16
# 3. 64
# 4. 128
# 5. 256
# 6. 512
# 7. 1024
# Default initialization with '1'
# Fifth parameter
# Change ADCRange of the INA236.
# Choose between 2 different values that are to be used for the internal calculations
# 0. ±81.92mV
# 1. ±20.48mV
# Default initialization with '±81.92mV'

# Sixth parameter
# Change Address of the INA236.
# Choose between 4 different values which define the I2C Adress that is to be used for the communication
# 1. GND_0x40
# 2. VS_0x41
# 3. SDA_0x42
# 4. SCL_0x43
# Default initialization with 'GND_0x40'
# Initialize the INA236.
# Initialize the INA236 with either predefined values from the library or with the user defined values
INA236.init_ina236(0, 7, 4, 4, 0, 0);
```

## Reset the INA236
The `reset_ina236` function enables the INA236 to be reset independently of the initialization status and exclusively on the basis of the I2C address used.
```python
# Change the address in the function call to tell the reset function which I2C address corresponds to the INA236.
# Choose between 4 different values which define the I2C Adress that is to be used for the reset can be changed on the board itself.
# 1. GND_0x40
# 2. VS_0x41
# 3. SDA_0x42
# 4. SCL_0x43
# Default initialization with 'GND_0x40'
INA236.reset_ina236(0x40);
```

## Calibrate the INA236
The `calibrate_ina236` function calibrates the INA236 to obtain accurate measured values about the shunt resistance.
```python
# Calibrate the INA236 for the correct Shunt measurement.
INA236.calibrate_ina236();
```

## Read current measurment
The `read_current` function enables the current at the shunt resistor to be read.
```python
# Read the current current at the shunt resistor.
Serial.print(INA236.read_current(), 4);
```

## Read power measurment
The `read_power` function enables the power to be read at the shunt resistor.
```python
# Read the current power at the shunt resistor.
Serial.print(INA236.read_power(), 4);
```

## Read shunt voltage measurment
The `read_shunt_voltage` function enables the reading of the voltage at the shunt resistor.
```python
# Read the current voltage at the shunt resistor.
Serial.print(INA236.read_shunt_voltage(), 4);
```

## Read bus voltage measurment
The `read_bus_voltage` function enables the bus voltage to be read at the screw terminals.
```python
# Read the current bus voltage at the screw terminals.
Serial.println(INA236.read_bus_voltage(), 4);
```

## Configure the alert register
The `mask_enable` function allows to choose between 10 different options in the alert register.
```python
# Choose a option in the alert register of the INA236.
# Choose between 10 different options to define the part of the register that is to be used
# 0. SOL (Shunt Over-limit)
# 1. SUL (Shunt Under-limit)
# 2. BOL (Bus Over-limit)
# 3. BUL (Bus Under-limit)
# 4. POL (Power Over-limit)
# 5. CNVR (Conversion Ready)
# 6. MemError
# 7. AFF (Alert Function Flag)
# 8. CVRF (Conversion Ready Flag)
# 9. OVF (Math Over-flow)
# Default initialization with '1'
INA236.mask_enable(1);
```

## Write to the alert register
The `write_alert_limit` function enables writing to the alert register of the INA236.
```python
# Write to the alert register of the INA236.
# Depending on the register that is choosen in the 'mask_enable' function values can be writen into the register using this function.
# It is important to note how large the values may be and in what format they must be written.
# All values must be given as hex numbers or as two's complement hex numbers like the 'SOL (Shunt Over-limit)' option of the 'mask_enable' function.
# All other options of the function 'mask_enable' take normal hex numbers, but the numbers must be calculated as in the library or
# as in the datasheet otherwise the alert register cannot work properly with them.
# Default initialization with '0x294'
INA236.write_alert_limit();
```

## Read from the alert register
The `print_alert_limit` function enables reading from the alert register of the INA236 and printed to serial output.
```python
# Read from the alert register of the INA236.
INA236.print_alert_limit(&Serial);
```

## Read the manufacturer ID
The `print_manufacturer_ID` function enables the manufacturer ID of the INA236 to be read and printed to serial output.
```python
# Read from the manufacturer ID register of the INA236.
INA236.print_manufacturer_ID(&Serial);
```

## Read the device ID
The `print_device_ID` function enables the device ID of the INA236 to be read and printed to serial output.
```python
# Read from the device ID register of the INA236.
INA236.print_device_ID(&Serial);
```

## Supported targets

for Arduino

## License

MIT
