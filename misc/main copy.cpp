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
#include "Ticks.h"
#include "TempControl.h"
#include "pins.h"

// global class objects static and defined in class cpp and h files

void setup(void);
void loop (void);


void setup()
{
	
	// Signals are inverted on the shield, so set to high
	digitalWrite(coolingPin, HIGH);
	digitalWrite(heatingPin, HIGH);
	
	pinMode(coolingPin, OUTPUT);
	pinMode(heatingPin, OUTPUT);
		
	#if(USE_INTERNAL_PULL_UP_RESISTORS)
		pinMode(doorPin, INPUT_PULLUP);
	#else
		pinMode(doorPin, INPUT);
	#endif
	
	tempControl.loadSettingsAndConstants(); //read previous settings from EEPROM
	tempControl.init();
	tempControl.updatePID();
	tempControl.updateState();
	
}

void loop(void)
{
	static unsigned long lastUpdate = 0;
	if(ticks.millis() - lastUpdate > 1000){ //update settings every second
		lastUpdate=ticks.millis();
		
		tempControl.updateTemperatures();		
		tempControl.detectPeaks();
		tempControl.updatePID();
		tempControl.updateState();
		tempControl.updateOutputs();
		if(rotaryEncoder.pushed()){
			rotaryEncoder.resetPushed();
			menu.pickSettingToChange();	
		}
		display.printState();
		display.printAllTemperatures();
		display.printMode();
		display.lcd.updateBacklight();
	}	
	//listen for incoming serial connections while waiting top update
	piLink.receive();
}

// catch bad interrupts here when debugging
ISR(BADISR_vect){
	while(1){
		;
	}
}

