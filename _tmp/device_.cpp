#include "device.h"
#include "utils.h"

#include "FS.h"
#include "SPIFFS.h"

#include "udp.h"

/**
 * Switch on/off a gate type device remotely sending a UDP message or
 *   making a TCP request
 * This function shall be integrated into the TGate class
 *
 * @param {IPAddress} host
 * @param {String} newState "on" of "off"
 */
void controlRemoteGate(IPAddress host, String newState) {
//  unsigned long tStart = millis();
  String msg = "gfswitch&cmd=setState&state=" + newState;
  sendUDPMessage(host, msg, UDP_SLAVE);
#ifdef DEBUG
//  Serial.printf("control remote gate %u ms\n", (tEnd - tStart));
#endif
}


void controlRemoteGate(String host, String newState) {
  String msg = host + "&cmd=setState&state=" + newState;
  for (int i = 1; i <= 5; i++) {
//    broadcastUDPMessage(msg);
    delay(500);
  }
}

TDevice::TDevice() {
  _changed = false;
  _jsonSize = 0;
  _readyState = 0; // ready
}

TDevice::TDevice(String id, uint8_t type, String name)
:TDevice() {
  _id = id;
  _type = type;
  _name = name;
}

String TDevice::getConfig() const {
  char tmp[255];
  String t = R"("type":%d,"id":"%s","name":"%s")";
  sprintf(tmp, t.c_str(), _type, _id.c_str(), _name.c_str());
  return String(tmp);
}

String TDevice::getState() const {
  char tmp[255];
  String t = R"("type":%d,"id":"%s","name":"%s")";
  sprintf(tmp, t.c_str(), _type, _id.c_str(), _name.c_str());
  return String(tmp);
}

bool TDevice::loadConfig() {
  String configFile = "/_" + String(_type) + _id + ".json";
#ifdef DEBUG
  Serial.print("check configuration file ");
  Serial.println(configFile.c_str());
#endif
  if (SPIFFS.exists(configFile)) {
#ifdef DEBUG
    Serial.print("load data from the configuration file ");
    Serial.println(configFile.c_str());
#endif
    File f = SPIFFS.open(configFile, "r");
    if (f) {
      DynamicJsonDocument doc(_jsonSize);
      DeserializationError err = deserializeJson(doc, f);
      if (err) {
#ifdef DEBUG
        Serial.println(err.c_str());
#endif
      }
      else {
        parseConfigData(doc);
        return true;
      }
    }
  }
  else {
#ifdef DEBUG
    Serial.print("config file not found ");
    Serial.println(configFile.c_str());
#endif
  }
  return false;
}

// ************** TThermometer class

TThermometer::TThermometer() {
  _type = SE_TEMPERATURE;
  _minValue = -1000;
  _maxValue = 1000;
  _measuredValue = 0;
  _tempSensor = NULL;
}

TThermometer::TThermometer(String id, String name)
:TThermometer() {
  setSensor(id, name);
}

TThermometer::TThermometer(TTemperatureSensor *sensor)
:TThermometer() {
  setSensor(sensor);
}

/**
 * Configuration means how the device is configured, what are the limits
 * The configuration is usually done once, and is changed only in rare case
 */
String TThermometer::getConfig() const {
  char tmp[255];
  String resp = "{" + TDevice::getConfig();
  String t = R"(,"min":%0.1f,"max":%0.1f)";

  sprintf(tmp, t.c_str(), _minValue, _maxValue);
  resp += String(tmp) + "}";
  return resp;
}

/**
 * State means the current state of the device. The sate varies in time
 *   faster or slower
 */
String TThermometer::getState() const {
  char tmp[255];
  String resp = "{" + TDevice::getState();
  String t = R"(,"temperature":%0.1f,"critical":%s)";

  sprintf(tmp, t.c_str(), getTemperature(), b2s(getIsCritical()).c_str());
  resp += String(tmp) + "}";
  return resp;
}

