#include <WebServer.h>
#include <ArduinoOTA.h>
#include <LittleFS.h>
#include <Ticker.h>

#include "ds18b20_utils.h"
#include "config.h"
#include "utils.h"
#include "udp.h"
#include "httpreq.h"
#include "heatctrl.h"

// cd /home/huba/data/work/svnroot/zyra/trunk/arduino/sketchbook/deakzoli
// arduino-cli compile --fqbn esp32:esp32:esp32 --libraries "/home/huba/data/work/svnroot/zyra/trunk/arduino/libraries/" dzheat_esp32/
// arduino-cli upload -p /dev/ttyUSB0 --fqbn esp32:esp32:esp32 dzheat_esp32/

extern const char* device;
extern const char* deviceId;
extern const char* ver;
extern const char* SSID;
extern const char* password;

extern const char* MONITOR_URL;
extern const char* ROOT_CA;
unsigned long lastWiFiConnectTrial;
// WebServer webServer(80);
unsigned long lastAlivePublished;

bool canReadTemperature = false;
// create a one wire instance to communicate with any OneWire device
OneWire oneWire(ONE_WIRE_BUS);
// pass our one wire reference to Dallas Temperature
DallasTemperature dtSensors(&oneWire);
Ticker tmrReadTemperature; // timer used to control temperature reading frequency
extern TTemperatureSensor temperatureSensors[];
static bool tempConversionPending = false;
static unsigned long tempConversionStartMs = 0;
static unsigned long tempConversionWaitMs = 750;

THeatCtrl heatCtrl("heatctrl", "Vezérlő", "93d50461-cd88-4761-9d2c-cacb4086c8ca");

// error handling
#define FREQ_LOW 2
#define FREQ_HIGH 1
Ticker tmrError; // timer used to indicate error severity
bool ledIsOn = false;

// simulation routine
uint8_t simulationRead = 0; // used to count temperature read skips when simulation is running

void connectToWiFi();
bool detectError();
void doCommunication();
void handleUDPRequests();
void initDevices();
void initFS();
void readTemperatures();
// void setupWebServer();
void setupTemperatureSensors();
void setupTimers();
void toggleLed(String newState);

void setup() {
  pinMode(PIN_LED, OUTPUT);
  toggleLed("off");

  pinMode(PIN_THERMOSTAT, INPUT_PULLUP);
	pinMode(PIN_WFP, OUTPUT); // wood furnace water pump
	pinMode(PIN_WBP, OUTPUT); // water buffer pump
	pinMode(PIN_GF, OUTPUT); // gas furnace
	pinMode(PIN_T1, OUTPUT); // tap 1
	pinMode(PIN_T2, OUTPUT); // tap 2

#ifdef DEBUG
  Serial.begin(115200);
  delay(1000);
  Serial.println();
#else
  delay(1500);
#endif
  initFS(); // initialize the file system
  setupTemperatureSensors();
  initDevices();

  connectToWiFi();
  // setupWebServer(); // initialize the web server
  ArduinoOTA.begin(); // initialize OTA

  setupTimers();
	udpListen();
}

void loop() {
  if (!WiFi.isConnected()) {
    if (millis() - lastWiFiConnectTrial > 30 * 1000) {
      connectToWiFi();
    }
  } else {
    doCommunication();

    // HTTP server listens to requests
    // webServer.handleClient();
    // listen if there is any request to upload code over the air
    ArduinoOTA.handle();
  }

  readTemperatures();
  heatCtrl.manageSystem();
  detectError();
  // turn on/off the LED if there is no error reported
  if (heatCtrl.getErrorLevel() == ERR_NO_ERROR) { toggleLed(heatCtrl.isHeatingTheHouse() ? "on" : "off"); }
	delay(3);
}

//*******************
// Display operations
//*******************

void displayLoadingStep(String msg) {
#ifdef DEBUG
  Serial.println(msg);
#endif
}

void endLoadingStep(String msg, int pause) {
#ifdef DEBUG
  Serial.println(msg);
#endif
  delay(pause);
}


//*******************
// Temperature sensors handling operations
//*******************

void setupTemperatureSensors() {
	dtSensors.begin(); // start temperature sensors library
	//  9 bit 0.5    precision,  ~94 ms conversion time
	// 10 bit 0.25   precision, ~188 ms conversion time
	// 11 bit 0.125  precision, ~375 ms conversion time
	// 12 bit 0.0625 precision, ~750 ms conversion time
	dtSensors.setResolution(11);
	dtSensors.setWaitForConversion(false);
	tempConversionWaitMs = temperatureConversionWait(dtSensors.getResolution());
#ifdef DEBUG
	Serial.println(enumTemperatureSensors(dtSensors).c_str()); // enumerate the accessible temperature sensors
#endif
}

