#ifndef FUNCTIONS_H
#define FUNCTIONS_H

#include <Arduino.h>

String getValue(String data, char separator, int index);
int digitalReadOutputPin(uint8_t pin);
void setupTimer1();
void setupTimer2();

void setPWM1A(float f);
void setPWM1B(float f);
void setPWM2(float f);
void tachISR();
unsigned long calcRPM();

#endif