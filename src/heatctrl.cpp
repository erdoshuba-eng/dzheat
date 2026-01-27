/*
 * heatctrl.cpp
 *
 *  Created on: Jan 25, 2026
 *      Author: huba
 *
 */

#include "heatctrl.h"
#include "config.h"

#define TDELTA 2 // +/- interval used as a buffer to control on/off at low temperatures
#define RELAY_DELAY 200 // delay, in ms, between two relay operation

const char* stateHints[] = {
  "alapállapot",
  "kikapcsolt",
  "várakozó",
  "fás kazán -> ház",
  "puffer -> ház",
  "gáz kazán -> ház",
  "fás kazán -> puffer"
};
extern TTemperatureSensor temperatureSensors[];

THeatCtrl::THeatCtrl(String id, String name, String deviceId/*, uint8_t pumpGPIO*/)
:TDevice(id, DEV_FURNACE, "dz_heatctrl", name, deviceId) {
  _hsMode = "auto";
  _hsState = HS_NONE;
  _lastStateChange = 0;
  _heatingTheHouse = false;
  _errorLevel = 0;
  _simulationRunning = false;
  _simulationStep = 0;
}

JsonDocument THeatCtrl::getState() const {
	JsonDocument doc;
	doc["id"] = _deviceId;
	doc["class"] = _type2;
  // doc["data"]["hsState"] = hsState;
  doc["data"]["hsStateHint"] = stateHints[_hsState];
  // doc["data"]["hsMode"] = hsMode;
  doc["data"]["heatingTheHouse"] = _heatingTheHouse;
  doc["data"]["errorLevelHint"] = errorHint();
  if (_simulationRunning) {
    doc["data"]["simulation"] = String(_simulationStep + 1) + ". lépés";
  } else {
    doc["data"]["simulation"] = "";
  }
  JsonArray sensors = doc["sensors"].to<JsonArray>();
  for (int i = 0; i < temperatureSensorsCount; i++) {
    sensors.add(getTempSensorState(i));
  }
  sensors.add(wfp.getState());
  sensors.add(wbp.getState());
  sensors.add(gf.getState());
  sensors.add(tap1.getState());
  sensors.add(tap2.getState());
	return doc;
}

const char* THeatCtrl::errorHint() const {
  switch (_errorLevel) {
    case ERR_LOW: return "Figyelmeztetés";
    case ERR_HIGH: return "Kritikus";
  }
  return "";
}

const char* THeatCtrl::stateHint(uint8_t state) const {
  return stateHints[state];
}

void THeatCtrl::startSimulation() {
  if (_simulationRunning) { return; }
  _simulationRunning = true;
  _hsMode = "auto";
  _heatingTheHouse = false;
  storeTemperature("fás kazán", 24);
  storeTemperature("puffer fent", 25);
  storeTemperature("puffer lent", 25);
  setHsState(HS_PAUSED);
  _simulationStep = 0;
}

/*
step | 0  1  2  3  4  5  6  7  8  9 10 11 12 13 14 15
TWF  |24 60       89 30       80    30
TBT  |25       90       75                   30
TBB  |25    61 90       70                30
heat | 0                    1     0     1        0
state| 2  6  2              4  3  6  2  4     5  2
err  | 0        1  2  1  0
*/
void THeatCtrl::stepSimulation() {
  _simulationStep++;
//  Serial.printf("%d.\n", simulationStep);
  switch (_simulationStep) {
    case 1: storeTemperature("fás kazán", 60); break;
    case 2: storeTemperature("puffer lent", 61); break;
    case 3: storeTemperature("puffer fent", 90); storeTemperature("puffer lent", 90); break;
    case 4: storeTemperature("fás kazán", 89); break;
    case 5: storeTemperature("fás kazán", 30); break;
    case 6: storeTemperature("puffer fent", 75); storeTemperature("puffer lent", 70); break;
    case 7: _heatingTheHouse = true; break;
    case 8: storeTemperature("fás kazán", 80); break;
    case 9: _heatingTheHouse = false; break;
    case 10: storeTemperature("fás kazán", 30); break;
    case 11: _heatingTheHouse = true; break;
    case 12: storeTemperature("puffer lent", 30); break;
    case 13: storeTemperature("puffer fent", 30); break;
    case 14: _heatingTheHouse = false; break;
    case 15: _simulationRunning = false; break;
  }
}

JsonDocument cmdStartSimulation() {
	JsonDocument cmd;
	cmd["command"] = "simulation";
	cmd["label"] = "Szimuláció indítás";
	return cmd;
}

JsonDocument THeatCtrl::capabilities() {
	JsonDocument doc;
	JsonArray commands = doc.to<JsonArray>();
	commands.add(cmdStartSimulation());
	// commands.add(cmdSetRefTemp());
	// commands.add(cmdForceOn());
	return doc;
}

bool THeatCtrl::processCommand(JsonDocument &doc, String &response) {
	String command = doc["command"] | "";
	if (command == "capabilities") {
		JsonDocument capDoc = capabilities();
		serializeJson(capDoc, response);
		return true;
	}
	if (command == "simulation") {
		String mode = doc["params"]["mode"] | "";
        startSimulation();
		return true;
	// } else
	// if (command == "setRefTemp") {
	// 	float temperature = doc["params"]["temperature"].as<float>();
	// 	return setRefTemperature(temperature);
	// } else
	// if (command == "forceOn") {
	// 	Serial.println("force on, duration " + doc["params"]["duration"].as<String>());
	// 	uint32_t duration = doc["params"]["duration"].as<uint32_t>() | 20;
	// 	return forceOn(true, duration);
	}
	return false;
}

/**
 * Put the 3-way taps into the default position
 */
