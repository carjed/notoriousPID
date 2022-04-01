// Notorious PID Fermentation Temperature Control v 0.9
#include <Arduino.h>
#include <avr/wdt.h>
#include <OneWire.h>
#include <ArduinoJson.h>
#include <DHT.h>
#include "PID_v1.h"
#include "probe.h"
// #include "fridge.h"
#include "globals.h"
#include "functions.h"
#include "Ticks.h"
#include "TempControl.h"
#include "temperatureFormats.h"
#include "pins.h"
#define DEBUG false  // debug flag for including debugging code
#define DHTTYPE DHT11
#define NUMBER_VARIABLES 20
#define OUTPUT_BUFFER_SIZE 5000
// #define _SS_MAX_TX_BUFF 256 // RX buffer size
// #define SERIAL_TX_BUFFER_SIZE 256

DHT dht(dhtPin, DHTTYPE);

void mainUpdate();  // update sensors, PID output, fridge state, write to log, run profiles
int setManual(String command);

// call all update subroutines
// start conversion for all sensors
// update sensors when conversion complete
void mainUpdate() {

  // update temp control
  tempControl.updateTemperatures();		
  tempControl.detectPeaks();
  tempControl.updatePID();
  tempControl.updateState();
  tempControl.updateOutputs();

  bool cooling_status = !digitalRead(coolingPin);
  bool heating_status = !digitalRead(heatingPin);

  // update humidity reading
  humidity = dht.readHumidity();

  // format JSON payload
  StaticJsonDocument<500> doc2;
  
  doc2["setpoint"] = tempControl.getBeerSetting();
  doc2["fanlevel"] = fan_level;
  doc2["probetemp"] = tempControl.getBeerTemp();
  doc2["chambertemp"] = tempControl.getFridgeTemp();
  doc2["humidity"] = humidity;
  doc2["cooling"] = cooling_status;
  doc2["heating"] = heating_status;
  char tempString[12];
  doc2["output"] = fixedPointToString(tempString, tempControl.cs.coolEstimator, 3, 12);
  doc2["heatoutput"] = fixedPointToString(tempString, tempControl.cs.heatEstimator, 3, 12);
  doc2["kp"] = fixedPointToString(tempString, tempControl.cc.Kp, 3, 12);
  doc2["ki"] = fixedPointToString(tempString, tempControl.cc.Ki, 3, 12);
  doc2["kd"] = fixedPointToString(tempString, tempControl.cc.Kd, 3, 12);

  if( Serial.available()) {
    serializeJson(doc2, Serial);
    // Serial.println();
  }

  if(cooling_status == false && heating_status == false){
    setPWM1A(0.0f);
  } else {
    setPWM1A(fan_level);
  }
}

int setManual(String command){
  // parse string in the format /set?param0=abc&param1=def
  String param = getValue(command, '=', 0);
  // strip '/set?' prefix from param value
  param.remove(0,5);

  String value = getValue(command, '=', 1);
  int str_len = value.length() + 1; 
  char val_chr[str_len];
  value.toCharArray(val_chr, str_len);
  
  if (param == "setpoint"){
    tempControl.cs.beerSetting = stringToTemp(val_chr);
  }
  else if (param == "fanlevel"){
    fan_level = value.toFloat();
    setPWM1A(fan_level);
  }
  else if (param == "kp"){
    tempControl.cc.Kp = stringToFixedPoint(val_chr);
  }
  else if (param == "ki"){
    tempControl.cc.Ki = stringToFixedPoint(val_chr);
  }
  else if (param == "kd"){
    tempControl.cc.Kd = stringToFixedPoint(val_chr);
  }
  
  // mainPID.SetTunings(Kp, Ki, Kd);
  // heatPID.SetTunings(heatKp, heatKi, heatKd); 

  return 1;
}

void setup() {
  // set up fans
  //enable outputs for Timer 1
  pinMode(fan1aPin, OUTPUT);
  pinMode(fan1bPin, OUTPUT);
  setupTimer1();
  //enable outputs for Timer 2
  pinMode(fan2Pin, OUTPUT); //2
  setupTimer2();
  //note that pin 11 will be unavailable for output in this mode!
  setPWM1A(fan_level); //set duty to 50% on pin 9
  // digitalWrite(fan1a, LOW);
  // digitalWrite(fan1b, LOW);

  // configure relay pins and write default HIGH (relay open)
  pinMode(coolingPin, OUTPUT);
    digitalWrite(coolingPin, HIGH);
  pinMode(heatingPin, OUTPUT);
    digitalWrite(heatingPin, HIGH);

	tempControl.loadSettingsAndConstants(); //read previous settings from EEPROM
	tempControl.init();
	tempControl.updatePID();
	tempControl.updateState();

  dht.begin();  

  //Serial.begin(9600);
  Serial.begin(115200);

  humidity = dht.readHumidity();
  // relays use 0 for on, 1 for off, so we invert for the api exposure
  bool cooling_status = !digitalRead(coolingPin);
  bool heating_status = !digitalRead(heatingPin);

  StaticJsonDocument<500> doc2;
  doc2["setpoint"] = tempControl.getBeerSetting();
  doc2["fanlevel"] = fan_level;
  doc2["probetemp"] = tempControl.getBeerTemp();
  doc2["chambertemp"] = tempControl.getFridgeTemp();
  doc2["humidity"] = humidity;
  doc2["cooling"] = cooling_status;
  doc2["heating"] = heating_status;
  char tempString[12];
  doc2["output"] = fixedPointToString(tempString, tempControl.cs.coolEstimator, 3, 12);
  doc2["heatoutput"] = fixedPointToString(tempString, tempControl.cs.heatEstimator, 3, 12);
  doc2["kp"] = fixedPointToString(tempString, tempControl.cc.Kp, 3, 12);
  doc2["ki"] = fixedPointToString(tempString, tempControl.cc.Ki, 3, 12);
  doc2["kd"] = fixedPointToString(tempString, tempControl.cc.Kd, 3, 12);
  
  if ( Serial.available()) {
    serializeJson(doc2, Serial);
    //Serial.println();
  }
  wdt_enable(WDTO_8S);  // enable watchdog timer with 8 second timeout (max setting)
}

void loop() {
  
  // reset the watchdog timer (once timer is set/reset, next reset pulse must be sent before timeout or arduino reset will occur)
  wdt_reset();

  // subroutines manage their own timings, call every loop
  mainUpdate();

  // check for serial input and update settings
  if(Serial.available()){
    String userInput = Serial.readStringUntil('\n');
    // Serial.print(userInput);
    Serial.println();
    if (userInput.startsWith("/set?")){
      setManual(userInput);
    }
  } 
}
