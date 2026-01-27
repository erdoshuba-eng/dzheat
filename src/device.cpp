#include "device.h"
#include "utils.h"

// #include "FS.h"
// #include "SPIFFS.h"
#include <LittleFS.h>

#include "udp.h"
// #include "logger.h"

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
	String msg = "allDevices&cmd=setState&state=" + newState;
	sendUDPMessage(host, msg, UDP_SLAVE);
/*  String respHeader = "";
	String respContent = "";
	bool addToHeader = true;
	WiFiClient webClient;
	if (webClient.connect(host, 80)) {
		webClient.print(String("GET /set") + "?cmd=state&state=" + newState + " HTTP/1.1\r\n" +
			"Host: " + host.toString() + "\r\n" +
			"Connection: close\r\n\r\n");
		while (webClient.connected() || webClient.available()) {
			if (webClient.available()) {
				String line = webClient.readStringUntil('\n');
				line.trim();
				if (addToHeader) {
					// the content is separated from the header by an empty line
					if (line.equals("")) { addToHeader = false; }
					else {
//            Serial.print(line);
					}
				}
				else {
					if (!respContent.equals("")) { respContent += "\n"; }
					respContent += line;
				}
			}
		}
#ifdef DEBUG
//      Serial.println(respContent);
#endif
		webClient.stop();
	}
	else {
		webClient.stop();
	}

	respContent.trim();
//  if (respContent.equals("ok"))
	unsigned long tEnd = millis();
#ifdef DEBUG
//  Serial.printf("control remote gate %u ms\n", (tEnd - tStart));
#endif*/
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
	_readyState = 0; // ready
}

