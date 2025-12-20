/*
 * config.cpp
 *
 *  Created on: Jul 12, 2020
 *      Author: huba
 */

#include "config.h"
// #include <device.h>
#include "ds18b20_utils.h"

const char* device = "dzheat";
// const char* ver = "4.0.0"; // 2020-07-25
const char* ver = "5.0.0"; // 2024-09-30
const char* deviceId = "e625bcbb-fd38-44d0-8cb4-356c9a57ca2e";
const char* logAuth = "ZGVha3pvbGk6Ln5beDIqOkRwflxb"; // zoli

#ifdef ENYEM
  // network
  const char* SSID = "nyanya";
  const char* password = "prikulics5E";
//  const char* rabbitUsr = "huba";
//  const char* rabbitPsw = "J<(#',4s^L_8";

  TTemperatureSensor temperatureSensors[temperatureSensorsCount] = {
    {false, "2873c479971303d0", "fás kazán", -1, 45, 85, false},
    {false, "28082cc10b000011", "puffer fent", -1, 40, 82, false},
    {false, "282733bf0b00002d", "puffer lent", -1, 40, 82, false}
  };
#else
  // network
  // const char* SSID = "UPC2455429"; // regi
  // const char* password = "h6dryuA7spne"; // regi
  const char* SSID = "TP-Link_9484";
  const char* password = "11681403";
//  const char* rabbitUsr = "deakzoli";
//  const char* rabbitPsw = ".~[x2*:Dp~\[";

  TTemperatureSensor temperatureSensors[temperatureSensorsCount] = {
    {false, "28ff283ba616050c", "fás kazán", -1, 45, 85, false}, // TWF
    {false, "28ffb1f0a41605aa", "puffer fent", -1, 40, 82, false}, // TBT
    {false, "28ffc92ca0160574", "puffer lent", -1, 40, 82, false} // TBB
  };
#endif

const char* stateNames[] = {
  "alapállapot",
  "kikapcsolt",
  "várakozó",
  "fás kazán -> ház",
  "puffer -> ház",
  "gáz kazán -> ház",
  "fás kazán -> puffer"
};
//const char* stateNames[] = {
//  "none",
//  "offline",
//  "paused",
//  "wood furnace -> house",
//  "buffer -> house",
//  "gas furnace -> house",
//  "wood furnace -> buffer",
//};
