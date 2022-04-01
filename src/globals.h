#ifndef GLOBALS_H
#define GLOBALS_H

// sensor values to expose to API
float humidity;
float fan_level = 0.0f;

// OneWire onewire(onewireData);  // declare instance of the OneWire class to communicate with onewire sensors
// probe beer(&onewire), fridge(&onewire);



// PID defaults
// double Setpoint = 3.3;
// double Output = 3.3;
// double Kp = 10.00;
// double Ki = 5E-4;
// double Kd = 500.0;
// double heatKp = 5.00;
// double heatKi = 0.25;
// double heatKd = 1.15;

// double Input, heatInput;
// double heatSetpoint, heatOutput;

// PID mainPID(&Input, &Output, &Setpoint, Kp, Ki, Kd, DIRECT);  // main PID instance for beer temp control (DIRECT: beer temperature ~ fridge(air) temperature)
// PID heatPID(&heatInput, &heatOutput, &heatSetpoint, heatKp, heatKi, heatKd, DIRECT);   // create instance of PID class for cascading HEAT control (HEATing is a DIRECT process)

#endif
