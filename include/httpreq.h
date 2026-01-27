/*
 * httpreq.h
 *
 *  Created on: Jan 21, 2026
 *      Author: huba
 *
 *  HTTP communication
 */

#ifndef HTTPREQ_H_
#define HTTPREQ_H_

#include <Arduino.h>
#include <ArduinoJson.h>

struct HttpHeader {
    const char* name;
    const char* value;
};

String httpGet(String url, std::initializer_list<HttpHeader> headers);
String httpGet(String url);
void httpPost(String url, JsonDocument& data, std::initializer_list<HttpHeader> headers);
void httpPost(String url, JsonDocument& data);

#endif /* HTTPREQ_H_ */
