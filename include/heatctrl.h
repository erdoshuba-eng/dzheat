/*
 * heatctrl.h
 *
 *  Created on: Jan 25, 2026
 *      Author: huba
 *
 *  The dz_heatctrl remote class implementation
 */

#ifndef HEATCTRL_H_
#define HEATCTRL_H_

#include "device.h"

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

class THeatCtrl: public TDevice {
protected:
  String _hsMode;
  uint8_t _hsState;
  unsigned long _lastStateChange;
  bool _heatingTheHouse;
  uint8_t _errorLevel;
  bool _simulationRunning;
  uint8_t _simulationStep;

  void parseConfigData(JsonDocument &doc) override {};
  JsonDocument configData() override { JsonDocument doc; return doc; }
  void M1();
  void M2();

public:
  TGate wfp; // wood furnace water pump
  TGate wbp; // water buffer pump
  TGate gf; // gas furnace
  TGate tap1; // tap 1
  TGate tap2; // tap 2
  THeatCtrl(String id, String name, String deviceId); // deviceId: UUID, registered in the monitor database
  virtual JsonDocument capabilities() override;
  virtual bool processCommand(JsonDocument &doc, String &response) override;
  void startSimulation();
  void stepSimulation();
  void manageSystem();

  JsonDocument getState() const;
  const char* stateHint(uint8_t state) const;
  uint8_t getHsState() const { return _hsState; }
  bool isHeatingTheHouse() const { return _heatingTheHouse; }
  uint8_t getErrorLevel() const { return _errorLevel; }
  const char* errorHint() const;
  bool isSimulationRunning() const { return _simulationRunning; }
  uint8_t getStep() const { return _simulationStep; }

  void setHsState(uint8_t value);
  void setHeatingTheHouse(bool value) { _heatingTheHouse = value; }
  void setErrorLevel(uint8_t value) { _errorLevel = value; }
};

#endif /* HEATCTRL_H_ */