/**
 * The routine is called by the timer to enable reading the temperature sensors
 */
void enableReadTemperature() {
  canReadTemperature = true;
}

/**
 * Read the temperature measured by the sensors
 */
void readTemperatures() {
	if (!canReadTemperature) { return; }
  if (heatCtrl.isSimulationRunning()) {
	  canReadTemperature = false;
    if (simulationRead < 3) { simulationRead++; return; }
    heatCtrl.stepSimulation(); // go to the next step
    simulationRead = 0;
    return;
  }
	unsigned long now = millis();
	uint8_t sensorsCount = dtSensors.getDeviceCount();
	if (sensorsCount < temperatureSensorsCount) {
		// enumerate temperature sensors
#ifdef DEBUG
		Serial.print("temperature sensors: ");
		Serial.println(sensorsCount);
#endif
		dtSensors.begin();
		enumTemperatureSensors(dtSensors);
	}
	if (!tempConversionPending) {
		dtSensors.requestTemperatures();
		tempConversionStartMs = now;
		tempConversionPending = true;
		return;
	}
	if (!dtSensors.isConversionComplete() && now - tempConversionStartMs < tempConversionWaitMs) {
		return;
	}
	DeviceAddress deviceAddress; // temperature sensor address
	for (uint8_t i = 0; i < sensorsCount; i++) {
		if (!dtSensors.getAddress(deviceAddress, i)) { continue; }
		float temperature = dtSensors.getTempC(deviceAddress);
		if (temperature == DEVICE_DISCONNECTED_C || temperature == 85.00) { continue; }
		storeTemperature(deviceAddress, temperature);
	}
	tempConversionPending = false;
	canReadTemperature = false;
}

//*******************
// HTML communication
//*******************
/*
void handleNotFound() {
  webServer.send(404, "text/plain", "404: Not found"); // Send HTTP status 404 (Not Found) when there's no handler for the URI in the request
}

// void htmlIndex() {
//   bool error = true;
//   String resp, s;
//   if (SPIFFS.exists("/index.html")) {
//     File f = SPIFFS.open("/index.html", "r");
//     if (f) {
//       while (f.position() < f.size()) {
//         s = f.readStringUntil('\n');
//         resp += s + "\n";
//       }
//       error = false;
//     }
//   }
//   if (error) {
//     handleNotFound();
//   }
//   else {
//     webServer.send(200, "text/html", resp.c_str());
//   }
// }

String getDbVersion() {
  char tmp[255];
  String t = R"({"type":0,"version":"%s"})";
  sprintf(tmp, t.c_str(), ver);
  return String(tmp);
}*/

