// Notorious PID Fermentation Temperature Control v 0.9
#include <Arduino.h>
#include <aREST.h>
#include <avr/wdt.h>
#include <OneWire.h>
// #include <ArduinoJson.h>
#include "PID_v1.h"
#include "probe.h"
#include "fridge.h"
#include "globals.h"
#include "functions.h"
#define DEBUG false  // debug flag for including debugging code

// #define AREST_PARAMS_MODE 1

aREST rest = aREST();
// StaticJsonDocument<200> doc;

void mainUpdate();  // update sensors, PID output, fridge state, write to log, run profiles
int setManual(String command);

// call all update subroutines
// start conversion for all sensors
// update sensors when conversion complete
void mainUpdate() {
  probe::startConv();
  if (probe::isReady()) {
    fridge.update();
    beer.update();
    Input = beer.getFilter();
  }

  mainPID.Compute();
  updateFridge();  

  bt = beer.getTemp();
  ft = fridge.getTemp();

  temp_reached = fabs(Setpoint - bt) <= epsilon;

  relay1_status = !digitalRead(relay1);
  relay2_status = !digitalRead(relay2);

  if(relay1_status == 1 && relay2_status == 1){
    setPWM1A(0.0f);
  } else {
    setPWM1A(fan_level);
  }
}

int setManual(String command){
  // parse string in the format param0=abc&param1=def
  for (int i=0; i<8; i++){
    String param = getValue(command, '&', i);
    String value = getValue(param, '=', 1);
    switch (i) {
      case 0:
        Setpoint = value.toDouble();
        break;
      case 1: 
        fan_level = value.toFloat();
        setPWM1A(fan_level);
        break;
      case 2:
        Kp = value.toDouble();
        break;
      case 3:
        Ki = value.toDouble();
        break;
      case 4:
        Kd = value.toDouble();
        break;
      case 5:
        heatKp = value.toDouble();
        break;
      case 6:
        heatKi = value.toDouble();
        break;
      case 7:
        heatKd = value.toDouble();
        break;
    }
  }

  // String param0 = getValue(command, '&', 0);
  // String param1 = getValue(command, '&', 1);

  // // get param values assuming they are in the order [setpoint,fanspeed]
  // String setpoint = getValue(param0, '=', 1);
  // String fanspeed = getValue(param1, '=', 1);
  // fan_level = fanspeed.toFloat();

  // // set new setpoint and fan speed
  // Setpoint = setpoint.toDouble();
  // setPWM1A(fan_level);
  return 1;
}

void setup() {
   //enable outputs for Timer 1
  pinMode(fan1a, OUTPUT);
  pinMode(fan1b, OUTPUT);
  setupTimer1();
  //enable outputs for Timer 2
  pinMode(fan2, OUTPUT); //2
  setupTimer2();
  //note that pin 11 will be unavailable for output in this mode!
  setPWM1A(fan_level); //set duty to 50% on pin 9
  // digitalWrite(fan1a, LOW);
  // digitalWrite(fan1b, LOW);

  pinMode(relay1, OUTPUT);  // configure relay pins and write default HIGH (relay open)
    digitalWrite(relay1, HIGH);
  pinMode(relay2, OUTPUT);
    digitalWrite(relay2, HIGH);

  // Serial.begin(9600);
  Serial.begin(115200);

  fridge.init();
  beer.init();
  
  mainPID.SetTunings(Kp, Ki, Kd);    // set tuning params
  mainPID.SetSampleTime(1000);       // (ms) matches sample rate (1 hz)
  mainPID.SetOutputLimits(0.3, 50);  // deg C (~32.5 - ~100 deg F)
  mainPID.SetMode(AUTOMATIC);
  mainPID.setOutputType(FILTERED);
  mainPID.setFilterConstant(10);
  mainPID.initHistory();

  heatPID.SetTunings(heatKp, heatKi, heatKd);
  heatPID.SetSampleTime(heatWindow);       // sampletime = time proportioning window length
  heatPID.SetOutputLimits(0, heatWindow);  // heatPID output = duty time per window
  heatPID.SetMode(AUTOMATIC);
  heatPID.initHistory();

  bt = beer.getTemp();
  ft = fridge.getTemp();

  // relays use 0 for on, 1 for off, so we invert for the api exposure
  relay1_status = !digitalRead(relay1);
  relay2_status = !digitalRead(relay2);

  // expose variables to REST API
  // modifiable variables
  rest.variable("setpoint", &Setpoint);
  rest.variable("fanlevel", &fan_level);

  // sensor values
  rest.variable("probetemp", &bt);
  rest.variable("chambertemp", &ft);
  rest.variable("cooling", &relay1_status);
  rest.variable("heating", &relay2_status);
  rest.variable("tempreached", &temp_reached);

  // PID values
  rest.variable("output", &Output);
  rest.variable("heatoutput", &heatOutput);
  rest.variable("kp", &Kp);
  // Serial communication sometimes hangs when enabling these vars
  rest.variable("ki", &Ki);
  rest.variable("kd", &Kd);
  rest.variable("heatkp", &heatKp);
  rest.variable("heatki", &heatKi);
  rest.variable("heatkd", &heatKd);

  rest.function((char*)"set", setManual);
  // rest.function((char*)"pidset", setManualPID);
  // rest.function((char*)"profile", setProfile);

  rest.set_id("2");
  rest.set_name((char*)"serial");
  
  wdt_enable(WDTO_8S);  // enable watchdog timer with 8 second timeout (max setting)
}

void loop() {
  
  rest.handle(Serial);
  wdt_reset();                      // reset the watchdog timer (once timer is set/reset, next reset pulse must be sent before timeout or arduino reset will occur)
  mainUpdate();                            // subroutines manage their own timings, call every loop
    
  // setPWM1B(0.2f); //set duty to 20% on pin 10
  // setPWM2(0.8f); //set duty to 80% on pin 3

  // unsigned long currentTime = millis();
  // const unsigned long updateInterval = 5000;
  // static unsigned long updateTimer = 0;
  // double oldSetpoint = Setpoint;

  // if(Serial.available()){
  //   String userInput = Serial.readStringUntil('\n');
  //   Setpoint = userInput.toDouble();
  // } 

  // if (currentTime - updateTimer <= updateInterval){
  //   // if(Setpoint != oldSetpoint){
  //   updateTimer += updateInterval;
  //   Serial.print("Setpoint: ");
  //   Serial.print(Setpoint, 1);
  //   Serial.println("°C");
  //   Serial.print("Beer Temp:");
  //   Serial.print(beer.getTemp(), 1);
  //   Serial.println("°C");
  //   Serial.print("Fridge Temp:");
  //   Serial.print(fridge.getTemp(), 1);
  //   Serial.println("°C");
  //   Serial.print("Output Temp:");
  //   Serial.print(Output, 1);
  //   Serial.println("°C");
  //   Serial.print("Heat Output Temp:");
  //   Serial.print(heatOutput, 1);
  //   Serial.println("°C");
  //   // }
  // }
}
