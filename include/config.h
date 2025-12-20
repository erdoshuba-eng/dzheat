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

#define DEBU
#define ENYE

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

// seconds between two temperature reading operation
#ifdef DEBUG
  #define freqReadTemperature 10
#else
  #define freqReadTemperature 15
#endif
/*
// connected devices
#define PIN_THERMOSTAT 12 // input, house thermostat
#define PIN_LED 13 // output, error and house heating indicator led

#define devicesCount 5
// control pins
#define PIN_WFP 16 // output, wood furnace water pump, zold
#define PIN_WBP 4 // output, water buffer pump, feher
#define PIN_GF 15 // output, gas furnace, sarga
#define PIN_T1 5 // output, tap1, barna
#define PIN_T2 17 // output, tap2, szurke*/

// connected devices
#define ONE_WIRE_BUS 0 // thermometer, OneWire communication
#define PIN_THERMOSTAT 4 // input, house thermostat
#define PIN_LED 5 // output, error and house heating indicator led

#define devicesCount 5
// control pins
#define PIN_WFP 27 // output, wood furnace water pump, zold
#define PIN_WBP 26 // output, water buffer pump, feher
#define PIN_GF 25 // output, gas furnace, sarga
#define PIN_T1 33 // output, tap1, barna
#define PIN_T2 14 // output, tap2, szurke

#define iTWF 0
#define iTBT 1
#define iTBB 2

#define TDELTA 2 // +/- interval used as a buffer to control on/off at low temperatures
#define RELAY_DELAY 200 // delay, in ms, between two relay operation

// states of the heating system
#define HS_NONE 0
#define HS_OFF 1
#define HS_PAUSED 2
#define HS_WOOD_HOUSE 3
#define HS_BUFFER_HOUSE 4
#define HS_GAS_HOUSE 5
#define HS_WOOD_BUFFER 6

// error states
#define ERR_NO_ERROR 0
#define ERR_LOW 1
#define ERR_HIGH 2
#define FREQ_LOW 2
#define FREQ_HIGH 1


#endif /* CONFIG_H_ */