/**
 * Return information of one temperature sensor
 *
 * @param {int} idx
 * @returns {String}
 * /
String getTempSensorState(int idx) {
  char tmp[255];
  String t = R"({"type":%u,"name":"%s","t":%0.2f,"tMin":%0.2f,"tMax":%0.2f})";
  sprintf(tmp, t.c_str(), SE_TEMPERATURE, temperatureSensors[idx].name, temperatureSensors[idx].measuredValue,
    temperatureSensors[idx].minValue, temperatureSensors[idx].maxValue);
  return String(tmp);
}* /

String getSystemInfo() {
  char tmp[255];
  String t = R"({"type":%u,"state":%u,"stateName":"%s","errorLevel":%u,"comment":"%s"})";
  String comment = "";
#ifdef SIMULATION
  if (simulationRunning) comment += String(simulationStep) + " szakasz";
#endif
  sprintf(tmp, t.c_str(), DEV_SYSTEM, hsState, stateNames[hsState], errorLevel, comment.c_str());
  return String(tmp);
}

String getThermostatState() {
  char tmp[255];
  String t = R"({"type":%u,"id":"","name":"termosztát","isOpen":%s})";
  sprintf(tmp, t.c_str(), SE_THERMOSTAT, b2s(heatingTheHouse));
  return String(tmp);
}

String getAllState() {
  String resp = "[";
  // resp += getDbVersion();
  // resp += "," + getSystemInfo();
  // for (int i = 0; i < temperatureSensorsCount; i++) {
  //   resp += "," + getTempSensorState(i);
  // }
  // resp += "," + getThermostatState();
  // resp += "," + tap1.getState();
  // resp += "," + tap2.getState();
  // resp += "," + wfp.getState();
  // resp += "," + wbp.getState();
  // resp += "," + gf.getState();
  resp += "]";
  return resp;
}

void reportState() {
  String resp = getAllState();
//#ifdef DEBUG
//  Serial.println(resp);
//#endif
  webServer.send(200, "text/plain", resp.c_str());
}

void getAnything() {
  String resp = "error";
  String cmd = webServer.arg("cmd");
//  if (cmd == "temperature") {
//    resp = String(temperatureSensors[iTTH].measuredValue);
//  } else
  if (cmd == "temperatures") {
    resp = enumTemperatureSensors(dtSensors);
    if (resp == "false" || (dtSensors.getDeviceCount() < temperatureSensorsCount)) {
      dtSensors.begin();
      delay(500);
      resp = enumTemperatureSensors(dtSensors);
    }
  }
  webServer.send(200, "text/html", resp.c_str());
}

void setAnything() {
  String resp = "ok";
  String cmd = webServer.arg("cmd");
  if (cmd == "state") {
    String newState = webServer.arg("state");
    if (webServer.hasArg("device")) {
      String device = webServer.arg("device");
      if (device == "wfp") wfp.setOpen(newState == "on"); else
      if (device == "wbp") wbp.setOpen(newState == "on"); else
      if (device == "gf") gf.setOpen(newState == "on"); else
      if (device == "tap1") tap1.setOpen(newState == "on"); else
      if (device == "tap2") tap2.setOpen(newState == "on");
    }
    else {
      // setHsState(uint8_t(newState.toInt()));
      setHsState(newState.toInt());
    }
  } else
  if (cmd == "reboot") {
    webServer.send(200, "text/html", resp.c_str());
    ESP.restart();
  }
  webServer.send(200, "text/html", resp.c_str());
}*/

/**
 * Returns the corresponding content type of the
 *   extension.
 *
 * @param {String} ext - the extension
 * @returns {String} -
* /
String contentType(String ext) {
	if (ext.equalsIgnoreCase("ico")) { return "image/x-icon"; } else
	if (ext.equalsIgnoreCase("html")) { return "text/html"; } else
	if (ext.equalsIgnoreCase("js")) { return "application/javascript"; } else
	if (ext.equalsIgnoreCase("css")) { return "text/css; charset=utf-8"; } else
	if (ext.equalsIgnoreCase("woff2")) { return "application/x-font-woff2"; } else
	{ return ""; }
}*/

/**
 * Load a file from the file system and return its content as a
 *   response to a request.
* /
void sendFile(String fName) {
	// if (!SPIFFS.exists("/" + fName)) { handleNotFound(); }
	// File f = SPIFFS.open("/" + fName, "r");
	// if (!f) { handleNotFound(); }
	// // split file name into name and extension
	// int n = fName.lastIndexOf(".");
	// String ext = fName.substring(n + 1);
	// webServer.streamFile(f, contentType(ext));
}*/

/**
 * A lambda expression stored in a function pointer type variable
* /
std::function<void()> sendIndex {
	[]() {
		sendFile("index.html");
	}
};

std::function<void()> sendFavicon {
	[]() {
		sendFile("favicon.ico");
	}
};

void setupWebServer() {
  displayLoadingStep("setup web server...");
  webServer.onNotFound(handleNotFound);
  webServer.on("", sendIndex);
  webServer.on("/", sendIndex);
	webServer.on("/favicon.ico", sendFavicon);
	webServer.on("/jquery-3.3.1.min.js", []() { sendFile("jquery-3.3.1.min.js"); });
	webServer.on("/bootstrap.min.js", []() { sendFile("bootstrap.min.js"); });
	webServer.on("/bootstrap.min.css", []() { sendFile("bootstrap.min.css"); });
	webServer.on("/bootstrap-slider.min.js", []() { sendFile("bootstrap-slider.min.js"); });
	webServer.on("/bootstrap-slider.min.css", []() { sendFile("bootstrap-slider.min.css"); });
	webServer.on("/font-awesome.min.css", []() { sendFile("font-awesome.min.css"); });
	webServer.on("/fonts/fontawesome-webfont.woff2", []() { sendFile("fontawesome-webfont.woff2"); });
	webServer.on("/index.js", []() { sendFile("index.js"); });
	webServer.on("/style.css", []() { sendFile("style.css"); });
//  server.on("/heating/getConfig", reportConfig);
  webServer.on("/getState", reportState);
  // for debugging
  webServer.on("/get", getAnything);
  webServer.on("/set", setAnything);
//  webServer.on("/setDbg", setDbg);
#ifdef SIMULATION
  webServer.on("/simulation", beginSimulation);
#endif

  // start the server
  webServer.begin();
  // wait 2 s for the server to start
  endLoadingStep("HTTP server started", 2000);
}*/

