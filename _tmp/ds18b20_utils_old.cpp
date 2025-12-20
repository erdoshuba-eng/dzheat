#include "ds18b20_utils.h"
//#include <utils.h>
#include <device.h>
#include "config.h"

extern TTempSensor temperatureSensors[];

/**
 * convert a sensor address to a string format
 */
String addressToStr(DeviceAddress addr) {
  String s = "";
  for (uint8_t i = 0; i < 8; i++) {
    if (addr[i] < 16) s += String(0, HEX);
    s += String(addr[i], HEX);
  }
  return s;
}

/**
 * Return a string about the found temperature sensors
 */
String enumTemperatureSensors(DallasTemperature &dtSensors) {
  DeviceAddress deviceAddress; // temperature sensor address
  int temperatureSensorsFound = dtSensors.getDeviceCount();
  if (temperatureSensorsFound > 0) {
    String s = "found " + String(temperatureSensorsFound) + " temperature sensors\n";

//   uint8_t n = 0; // the index where we store the address
  // loop through the temperature sensors and print their addresses
    for (uint8_t i = 0; i < temperatureSensorsFound; i++) {
      if (dtSensors.getAddress(deviceAddress, i)) {
        s += "device " + String(i) + " has address " + addressToStr(deviceAddress) + "\n";
//      if (n < temperatureSensorsCount) {
//        temperatureSensors[n].id = addressToStr(deviceAddress);
//        n++;
//      }
      }
      else {
        s += "found ghost device at " + String(i) + "\n";
      }
    }
    return s;
  }
  return "false";
}

/**
 * Store the measured value of a temperature sensor
 */
void storeTemperature(uint8_t idx, float value) {
//  Serial.printf("device %s temperature changed from %0.2f to %0.2f\n", temperatureSensors[idx].name.c_str(), temperatureSensors[idx].measuredValue, value);
  temperatureSensors[idx].measuredValue = value;
  temperatureSensors[idx].criticalState = value > temperatureSensors[idx].maxValue;
}

/**
 * Save temperature of the sensor identified by its address
 */
void storeTemperature(DeviceAddress addr, float value) {
  String deviceAddress = addressToStr(addr);
//  Serial.printf("store temperature %0.2f for device %s\n", value, deviceAddress.c_str());
  uint8_t n = temperatureSensorsCount; //countof(temperatureSensors);
  for (uint8_t i = 0; i < n; i++) {
    if (temperatureSensors[i].id == deviceAddress) {
      storeTemperature(i, value);
      break;
    }
  }
}

/**
 * Save temperature of the sensor identified by its name
 */
void storeTemperature(String name, float value) {
  for (uint8_t i = 0; i < temperatureSensorsCount; i++) {
    if (temperatureSensors[i].name == name) {
      storeTemperature(i, value);
      break;
    }
  }
}

/**
 * Return measured temperature of the sensor at the given index
 */
float T(uint8_t idx) {
  return temperatureSensors[idx].measuredValue;
}

/**
 * Return minimum temperature of the sensor at the given index
 */
float TMin(uint8_t idx) {
  return temperatureSensors[idx].minValue;
}

/**
 * Return maximum temperature of the sensor at the given index
 */
float TMax(uint8_t idx) {
  return temperatureSensors[idx].maxValue;
}
