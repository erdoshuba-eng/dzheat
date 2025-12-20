#ifndef __DS18B20UTILS_H
#define __DS18B20UTILS_H

#include <OneWire.h>
#include <DallasTemperature.h>

String addressToStr(DeviceAddress addr);
String enumTemperatureSensors(DallasTemperature &dtSensors);
void storeTemperature(uint8_t idx, float value);
void storeTemperature(DeviceAddress addr, float value);
void storeTemperature(String name, float value);

float T(uint8_t idx);
float TMin(uint8_t idx);
float TMax(uint8_t idx);

#endif
