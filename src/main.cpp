#include <WebServer.h>
//#include <ESPmDNS.h>
#include <ArduinoOTA.h>
#include "FS.h"
#include "SPIFFS.h"
#include <Ticker.h>

#include "ds18b20_utils.h"
#include "config.h"
#include <device.h>
#include "utils.h"

// cd /home/huba/data/work/svnroot/zyra/trunk/arduino/sketchbook/deakzoli
// arduino-cli compile --fqbn esp32:esp32:esp32 --libraries "/home/huba/data/work/svnroot/zyra/trunk/arduino/libraries/" dzheat_esp32/
// arduino-cli upload -p /dev/ttyUSB0 --fqbn esp32:esp32:esp32 dzheat_esp32/

extern const char* device;
extern const char* deviceId;
extern const char* ver;
extern const char* SSID;
extern const char* password;
extern const char* logAuth;
unsigned long lastWiFiConnectTrial;
WebServer webServer(80);

bool canReadTemperature = false;
// create a one wire instance to communicate with any OneWire device
OneWire oneWire(ONE_WIRE_BUS);
// pass our one wire reference to Dallas Temperature
DallasTemperature dtSensors(&oneWire);
Ticker tmrReadTemperature; // timer used to control temperature reading frequency
extern TTemperatureSensor temperatureSensors[];

// connected devices
TGate wfp(PIN_WFP); // wood furnace water pump
TGate wbp(PIN_WBP); // water buffer pump
TGate gf(PIN_GF); // gas furnace
TGate tap1(PIN_T1); // tap 1
TGate tap2(PIN_T2); // tap 2


// heating system attributes
uint8_t hsState = HS_NONE;
const char* hsMode = "auto";
bool heatingTheHouse = false; // based on the signal coming from the thermostat
unsigned long lastStateChange = 0;
extern const char* stateNames[];

// error handling
Ticker tmrError; // timer used to indicate error severity
uint8_t errorLevel = ERR_NO_ERROR;
bool ledIsOn = false;
bool sendRemoteLog = false;

// simulation routine
#define SIMULATION
#ifdef SIMULATION
#include "simulation.h"

extern int simulationStep;
extern bool simulationRunning;

void beginSimulation() {
  startSimulation();
  webServer.send(200, "text/html", "ok");
}
#endif

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
  unsigned long startRead = millis();
#ifdef SIMULATION
  if (simulationRunning) simulationStepNext(); else {
#endif
  if (dtSensors.getDeviceCount() < temperatureSensorsCount) {
    dtSensors.begin();
    delay(500);
    enumTemperatureSensors(dtSensors);
    sendRemoteLog = false;
  }
  else {
    DeviceAddress deviceAddress; // temperature sensor address
    dtSensors.requestTemperatures(); // read the temperatures
    for (uint8_t i = 0; i < dtSensors.getDeviceCount(); i++) {
      if (dtSensors.getAddress(deviceAddress, i)) {
        float temperature = dtSensors.getTempC(deviceAddress);
        if (temperature != DEVICE_DISCONNECTED_C && temperature != 85.00) {
          storeTemperature(deviceAddress, temperature);
        }
      }
    }
    sendRemoteLog = true;
  }
#ifdef SIMULATION
  }
#endif
  canReadTemperature = false;
  unsigned long endRead = millis();
#ifdef DEBUG
  for (uint8_t i = 0; i < temperatureSensorsCount; i++) {
    Serial.print(temperatureSensors[i].measuredValue);
    Serial.print(" ");
  }
//  Serial.println();
  if (endRead > startRead) {
    Serial.printf(", read operation took %u milliseconds [%u - %u]\n", (endRead - startRead), startRead, endRead);
  }
#endif
}

//*******************
// Heating system operations
//*******************
/**
 * Put the 3-way taps into the default position
 */
void M1() {
  tap1.setOpen(false);
  delay(RELAY_DELAY);
  tap2.setOpen(false);
  delay(RELAY_DELAY);
}

/**
 * Put the 3-way taps into the other position
 */
void M2() {
  tap1.setOpen(true);
  delay(RELAY_DELAY);
  tap2.setOpen(true);
  delay(RELAY_DELAY);
}

/**
 * Change the state of the heating system
 */