// ************** TGate class

TGate::TGate()
:TDevice("", SE_WATER_PUMP, "") {
  _openState = LOW;
  _closedState = HIGH;
  // old version, through 1st optocoupler
//  _openState = HIGH;
//  _closedState = LOW;
  _isOpen = false;
  _GPIO = 0;
  _isRemote = false;
  _remoteHost = "";
  _lastChange = 0;
}

TGate::TGate(uint8_t GPIO)
:TGate::TGate() {
  _GPIO = GPIO;
  pinMode(_GPIO, OUTPUT);
  setOpen(false);
}

TGate::TGate(uint8_t GPIO, uint8_t openState)
:TGate::TGate() {
  _GPIO = GPIO;
  _openState = openState;
  _closedState = !openState;
  setOpen(false);
}

String TGate::getConfig() const {
  char tmp[255];
  String resp = "{" + TDevice::getConfig();
  String t = R"(,"GPIO":%u)";

  sprintf(tmp, t.c_str(), _GPIO);
  resp += String(tmp) + "}";
  return resp;
}

String TGate::getState() const {
  char tmp[255];
  String resp = "{" + TDevice::getState();
  String t = R"(,"isOpen":%s)";

  sprintf(tmp, t.c_str(), b2s(getOpen()).c_str());
  resp += String(tmp) + "}";
  return resp;
}

void TGate::setOpen(bool openState) {
  if (_isRemote) {
    _isOpen = openState;
    unsigned long now = millis();
    if (now < _lastChange) { _lastChange = now - 501; }
    if (millis() - _lastChange > 500) {
      if (_remoteHost == "") {
        // remote address is defined by IP address
        controlRemoteGate(_remoteIp, openState ? "on" : "off");
      }
      else {
        controlRemoteGate(_remoteHost, openState ? "on" : "off");
      }
      _lastChange = millis();
    }
  }
  else {
    if ((_lastChange > 0) && (_isOpen == openState)) { return; }
    _isOpen = openState;
    digitalWrite(_GPIO, _isOpen ? _openState : _closedState);
    _lastChange = millis();
  }
}

TThermostat::TThermostat(String id, String name)
:TDevice(id, DEV_THERMOSTAT, name) {
  _needsHeating = false;
  // default configuration for auto mode
  _autoPrg = "p2";
  // 2 program, day and night
  _p2H0 = 530; // start time program1
  _p2T0 = 19; // temperature program1
  _p2H1 = 2200; // start time program2
  _p2T1 = 18; // temperature program2
  // default configuration for manual mode
  _manPrg = "day";
  _dayT = 0;
  _nightT = 0;
  _sensibiltity = 0.5;
//  _jsonSize = JSON_ARRAY_SIZE(1) + 2 * JSON_OBJECT_SIZE(5) + 100;
  _jsonSize = 1100;
  _enabled = true;
  _thermometer = TThermometer();
  _gate = TGate();
}

void TThermostat::parseConfigData(JsonDocument &doc) {
#ifdef DEBUG
  Serial.println("TThermostat::parseConfigData");
#endif
//  _dayT = doc["dayT"].as<float>();
//  _nightT = doc["nightT"].as<float>();
//  _mode = doc["mode"] | "man"; // off, auto, man (manual)
//  _prg = doc["prg"] | "day"; // currently running program: day, night, ...
  //set default mode
  _mode = "man";
  _prg = _manPrg;
  // load auto programs
  // load manual programs
  _dayT = doc["mode"]["man"]["dayT"].as<float>();
  _nightT = doc["mode"]["man"]["nightT"].as<float>();
/*  JsonObject obj = doc.as<JsonObject>();
  _dayT = obj["dayT"].as<int>();
  _nightT = obj["nightT"].as<int>();
  for (JsonPair p: obj) {
    Serial.printf("key: %s, ", p.key()); // is a JsonString
    if (p.value().is<int>()) {
      Serial.printf("value: %u\n", p.value().as<int>()); // is a JsonVariant
    }
    else if (p.value().is<float>()) {
      Serial.printf("value: %0.2f\n", p.value().as<float>()); // is a JsonVariant
    }
  }*/
}

