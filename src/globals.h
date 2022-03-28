#ifndef GLOBALS_H
#define GLOBALS_H

const byte onewireData = 2;  // one-wire data
const byte relay1 = 6;       // relay 1 (fridge compressor)
const byte relay2 = 5;       // relay 2 (heating element)
const byte dhtpin = 7;
// fan params
const byte fan1a = 9;
const byte fan1b = 10;
const byte fan2 = 3;
const byte tach = 4;
float fan_level = 0.0f;

OneWire onewire(onewireData);  // declare instance of the OneWire class to communicate with onewire sensors
probe beer(&onewire), fridge(&onewire);

// sensor values to expose to API
double bt, ft;
float humidity;
bool relay1_status, relay2_status;
bool temp_reached = false;
const double epsilon = 1.0;

// PID defaults
double Setpoint = 3.3;
double Output = 3.3;
double Kp = 10.00;
double Ki = 5E-4;
double Kd = 500.0;
double heatKp = 5.00;
double heatKi = 0.25;
double heatKd = 1.15;

double Input, heatInput;
double heatSetpoint, heatOutput;

PID mainPID(&Input, &Output, &Setpoint, Kp, Ki, Kd, DIRECT);  // main PID instance for beer temp control (DIRECT: beer temperature ~ fridge(air) temperature)
PID heatPID(&heatInput, &heatOutput, &heatSetpoint, heatKp, heatKi, heatKd, DIRECT);   // create instance of PID class for cascading HEAT control (HEATing is a DIRECT process)

#endif