void THeatCtrl::M1() {
  tap1.setOpen(false); delay(RELAY_DELAY);
  tap2.setOpen(false); delay(RELAY_DELAY);
}

/**
 * Put the 3-way taps into the other position
 */
void THeatCtrl::M2() {
  tap1.setOpen(true); delay(RELAY_DELAY);
  tap2.setOpen(true); delay(RELAY_DELAY);
}

/**
 * Change the state of the heating system
 */
void THeatCtrl::setHsState(uint8_t newState) {
  if (newState == _hsState) { return; }
  if (millis() - _lastStateChange < 500) { return; }
  switch (newState) {
  case HS_PAUSED:
    // kind of switch off, keep taps position, but stop pumps and gas furnace
    // different behavior based on the old state
    switch (_hsState) {
    case HS_GAS_HOUSE:
      gf.setOpen(false); // switch off heating from the gas furnace
      break;
    case HS_WOOD_HOUSE: // switch off heating from the wood furnace
      // if (heatingTheHouse && (T(iTWF) > TMin(iTWF) - TDELTA)) return;
      wfp.setOpen(false);
      break;
    case HS_BUFFER_HOUSE: // switch off heating from the buffer
      // if (heatingTheHouse && (T(iTBT) > TMin(iTBT) - TDELTA)) return;
      wbp.setOpen(false);
      break;
    case HS_WOOD_BUFFER: // switch off heating from the wood furnace
      // if (T(iTWF) > (TMin(iTWF) - TDELTA) && T(iTWF) > T(iTBB)) return;
      wfp.setOpen(false);
      break;
    }
    break;
  case HS_WOOD_HOUSE:
    M1();
    wfp.setOpen(true); delay(RELAY_DELAY);
    wbp.setOpen(false); delay(RELAY_DELAY);
    gf.setOpen(false);
    break;
  case HS_BUFFER_HOUSE:
    M2();
    wfp.setOpen(false); delay(RELAY_DELAY);
    wbp.setOpen(true); delay(RELAY_DELAY);
    gf.setOpen(false);
    break;
  case HS_GAS_HOUSE:
    // if (hsState == HS_BUFFER_HOUSE && heatingTheHouse && (T(iTBT) > TMin(iTBT) - TDELTA)) return;
    M1();
    wfp.setOpen(false); delay(RELAY_DELAY);
    wbp.setOpen(false); delay(RELAY_DELAY);
    gf.setOpen(true);
    break;
  case HS_WOOD_BUFFER:
    M2();
    wfp.setOpen(true); delay(RELAY_DELAY);
    wbp.setOpen(false); delay(RELAY_DELAY);
    gf.setOpen(false);
    break;
  default: // HS_OFF
    // close all gates
    newState = HS_OFF;
    wfp.setOpen(false);
    wbp.setOpen(false);
    gf.setOpen(false);
    tap1.setOpen(false);
    tap2.setOpen(false);
    break;
  }

#ifdef DEBUG
  Serial.println();
  if (_simulationRunning) {
    Serial.printf("step: %u\n", _simulationStep);
  }
  String s = _heatingTheHouse ? "YES" : "NO";
  Serial.println("heating the house: " + s);
  Serial.printf("old state: (%u) ", _hsState);
  Serial.print(stateHint(_hsState));
  Serial.printf(", new state: (%u) ", newState);
  Serial.println(stateHint(newState));
  for (int i = 0; i < temperatureSensorsCount; i++) {
    Serial.print(temperatureSensors[i].name);
    Serial.print(": ");
    Serial.print(temperatureSensors[i].measuredValue);
    Serial.print(", ");
  }
  Serial.println();
#endif
  _hsState = newState;
  _lastStateChange = millis();
}

void THeatCtrl::manageSystem() {
  if (_hsMode == "auto") { // automatic command mode
    if (_heatingTheHouse) {
      if (T(iTWF) > TMin(iTWF) + TDELTA) {
        setHsState(HS_WOOD_HOUSE); // heating from the wood furnace
        return;
      }
      if (_hsState == HS_WOOD_HOUSE && T(iTWF) < TMin(iTWF) - TDELTA) {
        setHsState(HS_PAUSED); // switch off heating from the wood furnace
      }
      if (T(iTBT) > TMin(iTBT) + TDELTA) {
        setHsState(HS_BUFFER_HOUSE); // heating from the water buffer
        return;
      }
      if (_hsState == HS_BUFFER_HOUSE && T(iTBT) < TMin(iTBT) - TDELTA) {
        setHsState(HS_PAUSED); // switch off heating from the water buffer
      }
      if (_hsState != HS_WOOD_HOUSE && _hsState != HS_BUFFER_HOUSE) {
        setHsState(HS_GAS_HOUSE); // heating from the gas furnace
      }
    } else {
      if (T(iTWF) >= T(iTBB)) {
        // heating the water buffer from the wood furnace while it's temperature
        // is greater than the water buffer's temperature measured at the bottom (or middle)
        setHsState(HS_WOOD_BUFFER);
      } else {
        // kind of switch off
        setHsState(HS_PAUSED); // keep taps position, but stop water pumps and gas furnace
      }
    }
  } else { // manual command mode, nothing allowed yet
    if (_hsState == HS_WOOD_HOUSE && T(iTWF) < TMin(iTWF)) {
      setHsState(HS_PAUSED);
    } else
    if (_hsState == HS_BUFFER_HOUSE && T(iTBT) < TMin(iTBT)) {
      setHsState(HS_PAUSED);
    } else
    if (_hsState == HS_WOOD_BUFFER) {
      if (T(iTWF) < TMin(iTWF) || T(iTWF) < T(iTBB)) {
        setHsState(HS_PAUSED);
      }
    }
  }

//   detectError();
}
