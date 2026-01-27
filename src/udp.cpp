#include "udp.h"
#if defined(ESP32)
	#include "AsyncUDP.h"
#elif defined(ESP8266)
	// #include "ESPAsyncUDp.h"
	#include "WiFiUdp.h"
#endif

bool sendLog = false; // send UDP log messages
WiFiUDP udpListener;

void broadcastUDPMessage(String ip, String message, uint16_t port) {
	IPAddress ipBroadcast;
	if (!ipBroadcast.fromString(ip)) { return; }
	broadcastUDPMessage(ipBroadcast, message, port);
}

void broadcastUDPMessage(IPAddress ip, String message, uint16_t port) {
	if (!WiFi.isConnected()) { return; }
#if defined(ESP32)
	AsyncUDP udp;
	// IPAddress ipBroadcast(192, 168, 1, 255);
	// udp.connect(ipBroadcast, 54325);
	udp.broadcastTo(message.c_str(), port);
#elif defined(ESP8266)
	// udp.connect(ipBroadcast, 54325);
	WiFiUDP udp;
	udp.beginMulticast(WiFi.localIP(), ip, port);
	// udp.printf(message.c_str());
	udp.write(message.c_str());
	udp.endPacket();
#endif
}

void broadcastUDPMessage(String message) {
	if (!WiFi.isConnected()) { return; }
	IPAddress ipLocal = WiFi.localIP();
	IPAddress mask = WiFi.subnetMask();
	IPAddress ipBroadcast((uint32_t)ipLocal | ~((uint32_t)mask));
	broadcastUDPMessage(ipBroadcast, message, UDP_PORT_MESSAGE);
}

void enableLogging(bool enable) {
	sendLog = enable;
}

/**
 * Check for incoming UDP request
 * Accept only messages starting with the required header
 */
String detectUDPRequest(String header) {
	if (!WiFi.isConnected()) { return ""; }
	int headerLen = header.length();
	int packetSize = 0;
	while ((packetSize = udpListener.parsePacket()) > 0) {
		char tmpMsg[256];
		int toRead = (packetSize < 255) ? packetSize : 255;
		int len = udpListener.read(tmpMsg, toRead);
		if (len <= 0) { continue; }
		tmpMsg[len] = 0;
		if (headerLen > 0) {
			if (len < headerLen || memcmp(tmpMsg, header.c_str(), headerLen) != 0) {
				continue;
			}
		}
		return String(tmpMsg);
	}
	return "";
}

/**
 * Return the value of a parameter from the request string
 */
String getUDPParam(String message, String param) {
	int nextPosition = 0;
	int i = message.indexOf("&" + param); // locate the position of the parameter name in the message
	String ret = "";
	if (i >= 0) {
		i += param.length() + 2; // skip the first '&', the parameter name, and the '=' on the end
		nextPosition = message.indexOf('&', i); // locate the start position of the next parameter
		// copy the value of the parameter
		if (nextPosition >= 0) {
			ret = message.substring(i, nextPosition);
		} else {
			ret = message.substring(i);
		}
	}
	return ret;
}

String getUDPParam(ZList<TUDPParam> &params, String name) {
	ZList<TUDPParam>::Iterator iterator = params.getIterator();
	while (iterator.hasNext()) {
		TUDPParam param = iterator.getElement(); // get current item
		if (param.name == name) {
			return param.value;
		}
		iterator.next();
	}
	return "";
}

TUDPParam breakPart(String part, bool isHeader) {
	TUDPParam param;
	if (isHeader) {
		param.name = "header";
		param.value = part;
	} else {
		int t = part.indexOf('=');
		param.name = part.substring(0, t);
		param.value = part.substring(t + 1);
	}
	return param;
}

ZList<TUDPParam> parseUDPMessage(String message) {
	ZList<TUDPParam> params;
	int lastPosition = 0, nextPosition, idxFrom, idxTo, l = message.length();
	do {
		nextPosition = message.indexOf('&', lastPosition);
		idxFrom = lastPosition;
		if (nextPosition >= 0) {
			idxTo = nextPosition;
			lastPosition = nextPosition + 1; // skip the coming '&'
		} else {
			// there are no (more) '&'-s
			idxTo = l;
			lastPosition = l;
		}
		String part = message.substring(idxFrom, idxTo);
		if (part.length() > 0) {
			params.push(breakPart(part, idxFrom == 0));
		}
	} while (lastPosition < l);

	return params;
}

void udpListen(uint16_t port) {
	uint8_t state = udpListener.begin(port);
	if (state == 0) { Serial.println("udp listener failed to start"); }
}

void logMessage(String message) {
	if (!sendLog) { return; }
	broadcastUDPMessage(message);
}

void sendUDPMessage(String ip, String message, uint16_t port) {
	IPAddress ipRemote;
	if (!ipRemote.fromString(ip)) { return; }
	sendUDPMessage(ipRemote, message, port);
}

void sendUDPMessage(IPAddress ip, String message, uint16_t port) {
  	if (!WiFi.isConnected()) { return; }
	WiFiUDP udpSender;
	udpSender.beginPacket(ip, port);
	udpSender.printf(message.c_str());
	udpSender.endPacket();
	// AsyncUDP udp;
	// udp.writeTo()
//  udp.broadcastTo(message.c_str(), port);
}