// void setHsState(uint8_t newState) {
//   if (newState == hsState) return;
//   int stateChangeDiff = millis() - lastStateChange;
//   if (stateChangeDiff < 0) stateChangeDiff = -stateChangeDiff;
//   if (stateChangeDiff < 1000) return;
//   switch (newState) {
//   case HS_PAUSED:
//     // kind of switch off, keep taps position, but stop pumps and gas furnace
//     // different behavior based on the old state
//     switch (hsState) {
//     case HS_GAS_HOUSE:
//       gf.setOpen(false); // switch off heating from the gas furnace
//       break;
//     case HS_WOOD_HOUSE: // switch off heating from the wood furnace
//       if (heatingTheHouse && (T(iTWF) > TMin(iTWF) - TDELTA)) return;
//       wfp.setOpen(false);
//       break;
//     case HS_BUFFER_HOUSE: // switch off heating from the buffer
//       if (heatingTheHouse && (T(iTBT) > TMin(iTBT) - TDELTA)) return;
//       wbp.setOpen(false);
//       break;
//     case HS_WOOD_BUFFER: // switch off heating from the wood furnace
//       if (T(iTWF) > (TMin(iTWF) - TDELTA) && T(iTWF) > T(iTBB)) return;
//       wfp.setOpen(false);
//       break;
//     }
//     break;
//   case HS_WOOD_HOUSE:
//     M1();
//     wfp.setOpen(true);
//     delay(RELAY_DELAY);
//     wbp.setOpen(false);
//     delay(RELAY_DELAY);
//     gf.setOpen(false);
//     break;
//   case HS_BUFFER_HOUSE:
//     M2();
//     wfp.setOpen(false);
//     delay(RELAY_DELAY);
//     wbp.setOpen(true);
//     delay(RELAY_DELAY);
//     gf.setOpen(false);
//     break;
//   case HS_GAS_HOUSE:
//     if (hsState == HS_BUFFER_HOUSE && heatingTheHouse && (T(iTBT) > TMin(iTBT) - TDELTA)) return;
//     M1();
//     wfp.setOpen(false);
//     delay(RELAY_DELAY);
//     wbp.setOpen(false);
//     delay(RELAY_DELAY);
//     gf.setOpen(true);
//     break;
//   case HS_WOOD_BUFFER:
//     M2();
//     wfp.setOpen(true);
//     delay(RELAY_DELAY);
//     wbp.setOpen(false);
//     delay(RELAY_DELAY);
//     gf.setOpen(false);
//     break;
//   default: // HS_OFF
//     // close all gates
//     newState = HS_OFF;
//     wfp.setOpen(false);
//     wbp.setOpen(false);
//     gf.setOpen(false);
//     tap1.setOpen(false);
//     tap2.setOpen(false);
//     break;
//   }

// #ifdef DEBUG
//   Serial.print("old state: ");
//   Serial.print(stateNames[hsState]);
//   Serial.print(", new state: ");
//   Serial.println(stateNames[newState]);
// #endif
//   hsState = newState;
//   lastStateChange = millis();
// }
void setHsState(uint8_t newState) {
  if (newState == hsState) return;
  int stateChangeDiff = millis() - lastStateChange;
  if (stateChangeDiff < 0) stateChangeDiff = -stateChangeDiff;
  if (stateChangeDiff < 1000) return;
  switch (newState) {
  case HS_PAUSED:
    // kind of switch off, keep taps position, but stop pumps and gas furnace
    // different behavior based on the old state
    switch (hsState) {
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
    wfp.setOpen(true);
    delay(RELAY_DELAY);
    wbp.setOpen(false);
    delay(RELAY_DELAY);
    gf.setOpen(false);
    break;
  case HS_BUFFER_HOUSE:
    M2();
    wfp.setOpen(false);
    delay(RELAY_DELAY);
    wbp.setOpen(true);
    delay(RELAY_DELAY);
    gf.setOpen(false);
    break;
  case HS_GAS_HOUSE:
    // if (hsState == HS_BUFFER_HOUSE && heatingTheHouse && (T(iTBT) > TMin(iTBT) - TDELTA)) return;
    M1();
    wfp.setOpen(false);
    delay(RELAY_DELAY);
    wbp.setOpen(false);
    delay(RELAY_DELAY);
    gf.setOpen(true);
    break;
  case HS_WOOD_BUFFER:
    M2();
    wfp.setOpen(true);
    delay(RELAY_DELAY);
    wbp.setOpen(false);
    delay(RELAY_DELAY);
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
  Serial.print("old state: ");
  Serial.print(stateNames[hsState]);
  Serial.print(", new state: ");
  Serial.println(stateNames[newState]);
#endif
  hsState = newState;
  lastStateChange = millis();
}

//*******************
// HTML communication
//*******************

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
}

/**
 * Return information of one temperature sensor
 *
 * @param {int} idx
 * @returns {String}
 */
String getTempSensorState(int idx) {
  char tmp[255];
  String t = R"({"type":%u,"name":"%s","t":%0.2f,"tMin":%0.2f,"tMax":%0.2f})";
  sprintf(tmp, t.c_str(), SE_TEMPERATURE, temperatureSensors[idx].name, temperatureSensors[idx].measuredValue,
    temperatureSensors[idx].minValue, temperatureSensors[idx].maxValue);
  return String(tmp);
}

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
  resp += getDbVersion();
  resp += "," + getSystemInfo();
  for (int i = 0; i < temperatureSensorsCount; i++) {
    resp += "," + getTempSensorState(i);
  }
  resp += "," + getThermostatState();
  resp += "," + tap1.getState();
  resp += "," + tap2.getState();
  resp += "," + wfp.getState();
  resp += "," + wbp.getState();
  resp += "," + gf.getState();
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
}

