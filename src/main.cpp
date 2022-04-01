// Notorious PID Fermentation Temperature Control v 0.9
#include <Arduino.h>
// #include <aREST.h>
#include <avr/wdt.h>
#include <OneWire.h>
#include <ArduinoJson.h>
#include <DHT.h>
#include "PID_v1.h"
#include "probe.h"
#include "fridge.h"
#include "globals.h"
#include "functions.h"
#define DEBUG false  // debug flag for including debugging code
#define NUMBER_VARIABLES 20
#define OUTPUT_BUFFER_SIZE 5000
#define _SS_MAX_TX_BUFF 256 // RX buffer size
#define SERIAL_TX_BUFFER_SIZE 256
#define DHTTYPE DHT11

DHT dht(dhtpin, DHTTYPE);

// aREST rest = aREST();

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

  humidity = dht.readHumidity();

  temp_reached = fabs(Setpoint - bt) <= epsilon;

  relay1_status = !digitalRead(relay1);
  relay2_status = !digitalRead(relay2);

  
  StaticJsonDocument<500> doc;
  doc["setpoint"] = Setpoint;
  doc["fanlevel"] = fan_level;
  doc["probetemp"] = bt;
  doc["chambertemp"] = ft;
  doc["humidity"] = humidity;
  doc["cooling"] = relay1_status;
  doc["heating"] = relay2_status;
  doc["output"] = Output;
  doc["heatoutput"] = heatOutput;
  doc["kp"] = Kp;
  doc["ki"] = Ki;
  doc["kd"] = Kd;
  doc["hkp"] = heatKp;
  doc["hki"] = heatKi;
  doc["hkd"] = heatKd;


  if( Serial.available()) {
    serializeJson(doc, Serial);
    // Serial.println();
  }

  if(relay1_status == false && relay2_status == false){
    setPWM1A(0.0f);
  } else {
    setPWM1A(fan_level);
  }
}

int setManual(String command){
  // parse string in the format param0=abc&param1=def
  //for (int i=0; i<8; i++){
  String param = getValue(command, '=', 0);
  String value = getValue(command, '=', 1);
  // strip '/set?' prefix from param value
  param.remove(0,5);
    //Serial.print(param);
    //Serial.println();
    //Serial.print(value);
    //Serial.println();
  if (param == "setpoint"){
    Setpoint = value.toDouble();
  }
  else if (param == "fanlevel"){
    fan_level = value.toFloat();
    setPWM1A(fan_level);
  }
  else if (param == "kp"){
    Kp = value.toDouble();
  }
  else if (param == "ki"){
    Ki = value.toDouble();
  }
  else if (param == "kd"){
    Kd = value.toDouble();
  }
  else if (param == "hkp"){
    heatKp = value.toDouble();
  }
  else if (param == "hki"){
    heatKi = value.toDouble();
  }
  else if (param == "hkd"){
    heatKd = value.toDouble();
  }
  
  mainPID.SetTunings(Kp, Ki, Kd);
  mainPID.initHistory();
  heatPID.SetTunings(heatKp, heatKi, heatKd); 
  heatPID.initHistory();
  return 1;
}

void setup() {
  // set up fans
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

  // configure relay pins and write default HIGH (relay open)
  pinMode(relay1, OUTPUT);
    digitalWrite(relay1, HIGH);
  pinMode(relay2, OUTPUT);
    digitalWrite(relay2, HIGH);

  dht.begin();  

  //Serial.begin(9600);
  Serial.begin(115200);

  // initialize PID controller
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

  humidity = dht.readHumidity();
  // relays use 0 for on, 1 for off, so we invert for the api exposure
  relay1_status = !digitalRead(relay1);
  relay2_status = !digitalRead(relay2);

  StaticJsonDocument<500> doc2;
  doc2["setpoint"] = Setpoint;
  doc2["fanlevel"] = fan_level;
  doc2["probetemp"] = bt;
  doc2["chambertemp"] = ft;
  doc2["humidity"] = humidity;
  doc2["cooling"] = relay1_status;
  doc2["heating"] = relay2_status;
  doc2["output"] = Output;
  doc2["heatoutput"] = heatOutput;
  doc2["kp"] = Kp;
  doc2["ki"] = Ki;
  doc2["kd"] = Kd;
  doc2["hkp"] = heatKp;
  doc2["hki"] = heatKi;
  doc2["hkd"] = heatKd;
  
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
    
  // unsigned long currentTime = millis();
  // const unsigned long updateInterval = 1000;
  // static unsigned long updateTimer = 0;

  // check for serial input and update settings
  if(Serial.available()){
    String userInput = Serial.readStringUntil('\n');
    // Serial.print(userInput);
    Serial.println();
    if (userInput.startsWith("/set?")){
      setManual(userInput);
    }
  } 

  // if (currentTime - updateTimer <= updateInterval){
  //   // if(Setpoint != oldSetpoint){
  //   updateTimer += updateInterval;
  //   mainUpdate();
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
