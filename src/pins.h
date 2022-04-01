/*
 * Copyright 2012 BrewPi/Elco Jacobs.
 *
 * This file is part of BrewPi.
 * 
 * BrewPi is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * BrewPi is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with BrewPi.  If not, see <http://www.gnu.org/licenses/>.
 */


#include <Arduino.h>

#ifndef PINS_H_
#define PINS_H_

// const byte onewireData = 2;  // one-wire data
// OneWire onewire(onewireData);  // declare instance of the OneWire class to communicate with onewire sensors
// probe beer(&onewire), fridge(&onewire);

// #define beerSensorPin    A5 // OneWire 1
// #define fridgeSensorPin  A4 // OneWire 2

#define onewirePin  2
#define coolingPin	6
#define heatingPin	5
#define dhtPin      7
#define fan1aPin    9
#define fan1bPin    10
#define fan2Pin     3
#define tachPin     4

// You can use the internal pull-up resistors instead of external ones for the doorPin and the rotary encoder pins
#define USE_INTERNAL_PULL_UP_RESISTORS false
// If you are using the light bulb of your fridge as a heater, set this define to true
// It will turn on the light when the door is opened
#define LIGHT_AS_HEATER false

#endif /* PINS_H_ */