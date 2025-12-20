#include "udp.h"
#include "AsyncUDP.h"

bool sendLog = false; // send UDP log messages

// void broadcastUDPMessage(String message) {
//   if (!WiFi.isConnected()) { return; }
//   IPAddress ipBroadcast(192, 168, 1, 255);
//   WiFiUDP udpSender;
//   udpSender.beginMulticast(ipBroadcast, 54325);
//   udpSender.printf(message.c_str());
//   udpSender.endPacket();
// }

void broadcastUDPMessage(String message) {
	if (!WiFi.isConnected()) { return; }
	AsyncUDP udp;
	// IPAddress ipBroadcast(192, 168, 1, 255);
	// udp.connect(ipBroadcast, 54325);
	udp.broadcastTo(message.c_str(), 54325);
	// WiFiUDP udpSender;
	// udpSender.beginMulticast(ipBroadcast, 54325);
	// udpSender.printf(message.c_str());
	// udpSender.endPacket();
}

void enableLogging(bool enable) {
	sendLog = enable;
}

/**
 * Check for incoming UDP request
 * Accept only messages starting with the required header
 * /
String detectUDPRequest(WiFiUDP &listener, String header) {
  String msg = "";
  if (!WiFi.isConnected()) { return msg; }
  int packetSize = listener.parsePacket();
  if (packetSize) {
	char tmpMsg[255];
	int len = listener.read(tmpMsg, 255);
	if (len > 0) {
	  tmpMsg[len] = 0;
	}
	msg = String(tmpMsg);
	// handle only requests addressed to the device
	if (!msg.startsWith(header)) return "";
  }
  return msg;
}*/

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

void logMessage(String message) {
	if (!sendLog) { return; }
	broadcastUDPMessage(message);
}

void sendUDPMessage(IPAddress host, String message, uint16_t port) {
  	if (!WiFi.isConnected()) { return; }
	WiFiUDP udpSender;
	udpSender.beginPacket(host, port);
	udpSender.printf(message.c_str());
	udpSender.endPacket();
	// AsyncUDP udp;
	// udp.writeTo()
//  udp.broadcastTo(message.c_str(), port);
}
