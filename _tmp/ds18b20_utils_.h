#ifndef __DS18B20UTILS_H
#define __DS18B20UTILS_H

#include <OneWire.h>
#include <DallasTemperature.h>

// temperature sensor
struct TTemperatureSensor {
	bool isReady;
	const char *id;
	const char *name;
	float measuredValue;
	float minValue;
	float maxValue;
	bool criticalState;
};

String addressToStr(DeviceAddress addr);
String enumTemperatureSensors(DallasTemperature &dtSensors);
TTemperatureSensor getTemperatureSensor(uint8_t idx);
uint8_t getTemperatureSensorsCount();
void storeTemperature(uint8_t idx, float value);
void storeTemperature(DeviceAddress addr, float value);
void storeTemperature(const char *name, float value);

double getSensorTemperature(uint8_t idx);
double getSensorTemperatureByName(const char *name);

float T(uint8_t idx);
float TMin(uint8_t idx);
float TMax(uint8_t idx);

#endif
