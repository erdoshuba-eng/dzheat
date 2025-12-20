#ifndef __UDP_UTILS_H
#define __UDP_UTILS_H

// #include <Arduino.h>
#include <WiFi.h>
//#include <WiFiUdp.h>

#define UDP_MASTER 54321  // port used to send messages to master
//#define PORT_OUT 54322  // port used for outgoing messages
#define UDP_SLAVE 54323  // port used to send messages to slave

void broadcastUDPMessage(String message);
//String detectUDPRequest(WiFiUDP &listener, String header);
void enableLogging(bool enable);
String getUDPParam(String message, String param);
void logMessage(String message);
void sendUDPMessage(IPAddress host, String message, uint16_t port);

#endif
