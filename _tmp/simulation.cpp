/*
 * simulation.cpp
 *
 *  Created on: Jul 18, 2020
 *      Author: huba
 */


#include "simulation.h"
#include "ds18b20_utils.h"

extern char* hsMode;
// extern bool heatingTheHouse;
bool heatingTheHouse;

int simulationStep = 0;
bool simulationRunning = false;

/*
step | 0  1  2  3  4  5  6  7  8  9 10 11 12 13 14 15
TWF  |25 60       90 30       80    30
TBT  |25       90       70                   30
TBB  |25    60 90       70                30
heat | 0                    1     0     1        0
state| 2  6  2              4  3  6  2  4     5  2
err  | 0        1  2  1  0
*/

void simulationStepNext() {
  simulationStep++;
//  Serial.printf("%d.\n", simulationStep);
  switch (simulationStep) {
    case 1: storeTemperature("fás kazán", 60); break;
    case 2: storeTemperature("puffer lent", 60); break;
    case 3: storeTemperature("puffer fent", 90); storeTemperature("puffer lent", 90); break;
    case 4: storeTemperature("fás kazán", 90); break;
    case 5: storeTemperature("fás kazán", 30); break;
    case 6: storeTemperature("puffer fent", 70); storeTemperature("puffer lent", 70); break;
    case 7: heatingTheHouse = true; break;
    case 8: storeTemperature("fás kazán", 80); break;
    case 9: heatingTheHouse = false; break;
    case 10: storeTemperature("fás kazán", 30); break;
    case 11: heatingTheHouse = true; break;
    case 12: storeTemperature("puffer lent", 30); break;
    case 13: storeTemperature("puffer fent", 30); break;
    case 14: heatingTheHouse = false; break;
    case 15: simulationRunning = false; break;
  }
}

void startSimulation() {
  simulationRunning = true;
  hsMode = (char *)"auto";
  heatingTheHouse = false;
  storeTemperature("fás kazán", 25);
  storeTemperature("puffer fent", 25);
  storeTemperature("puffer lent", 25);
  simulationStep = 0;
}