bool TThermostat::storeConfig() {
  if (!_changed) { return true; }
  return true;
}

/**
 * Convert an integer value to a time representation, like 530 => 5:30
 *
 * @param {int} iHour
 * @returns {String}
 */
String valToHour(int iHour) {
  int whole = iHour / 100;
  int rem = iHour % 100;
  return String(whole) + ":" + String(rem);
}

String TThermostat::getConfig() const {
  char tmp[500];
  String resp = "{" + TDevice::getConfig();
  String t =
R"(,"sensor":[%s],"mode":{"off":{},"auto":{"prg":"%s","p2":[{"hour":"%s","temp":%0.1f},{"hour":"%s","temp":%0.1f}],"week":{}},"man":{"prg":"%s","dayT":%0.1f,"nightT":%0.1f}})";

  sprintf(tmp, t.c_str(), _thermometer.getConfig().c_str(),
    _autoPrg, valToHour(_p2H0).c_str(), _p2T0, valToHour(_p2H1).c_str(), _p2T1, // mode auto, 2 program
    _manPrg, _dayT, _nightT // mode man
    );
  resp += String(tmp) + "}";
  return resp;
}

String TThermostat::getState() const {
  char tmp[255];
  String resp = "{" + TDevice::getState();
  String t =
R"(,"mode":"%s","needsHeating":%s,"enabled":%s,"program":"%s","sensor":[%s])";

  sprintf(tmp, t.c_str(), _mode.c_str(), b2s(_needsHeating).c_str(), b2s(_enabled).c_str(),
    _prg.c_str(), _thermometer.getState().c_str());
  resp += String(tmp) + "}";
  return resp;
}

bool TThermostat::setMode(String mode, String prg) {
  String oldMode = _mode;
  if (mode == "off" || mode == "auto" || mode == "man") {
    _mode = mode;
    if (mode == "off") { _prg = ""; } else
    if (mode == "auto") {
      if (prg == "p2") { _prg = prg; }
      else {
        _mode = oldMode;
        return false;
      }
      _autoPrg = prg; // save selected mode
    } else
    if (mode == "man") {
      if (prg == "day" || prg == "night") { _prg = prg; }
      else {
        _mode = oldMode;
        return false;
      }
      _manPrg = prg; // save selected mode
    }
    _changed = true;
    return true;
  }
  return false;
}

bool TThermostat::setProgram(String prg) {
  if (_mode == "auto") {
    if (prg == "p2") {
      _prg = prg;
      _autoPrg = prg; // save selected mode
      _changed = true;
      return true;
    }
  } else
  if (_mode == "man") {
    if (prg == "day" || prg == "night") {
      _prg = prg;
      _manPrg = prg; // save selected mode
      _changed = true;
      return true;
    }
  }
  return false;
}

bool TThermostat::setTemperature(float temperature, String ttype) {
  if (ttype == "day") {
    _dayT = temperature;
    _changed = true;
    return true;
  } else if (ttype == "night") {
    _nightT = temperature;
    _changed = true;
    return true;
  } else if (ttype == "p2T0") {
    _p2T0 = temperature;
    _changed = true;
    return true;
  } else if (ttype == "p2T1") {
    _p2T1 = temperature;
    _changed = true;
    return true;
  }
  return false;
}

/**
 * The reference temperature is used to detect if the
 *   furnace should be turned on or off
 */
float TThermostat::getRefTemperature() {
  float refT;
  if (_mode.equalsIgnoreCase("auto")) {
    refT = 0;
  } else
  if (_mode.equalsIgnoreCase("man")) {
    refT = _prg == "day" ? _dayT : _nightT;
  }
  return refT;
}

