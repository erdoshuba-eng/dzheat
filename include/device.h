#ifndef __DEVICE_H
#define __DEVICE_H

#include <Arduino.h>
#include <ArduinoJson.h>
#if defined(ESP32)
    #include <WiFi.h>
#elif defined(ESP8266)
    #include <ESP8266WiFi.h>
#endif
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

#define UNDEFINED_TEMP -1000

//void controlRemoteGate(IPAddress host, String newState);
//void controlRemoteGate(String host, String newState);

class TDevice {
protected:
  bool _changed;
  // size_t _jsonSize;
  virtual void parseConfigData(JsonDocument &doc) = 0;
  virtual JsonDocument configData() = 0;
public:
  String _id;
  String _deviceId;
  uint8_t _type;
  String _type2; // remote class name
  String _name;
  uint8_t _readyState;
  TDevice();
  TDevice(String id, uint8_t type, String type2, String name, String deviceId = ""); // deviceId: UUID, registered in the monitor database
  virtual ~TDevice() {}
  virtual JsonDocument capabilities() { JsonDocument doc; return doc; }
  virtual bool processCommand(JsonDocument &doc, String &response) { return false; }
  virtual void publishStatus() {}
  String getConfig() const;
  String getState() const; // for the monitor
  JsonDocument getState2() const; // internal
//  String toString(); // used for serialization
  bool loadConfig(); // load configuration from the file system
  bool storeConfig(); // store configuration to the file system
  bool hasChanged() const { return _changed; }
  void clearChanged() { _changed = false; }
  void setName(String name) { _name = name; }
  void setType(uint8_t type) { _type = type; }
};

class TThermometer: public TDevice {
protected:
  TTemperatureSensor *_tempSensor;
  // float _measuredValue;
  float _minValue;
  float _maxValue;

  void parseConfigData(JsonDocument &doc) override {}
  JsonDocument configData() override { JsonDocument doc; return doc; }

public:
  TThermometer();
  TThermometer(String id, String name);
  TThermometer(TTemperatureSensor *sensor);
  String getConfig() const;
  float getMinValue() const { return _minValue; }
  float getMaxValue() const { return _maxValue; }
  JsonDocument getState() const;
  JsonDocument getState2() const;
  // virtual bool storeConfig() { return true; }
  short getDirection() const {
    if (_tempSensor) {
      return _tempSensor->direction;
    }
    return 0;
  }
  float getTemperature() const {
    if (_tempSensor) { return _tempSensor->measuredValue; }
    return UNDEFINED_TEMP;
  }
  bool getIsCritical() const {
    if (_tempSensor) { return _tempSensor->criticalState; }
    return false;
  }
  void setTemperature(float value) {
    if (_tempSensor) {
      _tempSensor->measuredValue = value;
      return;
    }
    // _measuredValue = value;
  }
  // void setSensor(String address, String name) {
  //   _id = address;
  //   _name = name;
  // }
  void setSensor(TTemperatureSensor *sensor) {
    _tempSensor = sensor;
    _id = _tempSensor->address;
    _name = _tempSensor->name;
  }
};

struct GateConfig {
  uint8_t GPIO;
  uint8_t openState;
  uint8_t closedState;
  bool isOpen;
  bool isRemote;
  String name;
  // IPAddress remoteIp;
  String ip;
  uint16_t port;
  unsigned long lastChange;
};

class TGate: public TDevice {
protected:
  // uint8_t _GPIO;
  // uint8_t _openState;
  // uint8_t _closedState;
  // bool _isOpen;
  // bool _isRemote;
  // IPAddress _remoteIp;
  // String _remoteHost;
  // uint16_t _port;
  // unsigned long _lastChange;
  GateConfig _config; // internal state
  GateConfig config; // published state, used to track changes

  void parseConfigData(JsonDocument &doc) override;
  JsonDocument configData() override { JsonDocument doc; return doc; }

public:
  TGate();
  TGate(uint8_t GPIO); // deprecated
  TGate(uint8_t GPIO, uint8_t openState);
  void identify(String id, String name, String deviceId = "") { _id = id; _name = name; _deviceId = deviceId; }
  String getConfig() const;
  JsonDocument getState() const;
  JsonDocument getState2() const;
  // virtual bool storeConfig() { return true; }
  unsigned long getLastChange() const { return _config.lastChange; };
  bool getOpen() const { return _config.isOpen; };

  void setGPIO(uint8_t GPIO) { _config.GPIO = GPIO; }
  void setOpen(bool bOpen, bool isForced = false);
  void setRemoteOpen(bool bOpen);
  void setOpenState(uint8_t openState);
  // ensure that only one is used !!!!!
  void setRemote(String host) { _config.isRemote = true; _config.ip = host; }
  void publishStatus() override;
  // void setRemote(IPAddress host) { _config.isRemote = true; _config.ip = host; _config.remoteHost = ""; }
  // void setRemote(String host) { _config.isRemote = true; _config.remoteHost = host; _config.remoteIp = IPAddress(); }
};

struct ThermostatConfig {
  float refTemp; // reference temperature to decide if heating is needed
  // uint16_t refTempMin; // minimum allowed reference temperature
  // uint16_t refTempMax; // maximum allowed reference temperature
  String sensibiltity; // code for sensitivity, e.g. "050" means 0.5 degree
  bool enabled; // put on hold
  String mode; // off, man (manual), auto
  bool isOn; // controls the gate open/closed state
  bool forceOn; // force the gate to be open, overrides _isOn
  uint32_t forceOnDuration; // in seconds, how long to keep the gate open when forced
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
  ThermostatConfig _config; // internal state
  ThermostatConfig config; // published state, used to track changes
  unsigned long _forceOnStart; // when the forced on state started
  TGate _gate;
  TThermometer _thermometer;

  void parseConfigData(JsonDocument &doc) override;
  JsonDocument configData();

public:
  TThermostat(String id, String name, String deviceId); // deviceId: UUID, registered in the monitor database
  TGate& getGate() { return _gate; }
  String getMode() const { return _config.mode; }
  /**
   * The reference temperature is used to detect if the
   *   furnace should be turned on or off
   */
  float getRefTemperature() const { return _config.refTemp; }
  float getTemperature() const { return _thermometer.getTemperature(); }
  float getSensibility() const { return _config.sensibiltity.toFloat() / 100.0; }
  virtual JsonDocument capabilities() override;
  virtual bool processCommand(JsonDocument &doc, String &response) override;
  JsonDocument getState();
  bool isEnabled() const { return _config.enabled; }
  bool isOn() const { return _config.isOn || _config.forceOn; }

  void setEnabled(bool enabled) { _config.enabled = enabled; }
  bool forceOn(bool isForced, uint32_t duration = 0);
  bool setMode(String mode);
  bool setRefTemperature(float temperature);
  void adjustRefTemperature(bool up);
  TThermometer& getThermometer() { return _thermometer; }

  bool detectChanges(String reportedState);
};

#endif