/**
 * Send a status message to the MQTT broker
 */
void publishStatus() {
	// send the heartbeat to the monitoring server
	JsonDocument doc = heatCtrl.getState();

	String url = String(MONITOR_URL) + "/api/device/" + deviceId + "/heartbeat";
	httpPost(url, doc, {{"X-Device-ID", deviceId}});

  // heatCtrl.tap1.publishStatus();
  // heatCtrl.tap2.publishStatus();
}

/**
 * The commands are coming from the web ui
 */
void procesDeviceCommand(TDevice &device) {
	String url = String(MONITOR_URL) + "/api/device/" + device._deviceId + "/command/next";
	String response = httpGet(url, {{"X-Device-ID", deviceId}});
	if (response == "") { return; }

	bool cmdProcessed = false;
	String processResult = "";
	JsonDocument doc;
	DeserializationError error = deserializeJson(doc, response);
	if (!error) {
		JsonDocument payloadDoc = doc["payload"];
		cmdProcessed = device.processCommand(payloadDoc, processResult);
	}

	// acknowledge the command
	url = String(MONITOR_URL) + "/api/device/" + device._deviceId + "/command/ack";
	JsonDocument ackDoc;
	ackDoc["command_id"] = doc["command_id"];
	ackDoc["success"] = cmdProcessed;
	ackDoc["result"] = processResult;
	httpPost(url, ackDoc, {{"X-Device-ID", deviceId}});
}

/**
 * Read and process commands one by one for every device
 */
void processCommands() {
  handleUDPRequests();
	procesDeviceCommand(heatCtrl);
}

void doCommunication() {
	if (millis() - lastAlivePublished < 5 * 1000) { return; }

	processCommands();
	publishStatus();
	lastAlivePublished = millis();
}

void connectToWiFi() {
  int i = 0;
  displayLoadingStep("connecting to WiFi");

//#ifdef DEBUG
//  Serial.print("WiFi status code: ");
//  Serial.println(WiFi.status());
//#endif
//  WiFi.persistent(false);
//  WiFi.setSleepMode(WIFI_NONE_SLEEP, 0);
  WiFi.mode(WIFI_STA); // mode station
  WiFi.setHostname(device);
  WiFi.config(ipHeatCtrl, ipGateway, ipSubnet, IPAddress(8, 8, 8, 8));
  WiFi.begin(SSID, password); // start connecting
  while (WiFi.status() != WL_CONNECTED && i < 30) {
    delay(500);
    i++;
#ifdef DEBUG
    Serial.print(".");
#endif
  }
#ifdef DEBUG
  Serial.println();
#endif
  if (WiFi.isConnected()) {
    WiFi.setHostname(device);
//#ifdef DEBUG
//    Serial.println();
//    Serial.print("WiFi status code: ");
//    Serial.println(WiFi.status());
//#endif
#ifdef DEBUG
		Serial.println("connected to " + String(SSID) + "\nIP address is: " + WiFi.localIP().toString() + "\nstrength: " + String(WiFi.RSSI()));
		if (!WiFi.getAutoReconnect()) { Serial.println("auto reconnect is not enabled"); }
#endif
  }
  else {
#ifdef DEBUG
    Serial.println("no network");
#endif
  }
  lastWiFiConnectTrial = millis();
}

//*******************
// File system operations
//*******************

/**
 * initialize the file system
 */
void initFS() {
	if (!LittleFS.begin()) {
#ifdef DEBUG
		Serial.println("unable to activate FS");
#endif
		return;
	}
#ifdef DEBUG
	Serial.println("FS is activated");
#endif
	// listDir(LittleFS, "/", 1);
}

//*******************
// Error handling
//*******************

void toggleLed(String newState) {
  if (newState == "") ledIsOn = !ledIsOn; else
  ledIsOn = newState == "on";
  digitalWrite(PIN_LED, ledIsOn ? HIGH: LOW);
}

void toggleLed() {
  toggleLed("");
}