/**
 * Returns the corresponding content type of the
 *   extension.
 *
 * @param {String} ext - the extension
 * @returns {String} -
*/
String contentType(String ext) {
	if (ext.equalsIgnoreCase("ico")) { return "image/x-icon"; } else
	if (ext.equalsIgnoreCase("html")) { return "text/html"; } else
	if (ext.equalsIgnoreCase("js")) { return "application/javascript"; } else
	if (ext.equalsIgnoreCase("css")) { return "text/css; charset=utf-8"; } else
	if (ext.equalsIgnoreCase("woff2")) { return "application/x-font-woff2"; } else
	{ return ""; }
}

/**
 * Load a file from the file system and return its content as a
 *   response to a request.
*/
void sendFile(String fName) {
	if (!SPIFFS.exists("/" + fName)) { handleNotFound(); }
	File f = SPIFFS.open("/" + fName, "r");
	if (!f) { handleNotFound(); }
	// split file name into name and extension
	int n = fName.lastIndexOf(".");
	String ext = fName.substring(n + 1);
	webServer.streamFile(f, contentType(ext));
}

/**
 * A lambda expression stored in a function pointer type variable
*/
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
}

void connectToWiFi(bool inSetup) {
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
    Serial.println("connected to " + String(SSID) + "\nIP address is: " + WiFi.localIP().toString());
#endif
/*    if (inSetup) {
      bool mDNSReady = MDNS.begin(device); // start the mDNS responder
#ifdef DEBUG
      if (mDNSReady) {
        Serial.println("mDNS: " + String(device));
      }
      else {
        Serial.println("Error setting up MDNS responder!");
      }
#endif
    }*/
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
  if (SPIFFS.begin()) {
    displayLoadingStep("SPIFFS is activated");
  }
  else {
    displayLoadingStep("unable to activate SPIFFS");
  }
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
  if (newLevel != errorLevel) {
#ifdef DEBUG
    Serial.printf("set error state to %d\n", newLevel);
#endif
    tmrError.detach();
    switch (newLevel) {
      case ERR_NO_ERROR:
        // LED is on when heating the house
        if (!heatingTheHouse) { toggleLed("off"); }
        break;
      case ERR_LOW:
        tmrError.attach(FREQ_LOW, toggleLed);
        break;
      default:
        tmrError.attach(FREQ_HIGH, toggleLed);
        break;
    }
    errorLevel = newLevel;
//    tmrError.attach(freqError, showErrorLevel);
    handleCriticalState();
  }
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

void setup() {
  pinMode(PIN_LED, OUTPUT);
  toggleLed("off");

  pinMode(PIN_THERMOSTAT, INPUT_PULLUP);

  // gate type controls final configuration
  wfp.setName("fás kazán vízpumpa");
  wbp.setName("puffer vízpumpa");
  gf.setType(SE_GAS_FURNACE);
  gf.setName("gáz kazán");
  tap1.setType(SE_TWO_STATE_TAP);
  tap1.setName("csap1");
  tap2.setType(SE_TWO_STATE_TAP);
  tap2.setName("csap2");

#ifdef DEBUG
  Serial.begin(115200);
  delay(1000);
  Serial.println();
#else
  delay(1500);
#endif
  setHsState(HS_OFF);

  connectToWiFi(true);
  dtSensors.begin(); // start temperature sensors library
#ifdef DEBUG
  Serial.println(enumTemperatureSensors(dtSensors).c_str()); // enumerate the accessible temperature sensors
#endif
  initFS(); // initialize the file system
  setupWebServer(); // initialize the web server
  ArduinoOTA.begin(); // initialize OTA

  setupTimers();
}

void manageSystem() {
//  if (detectError()) return;

#ifdef SIMULATION
  if (!simulationRunning)
#endif
  heatingTheHouse = digitalRead(PIN_THERMOSTAT) == LOW;

  if (hsMode == "auto") { // automatic command mode
    // turn on/off the LED if there is no error reported
    if (errorLevel == ERR_NO_ERROR) { toggleLed(heatingTheHouse ? "on" : "off"); }
    if (heatingTheHouse) {
      if (T(iTWF) > TMin(iTWF) + TDELTA){
        setHsState(HS_WOOD_HOUSE); // heating from the wood furnace
      }
      if (hsState == HS_WOOD_HOUSE && T(iTWF) < TMin(iTWF) - TDELTA) {
        hsState = HS_PAUSED; // switch off heating from the wood furnace
      }
      if (T(iTBT) > TMin(iTBT) + TDELTA) {
        setHsState(HS_BUFFER_HOUSE); // heating from the water buffer
      }
      if (hsState == HS_BUFFER_HOUSE && T(iTBT) < TMin(iTBT) - TDELTA) {
        hsState = HS_PAUSED; // switch off heating from the water buffer
      }
      if (hsState != HS_WOOD_HOUSE || hsState != HS_BUFFER_HOUSE) {
        setHsState(HS_GAS_HOUSE); // heating from the gas furnace
      }
      // if (T(iTWF) > TMin(iTWF) + TDELTA) {
      //   setHsState(HS_WOOD_HOUSE); // heating from the wood furnace
      // }
      // else {
      //   if (T(iTBT) > TMin(iTBT) + TDELTA) {
      //     setHsState(HS_BUFFER_HOUSE); // heating from the water buffer
      //   }
      //   else {
      //     setHsState(HS_GAS_HOUSE); // heating from the gas furnace
      //   }
      // }
    }
    else {
      if (T(iTWF) > T(iTBB)) {
      // if (T(iTWF) > (TMin(iTWF) + TDELTA) && T(iTWF) > T(iTBB)) {
        // heating the water buffer from the wood furnace while it's temperature
        // is greater than the water buffer's temperature measured at the bottom (or middle)
        setHsState(HS_WOOD_BUFFER);
      }
      else {
        // kind of switch off
        setHsState(HS_PAUSED); // keep taps position, but stop water pumps and gas furnace
      }
    }
  }
  else { // manual command mode, nothing allowed yet
    if (hsState == HS_WOOD_HOUSE && T(iTWF) < TMin(iTWF)) {
      setHsState(HS_PAUSED);
    } else
    if (hsState == HS_BUFFER_HOUSE && T(iTBT) < TMin(iTBT)) {
      setHsState(HS_PAUSED);
    } else
    if (hsState == HS_WOOD_BUFFER) {
      if (T(iTWF) < TMin(iTWF) || T(iTWF) < T(iTBB)) {
        setHsState(HS_PAUSED);
      }
    }
  }

  detectError();
}

void loop() {
  if (!WiFi.isConnected()) {
    if (millis() - lastWiFiConnectTrial > 30 * 1000) {
      connectToWiFi(false);
    }
  }
  // HTTP server listens to requests
  webServer.handleClient();
  // listen if there is any request to upload code over the air
  ArduinoOTA.handle();

  readTemperatures();
  manageSystem();
//   if (sendRemoteLog) {
// //    if (remoteLog(1, getAllState(), logAuth, deviceId)) Serial.println("log sent");
//     remoteLog(1, getAllState(), logAuth, deviceId);
//     sendRemoteLog = false;
//   }
}
