/*
 * config.h
 *
 *  Created on: Jul 12, 2020
 *      Author: huba
 *
 *  A file used to store configuration parameters (network, sensors, pin usage, ...)
 */

#ifndef CONFIG_H_
#define CONFIG_H_

#include <WiFi.h>

#define DEBUG_
#define ENYEM_
#define USE_SSL

#ifdef ENYEM
  // network
  const IPAddress ipHeatCtrl(192, 168, 1, 13);
  const IPAddress ipGateway(192, 168, 1, 1);
  const IPAddress ipSubnet(255, 255, 255, 0);

  // temperature sensors
  #define temperatureSensorsCount 3
#else
  // network
  const IPAddress ipHeatCtrl(192, 168, 0, 2);
  const IPAddress ipGateway(192, 168, 0, 1);
  const IPAddress ipSubnet(255, 255, 255, 0);

  // temperature sensors
  #define temperatureSensorsCount 3
#endif

// seconds between two temperature reading operations
#define freqReadTemperature 8

// connected devices
#define ONE_WIRE_BUS 0 // thermometer, OneWire communication
#define PIN_THERMOSTAT 4 // input, house thermostat
#define PIN_LED 5 // output, error and house heating indicator led

// control pins
#define PIN_WFP 27 // output, wood furnace water pump, zold
#define PIN_WBP 26 // output, water buffer pump, feher
#define PIN_GF 25 // output, gas furnace, sarga
#define PIN_T1 33 // output, tap1, barna
#define PIN_T2 14 // output, tap2, szurke

#define iTWF 0
#define iTBT 1
#define iTBB 2

#endif /* CONFIG_H_ */