void handleCriticalState() {
/*  if ((errorLevel == ERR_LOW) || (errorLevel == ERR_HIGH)) {
    if (temperatureSensors[iTFK].criticalState) { // && T(iTFK) > TMax(iTFK)
//      if (T(iTPUT) < TMax(iTPUT)
      setHsState(HS_WOOD_HOUSE);
    } else
    if (temperatureSensors[iTPUT].criticalState) { // && T(iTPUT) > TMax(iTPUT)
      setHsState(HS_BUFFER_HOUSE);
    }
  }
  else {
    setHsState(HS_PAUSED);
  }*/
}

void setErrorLevel(uint8_t newLevel) {
  if (newLevel > 2) { return; }
  if (newLevel == heatCtrl.getErrorLevel()) { return; }
#ifdef DEBUG
  if (heatCtrl.isSimulationRunning()) {
    Serial.printf("step: %u\n", heatCtrl.getStep());
  }
  Serial.printf("set error state to %d\n", newLevel);
#endif
  tmrError.detach();
  switch (newLevel) {
    case ERR_NO_ERROR:
      // LED is on when heating the house
      if (!heatCtrl.isHeatingTheHouse()) { toggleLed("off"); }
      break;
    case ERR_LOW:
      tmrError.attach(FREQ_LOW, toggleLed);
      break;
    default:
      tmrError.attach(FREQ_HIGH, toggleLed);
      break;
  }
  heatCtrl.setErrorLevel(newLevel);
  handleCriticalState();
}

bool detectError() {
  uint8_t level = ERR_NO_ERROR;
  for (int i = 0; i < temperatureSensorsCount; i++) {
    if (temperatureSensors[i].criticalState) {
      level = ERR_LOW;
    }
  }
  if (temperatureSensors[iTWF].criticalState && temperatureSensors[iTBT].criticalState) {
    level = ERR_HIGH;
  }
  setErrorLevel(level);
  return level > ERR_NO_ERROR;
}

//*******************
// Initialization and setup
//*******************

void setupTimers() {
  // temperature
  tmrReadTemperature.attach(freqReadTemperature, enableReadTemperature);
  enableReadTemperature();
}


void initDevices() {
  heatCtrl.wfp.setGPIO(PIN_WFP);
	heatCtrl.wfp.identify("wfpswitch", "fás kazán vízpumpa", "b219bea7-d910-4e40-99e8-300f3e05a41a");
	heatCtrl.wfp.loadConfig();
  heatCtrl.wbp.setGPIO(PIN_WBP);
	heatCtrl.wbp.identify("wbpswitch", "puffer vízpumpa", "4c436be8-c444-4183-b50f-de44a504c9c0");
  heatCtrl.wbp.loadConfig();
  heatCtrl.gf.setGPIO(PIN_GF);
  heatCtrl.gf.setType(SE_GAS_FURNACE);
	heatCtrl.gf.identify("gfswitch", "gáz kazán", "fa41f40d-5232-4053-9e44-ea8a3f250ab4");
  heatCtrl.gf.loadConfig();
  heatCtrl.tap1.setGPIO(PIN_T1);
  heatCtrl.tap1.setType(SE_TWO_STATE_TAP);
	heatCtrl.tap1.identify("tap1switch", "csap1", "5cbaa836-9ceb-4162-87dd-626d543f2542");
  heatCtrl.tap1.loadConfig();
  heatCtrl.tap2.setGPIO(PIN_T2);
  heatCtrl.tap2.setType(SE_TWO_STATE_TAP);
	heatCtrl.tap2.identify("tap2switch", "csap2", "c82e1f6d-2dad-4ed3-868c-3f1d290cc99f");
  heatCtrl.tap2.loadConfig();
  heatCtrl.setHsState(HS_OFF);
}

/**
 * Check for incoming UDP request
 */
void handleUDPRequests() {
	String request = detectUDPRequest("");
	if (request == "") { return; }
	// Serial.println("udp message: " + request);
  ZList<TUDPParam> params = parseUDPMessage(request);

	String cmd = getUDPParam(params, "cmd"); // what is the command, the other parameters depends on this
	if (cmd.equalsIgnoreCase("setState")) {
		String newState = getUDPParam(params, "state");
		String device = getUDPParam(params, "device");
    if (device.equalsIgnoreCase("gfswitch")) {
      if (heatCtrl.isSimulationRunning()) { return; }
      heatCtrl.setHeatingTheHouse(newState.equalsIgnoreCase("on"));
    }
	}
}