void TThermostat::detectChanges(String reportedState) {
  bool gateIsOpen = reportedState == "on"; // the gate commanded by the thermostat is open?
//  bool saveNeedsHeating = _needsHeating;
  if (!_enabled || _mode.equalsIgnoreCase("off")) {
    _needsHeating = false; // will command close gate
  }
  else {
    float t = _thermometer.getTemperature();
    if (t > 0) {
      // the reference temperature depends on the current program
      float refT = getRefTemperature();
//      if (_mode.equalsIgnoreCase("auto")) { return; } else
//      if (_mode.equalsIgnoreCase("man")) {
        // the temperature used as a reference
//        float ct = _prg == "day" ? _dayT : _nightT;
        if (t > refT + _sensibiltity) {
          _needsHeating = false;
        } else
        if (t < refT - _sensibiltity) {
          _needsHeating = true;
        }
//      }
    }
  }
//  if (saveNeedsHeating != _needsHeating) {
  if (gateIsOpen != _needsHeating) { // send command if state differ
    _gate.setOpen(_needsHeating);
  }
}

TWoodFurnace::TWoodFurnace(String id, String name/*, uint8_t pumpGPIO*/)
:TDevice(id, DEV_FURNACE, name) {
  _mode = "off";
  _onT = 0;
  _isCooling = false;
  _jsonSize = JSON_ARRAY_SIZE(2) + JSON_OBJECT_SIZE(2) + JSON_OBJECT_SIZE(3) + JSON_OBJECT_SIZE(5) + 100;
  _aliveT = 35;
  _thermometer = TThermometer();
  _pump = TGate();
}

/**
 * Import configuration
 *
 * @param {JsonDocument} doc -
 * @returns {void}
 */
void TWoodFurnace::parseConfigData(JsonDocument &doc) {
  _onT = doc["onT"].as<int>();
  _mode = doc["mode"] | "off";
}

bool TWoodFurnace::storeConfig() {
  return true;
}

String TWoodFurnace::getConfig() const {
  char tmp[255];
  String resp = "{" + TDevice::getConfig();
  String t =
R"(,"onT":%u,"sensor":[%s,%s])";

  sprintf(tmp, t.c_str(), _onT, _thermometer.getConfig().c_str(), _pump.getConfig().c_str());
  resp += String(tmp) + "}";
  return resp;
}

String TWoodFurnace::getState() const {
  char tmp[255];
  String resp = "{" + TDevice::getState();
  String t =
R"(,"mode":"%s","sensor":[%s,%s])";

  sprintf(tmp, t.c_str(), _mode.c_str(), _thermometer.getState().c_str(), _pump.getState().c_str());
  resp += String(tmp) + "}";
  return resp;
}

bool TWoodFurnace::setMode(String mode) {
  if (mode != "off" && mode != "on") { return false; }
  // do not let switching off if the measured temperature is above of certain level
  if (mode == "off" && _thermometer.getTemperature() >= _aliveT) { return false; }
  if (mode == "off") { _pump.setOpen(false); }
  _mode = mode;
  _changed = true;
  return true;
}

bool TWoodFurnace::setTemperature(int temperature, String ttype) {
  if (ttype == "on") {
    _onT = temperature;
    if (_onT == _aliveT) { _isCooling = true; } // put the furnace into the cooling state
    _changed = true;
    return true;
  }
  return false;
}

void TWoodFurnace::detectChanges() {
  float t = _thermometer.getTemperature();
  // the wood furnace becomes alive when the water temperature gets bigger than "aliveT" (default 35)
  if (t >= _aliveT && _mode == "off") { _mode = "on"; }
  // switch on or off the water pump based on the measured temperature
  bool setOn = t >= _onT;
  _pump.setOpen(setOn);
  if (!setOn && _isCooling) {
    // if the pump was switched off while the furnace was cooling,
    //   get out from the cooling operation and return to the normal state
    _onT = 44;
    _isCooling = false;
  }
}
