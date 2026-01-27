#ifndef __DEVICE_H
#define __DEVICE_H

// #include <Arduino.h>
#include <ArduinoJson.h>
#include "ds18b20_utils.h"

#define SE_VERSION 0

// sensor types
#define SE_TEMPERATURE 1
#define SE_WATER_PUMP 2
#define SE_MOISTURE 3
#define SE_TWO_STATE_TAP 4
#define SE_GAS_FURNACE 5
#define SE_THERMOSTAT 6
// device types
#define DEV_THERMOSTAT 101
#define DEV_FURNACE 102
#define DEV_SYSTEM 103 // generic system

// temperature sensor
// struct TTempSensor {
//   bool isReady;
//   String id;
//   String name;
//   float measuredValue;
//   float minValue;
//   float maxValue;
//   bool criticalState;
// };

//void controlRemoteGate(IPAddress host, String newState);
//void controlRemoteGate(String host, String newState);

class TDevice {
protected:
  bool _changed;
  size_t _jsonSize;
  virtual void parseConfigData(JsonDocument &doc) = 0;
public:
  String _id;
  uint8_t _type;
  String _name;
  uint8_t _readyState;
  TDevice();
  TDevice(String id, uint8_t type, String name);
  virtual ~TDevice() {}
  String getConfig() const;
  String getState() const;
//  String toString(); // used for serialization
  bool loadConfig();
  void setName(String name) { _name = name; }
  void setType(uint8_t type) { _type = type; }
};

class TThermometer: public TDevice {
protected:
  TTemperatureSensor *_tempSensor;
  float _measuredValue;
  float _minValue;
  float _maxValue;

  void parseConfigData(JsonDocument &doc) override {}

public:
  TThermometer();
  TThermometer(String id, String name);
  TThermometer(TTemperatureSensor *sensor);
  String getConfig() const;
  float getMinValue() const { return _minValue; }
  float getMaxValue() const { return _maxValue; }
  String getState() const;
  float getTemperature() const {
    if (_tempSensor) {
      return _tempSensor->measuredValue;
    }
    return _measuredValue;
  }
  bool getIsCritical() const {
    if (_tempSensor) {
      return _tempSensor->criticalState;
    }
    return false; // until isn't solved properly
  }
  void setTemperature(float value) {
    if (_tempSensor) {
      _tempSensor->measuredValue = value;
      return;
    }
    _measuredValue = value;
  }
  void setSensor(String id, String name) {
    _id = id;
    _name = name;
  }
  void setSensor(TTemperatureSensor *sensor) {
    _tempSensor = sensor;
    _id = _tempSensor->id;
    _name = _tempSensor->name;
  }
};

class TGate: public TDevice {
protected:
  uint8_t _GPIO;
  uint8_t _openState;
  uint8_t _closedState;
  bool _isOpen;
  bool _isRemote;
  IPAddress _remoteIp;
  String _remoteHost;
  unsigned long _lastChange;

  void parseConfigData(JsonDocument &doc) override {}

public:
  TGate();
  TGate(uint8_t GPIO);
  TGate(uint8_t GPIO, uint8_t openState);
  String getConfig() const;
  String getState() const;
  unsigned long getLastChange() const { return _lastChange; };
  bool getOpen() const { return _isOpen; };

  void setGPIO(uint8_t GPIO) { _GPIO = GPIO; }
  void setOpen(bool openState);
  // ensure that only one is used !!!!!
  void setRemote(IPAddress host) { _isRemote = true; _remoteIp = host; _remoteHost = ""; }
  void setRemote(String host) { _isRemote = true; _remoteHost = host; _remoteIp = IPAddress(); }
};

/**
 * TThermostat class
 *
 * has a thermometer for measuring the temperature
 * has a gate (relay) to control switching on/off something (a water pump or a gas furnace)
 *   this gate can be controlled directly through a GPIO or remotely using an IP address
 *
 */
class TThermostat: public TDevice {
protected:
  // configuration for auto mode
  String _autoPrg; // default program
  // 2 program, the day is split on two periods
  int _p2H0; // integer representation of the time: 8:30 => 830
  float _p2T0;
  int _p2H1;
  float _p2T1;
  float _sensibiltity;
  // configuration for manual mode
  String _manPrg; // default program
  // temperatures for manual mode
  float _dayT; // temperature for day
  float _nightT; // temperature for night
  String _mode; // off, auto, man (manual)
  bool _needsHeating;
  String _prg; // currently running program: day, night, ...
  bool _enabled; // put on hold
  TGate _gate;
  TThermometer _thermometer;

  void parseConfigData(JsonDocument &doc) override;

public:
  TThermostat(String id, String name);
  String getConfig() const;
  float getDayT() const { return _dayT; }
  TGate& getGate() { return _gate; }
  String getMode() const { return _mode; }
  float getNightT() const { return _nightT; }
  String getPrg() const { return _prg; }
  float getRefTemperature();
  String getState() const;
//  String toString();
  bool isEnabled() const { return _enabled; }
  bool needsHeating() const { return _needsHeating; }

  void setEnabled(bool enabled) { _enabled = enabled; }
  bool setMode(String mode, String prg);
  bool setProgram(String prg);
  bool setPrg2Hours(int h0, int h1) {
    _p2H0 = h0;
    _p2H1 = h1;
    return true;
  }
  bool setTemperature(float temperature, String ttype);
  TThermometer& getThermometer() { return _thermometer; }
  bool storeConfig();

  void detectChanges(String reportedState);
};

class TWoodFurnace: public TDevice {
protected:
  bool _isCooling; // switch on the water pump to cool the furnace
  String _mode; // off, on
  int _onT; // at which temperature to switch on the pump
  int _aliveT; // above this temp. we switch off the thermostat automatically
  TGate _pump;
  TThermometer _thermometer;

  void parseConfigData(JsonDocument &doc) override;

public:
  TWoodFurnace(String id, String name);
  int getAliveT() const { return _aliveT; }
  String getConfig() const;
  String getMode() const { return _mode; }
  String getState() const;
  int getOnTemperature() const { return _onT; }
  TGate& getPump() { return _pump; }
  float getTemperature() const {
    return _thermometer.getTemperature();
  }
  TThermometer& getThermometer() { return _thermometer; }

  bool setMode(String mode);
  bool setTemperature(int temperature, String ttype);
  void setOpen(bool openState) { _pump.setOpen(openState); }
  bool storeConfig();

  void detectChanges();
};

#endif