TDevice::TDevice(String id, uint8_t type, String type2, String name, String deviceId)
:TDevice() {
	_id = id;
	_deviceId = deviceId;
	_type = type;
	_type2 = type2;
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

JsonDocument TDevice::getState2() const {
	JsonDocument doc;
	doc["type"] = _type2;
	doc["id"] = _id;
	doc["name"] = _name;
	return doc;
}

bool TDevice::loadConfig() {
	String configFile = "/_" + _id + ".json";

	File f = LittleFS.open(configFile, "r");
	if (!f) {
		// Serial.println("Failed to open the " + configFile + " config file to read.");
		return false;
	}

	JsonDocument doc;
	DeserializationError error = deserializeJson(doc, f);
	f.close();
	if (error) {
		// Serial.println("Failed to read file content " + String(error.f_str()));
		return false;
	}
	parseConfigData(doc);
	return true;
}

bool TDevice::storeConfig() {
	if (!_changed) { return true; }
	JsonDocument doc = configData();
	if (doc.isNull()) { return true; }
	String configFile = "/_" + _id + ".json";
	File f = LittleFS.open(configFile, "w");
	if (!f) { return false; }

	size_t result = serializeJson(doc, f);
	f.close();
	_changed = false;
	return result != 0;
}

// ************** TThermometer class

TThermometer::TThermometer() {
	_type = SE_TEMPERATURE;
	_type2 = "temp";
	_minValue = -1000;
	_maxValue = 1000;
	// _measuredValue = 0;
	_tempSensor = NULL;
}

// TThermometer::TThermometer(String id, String name)
// :TThermometer() {
// 	setSensor(id, name);
// }

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
JsonDocument TThermometer::getState() const {
	return getTempSensorState(*_tempSensor);
	// char tmp[255];
	// String resp = "{" + TDevice::getState();
	// String t = R"(,"temperature":%0.1f,"critical":%s)";

	// sprintf(tmp, t.c_str(), getTemperature(), b2s(getIsCritical()).c_str());
	// resp += String(tmp) + "}";
	// return resp;
}

JsonDocument TThermometer::getState2() const {
	JsonDocument doc = TDevice::getState2();
	doc["temperature"] = getTemperature();
	doc["critical"] = getIsCritical();
	return doc;
}

// ************** TGate class

TGate::TGate()
:TDevice("", SE_WATER_PUMP, "switch", "") {
	_config.openState = LOW;
	_config.closedState = HIGH;
	// old version, through 1st optocoupler
//  _openState = HIGH;
//  _closedState = LOW;
	_config.isOpen = true; // force open state change
	_config.GPIO = 0;
	_config.isRemote = false;
	_config.ip = "";
	_config.lastChange = 0;
}

// deprecated
TGate::TGate(uint8_t GPIO)
:TGate::TGate() {
	_config.GPIO = GPIO;
	pinMode(_config.GPIO, OUTPUT);
	setOpen(false);
}

TGate::TGate(uint8_t GPIO, uint8_t openState)
:TGate::TGate() {
	_config.GPIO = GPIO;
	setOpenState(openState);
	setOpen(false);
}

void TGate::parseConfigData(JsonDocument &doc) {
	// "name":"Termosztat",
	_config.isRemote = doc["isRemote"] | false;
	_config.ip = doc["ip"] | "";
	_config.port = doc["port"].as<uint16_t>() | UDP_PORT_MESSAGE;
}

void TGate::publishStatus() {
	if (_config.isRemote) {
		setRemoteOpen(_config.isOpen);
	} else {
		digitalWrite(_config.GPIO, _config.isOpen ? _config.openState : _config.closedState);
	}
}

String TGate::getConfig() const {
	char tmp[255];
	String resp = "{" + TDevice::getConfig();
	String t = R"(,"GPIO":%u)";

	sprintf(tmp, t.c_str(), _config.GPIO);
	resp += String(tmp) + "}";
	return resp;
}

JsonDocument TGate::getState() const {
	JsonDocument doc;
	doc["id"] = _deviceId;
	doc["class"] = _type2;
	doc["data"]["isOn"] = getOpen();
	return doc;
}

JsonDocument TGate::getState2() const {
	JsonDocument doc = TDevice::getState2();
	doc["isOpen"] = _config.isOpen;
	return doc;
}

void TGate::setRemoteOpen(bool bOpen) {
	String newState = _config.isOpen ? "on" : "off";
	String msg = "allDevices&cmd=setState&state=" + newState + "&device=" + _id;
	sendUDPMessage(_config.ip, msg, _config.port);
}

void TGate::setOpen(bool bOpen, bool isForced) {
	unsigned long now = millis();
	// if (!isForced) {
	// 	if ((now > _config.lastChange) && (now - _config.lastChange < 1000)) { return; }
	// 	if (now < _config.lastChange) {
	// 		unsigned long diff = 4294967295 - _config.lastChange + now;
	// 		if (diff < 1000) { return; }
	// 	}
	// }
	// always set the state to override state changes caused by other sources
	if (_config.isOpen == bOpen) { return; }
	_config.isOpen = bOpen;
	publishStatus();
	_config.lastChange = millis();
}

/**
 * Set the open state of the relay
 *
 * @param (uint8_t) openState - LOW or HIGH
*/
void TGate::setOpenState(uint8_t openState) {
	_config.openState = openState;
	_config.closedState = !openState;
}

/**
 * id: used to access config file
 * name:
 * @param {UUID} deviceId: remote database id
 */
TThermostat::TThermostat(String id, String name, String deviceId)
:TDevice(id, DEV_THERMOSTAT, "thermostat", name, deviceId) {
	_config.enabled = true;
	_config.isOn = false;
	_config.forceOn = false;
	_thermometer = TThermometer();
	_gate = TGate();
}

void TThermostat::parseConfigData(JsonDocument &doc) {
	// "name":"Termosztat",
	_config.mode = doc["mode"] | "off"; // off, auto, man (manual)
	_config.refTemp = doc["refTemp"].as<float>();
	_config.sensibiltity = doc["sensibility"] | "050";
	_config.forceOnDuration = doc["forceOnDuration"].as<uint32_t>();
	// _verboseState = true;
}

JsonDocument TThermostat::configData() {
	JsonDocument doc;
	doc["name"] = "Termosztat";
	doc["mode"] = _config.mode;
	doc["refTemp"] = _config.refTemp;
	doc["sensibiltity"] = _config.sensibiltity;
	doc["forceOnDuration"] = _config.forceOnDuration;
	return doc;
}

JsonDocument cmdSetMode() {
	JsonDocument cmd;
	cmd["command"] = "setMode";
	cmd["label"] = "Üzemmód váltás";
	JsonArray params = cmd["params"].to<JsonArray>();
	JsonDocument param1;
	param1["name"] = "mode";
	param1["label"] = "Üzemmód";
	param1["required"] = true;
	param1["attr"] = "mode";
	// param1["type"]["type"] = "radioGroup";
	// JsonArray items = param1["type"]["items"]["data"].to<JsonArray>();
	// JsonDocument item1;
	// item1["key"] = "off";
	// item1["text"] = "Kikapcsolt";
	// items.add(item1);
	// JsonDocument item2;
	// item2["key"] = "man";
	// item2["text"] = "Kézi";
	// items.add(item2);
	// JsonDocument item3;
	// item3["key"] = "auto";
	// item3["text"] = "Automata";
	// items.add(item3);
	params.add(param1);
	return cmd;
}

JsonDocument cmdSetRefTemp() {
	JsonDocument cmd;
	cmd["command"] = "setRefTemp";
	cmd["label"] = "Munkahőmérséklet";
	JsonArray params = cmd["params"].to<JsonArray>();
	JsonDocument param1;
	param1["name"] = "temperature";
	param1["label"] = "Hőmérséklet (°C)";
	param1["required"] = true;
	param1["attr"] = "refTemp";
	params.add(param1);
	return cmd;
}

JsonDocument cmdForceOn() {
	JsonDocument cmd;
	cmd["command"] = "forceOn";
	cmd["label"] = "Bekapcsolás";
	JsonArray params = cmd["params"].to<JsonArray>();
	JsonDocument param1;
	param1["name"] = "duration";
	param1["label"] = "Időtartam (mperc)";
	param1["required"] = true;
	param1["attr"] = "forceOnDuration";
	params.add(param1);
	return cmd;
}

JsonDocument TThermostat::capabilities() {
	JsonDocument doc;
	JsonArray commands = doc.to<JsonArray>();
	commands.add(cmdSetMode());
	commands.add(cmdSetRefTemp());
	commands.add(cmdForceOn());
	return doc;
}

bool TThermostat::processCommand(JsonDocument &doc, String &response) {
	String command = doc["command"] | "";
	if (command == "capabilities") {
		JsonDocument capDoc = capabilities();
		serializeJson(capDoc, response);
		return true;
	}
	if (!_config.enabled) {
		return false;
	}
	if (command == "setMode") {
		String mode = doc["params"]["mode"] | "";
		return setMode(mode);
	} else
	if (command == "setRefTemp") {
		float temperature = doc["params"]["temperature"].as<float>();
		return setRefTemperature(temperature);
	} else
	if (command == "forceOn") {
		Serial.println("force on, duration " + doc["params"]["duration"].as<String>());
		uint32_t duration = doc["params"]["duration"].as<uint32_t>() | 20;
		return forceOn(true, duration);
	}
	return false;
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
/*
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
}*/

JsonDocument TThermostat::getState() {
	JsonDocument doc;
	doc["id"] = _deviceId;
	doc["class"] = _type2;
	bool isFirst = config.mode == "";
	// return only what changed
	if (_config.mode != config.mode) {
		doc["data"]["mode"] = _config.mode;
		config.mode = _config.mode;
	}
	if (isFirst || (_config.isOn != config.isOn)) { // boolean false values will not appear in JSON if we don't check "isFirst"
		doc["data"]["isOn"] = _config.isOn;
		config.isOn = _config.isOn;
	}
	if (isFirst || (_config.forceOn != config.forceOn)) {
		doc["data"]["forceOn"] = _config.forceOn;
		config.forceOn = _config.forceOn;
	}
	if (_config.forceOnDuration != config.forceOnDuration) {
		doc["data"]["forceOnDuration"] = _config.forceOnDuration;
		config.forceOnDuration = _config.forceOnDuration;
	}
	if (isFirst || (_config.enabled != config.enabled)) {
		doc["data"]["enabled"] = _config.enabled;
		config.enabled = _config.enabled;
	}
	if (_config.refTemp != config.refTemp) {
		doc["data"]["refTemp"] = _config.refTemp;
		config.refTemp = _config.refTemp;
	}
	if (_config.sensibiltity != config.sensibiltity) {
		doc["data"]["sensibility"] = _config.sensibiltity;
		config.sensibiltity = _config.sensibiltity;
	}
	JsonArray sensors = doc["sensors"].to<JsonArray>();
	sensors.add(_thermometer.getState());
	return doc;
}

/**
 * Rturns true if "forceOn" was changed.
 */
bool TThermostat::forceOn(bool isForced, uint32_t duration) {
	if (isForced) {
		if (_config.forceOn || !_config.enabled || _config.mode.equalsIgnoreCase("off")) {
			return false; // not changed
		}
		if (duration != 0) { _config.forceOnDuration = duration; } // seconds
		_forceOnStart = millis();
	}
	_config.forceOn = isForced;
	return true; // changed
}

bool TThermostat::setMode(String mode) {
	if (mode != "off" && mode != "auto" && mode != "man") { return false; }
	String oldMode = _config.mode;
	if (_config.mode == mode) { return true; } // no change
	_config.mode = mode;
	_changed = true;
	storeConfig();
	return true;
}
/*
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
*/
bool TThermostat::setRefTemperature(float temperature) {
	if (temperature != _config.refTemp) {
		_config.refTemp = temperature;
		_changed = true;
		storeConfig();
	}
	return true;
}

void TThermostat::adjustRefTemperature(bool up) {
	double change = _config.sensibiltity.toDouble() / 100;
	if (up) {
		_config.refTemp += change;
	} else {
		_config.refTemp -= change;
	}
	_changed = true;
	storeConfig();
}

/**
 * Detect changes in the thermometers' temperature
 *   and command the gate accordingly
 * Detect forced on state and override the automatic control
 */
bool TThermostat::detectChanges(String reportedState) {
	if (_config.forceOn) {
		// check if the forced on duration (seconds) has elapsed
		if ((millis() - _forceOnStart) >= ((unsigned long)_config.forceOnDuration * 1000UL)) {
			forceOn(false);
		}
	}
	// bool gateIsOpen = reportedState == "on"; // the gate commanded by the thermostat is open?
	bool openGate = false;
	if (!_config.enabled || _config.mode.equalsIgnoreCase("off")) {
		_config.isOn = false; // matches openGate = false, so it will not change
	} else if (_config.forceOn) {
		openGate = true;
		// _config.isOn = true;
	} else {
		float t = _thermometer.getTemperature();
		if (t > 0) {
			// the reference temperature depends on the current program
			float refT = getRefTemperature();
			if (t >= refT + getSensibility()) {
				// _config.isOn = false; // switch off if the temperature is higher than the requested
				openGate = false; // switch off if the temperature is higher than the requested
			} else
			if (t < refT - getSensibility()) {
				// _config.isOn = true;
				openGate = true;
			}
			// openGate = _config.isOn;
		}
	}
	if (_config.isOn != openGate) {
		_gate.setOpen(openGate);
		_config.isOn = openGate; // we hope the command is executed (there is no feedback)
		return true;
	}
	return false;
}
