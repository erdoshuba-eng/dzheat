
#include "httpreq.h"
#if defined(ESP32)
	#include <HTTPClient.h>
	#include "mbedtls/md.h"   // HMAC
#elif defined(ESP8266)
	#include <ESP8266HTTPClient.h>
	#include <WiFiClientSecure.h>
	#include <bearssl/bearssl_hmac.h>
	#include <bearssl/bearssl_hash.h>
#endif
#include "config.h"

extern const char *apiKey;

/**
 * HMAC-SHA256 (used for signing)
 */
bool hmac_sha256(const uint8_t* key, size_t key_len,
                 const uint8_t* msg, size_t msg_len,
                 uint8_t out[32]) {
#if defined(ESP32)
  const mbedtls_md_info_t* md = mbedtls_md_info_from_type(MBEDTLS_MD_SHA256);
  if (!md) return false;
  if (mbedtls_md_hmac(md, key, key_len, msg, msg_len, out) != 0) return false;
  return true;
#elif defined(ESP8266)
  br_hmac_key_context kc;
  br_hmac_context hc;
  br_hmac_key_init(&kc, &br_sha256_vtable, key, key_len);
  br_hmac_init(&hc, &kc, 0);       // 0 = full output length
  br_hmac_update(&hc, msg, msg_len);
  br_hmac_out(&hc, out);
  return true;
#endif
}

/**
 * hex encoder
 */
String toHex(const uint8_t* buf, size_t len) {
  static const char* hex = "0123456789abcdef";
  String out;
  out.reserve(len * 2);
  for (size_t i = 0; i < len; ++i) {
    out += hex[(buf[i] >> 4) & 0xF];
    out += hex[buf[i] & 0xF];
  }
  return out;
}

String httpGet(String url, std::initializer_list<HttpHeader> headers) {
#ifdef USE_SSL
	WiFiClientSecure client;
	// client.setCACert(ROOT_CA);
	client.setInsecure(); // do not verify server certificate
#else
	WiFiClient client;
#endif
	HTTPClient http;

	http.begin(client, url);
	for (const auto& header : headers) {
		http.addHeader(header.name, header.value);
	}
	int httpResponseCode = http.GET();
	String body = httpResponseCode == 200 ? http.getString() : "";
	http.end();
	// if (body != "") { Logger::logln("get command " + body); }
	return body;
}

String httpGet(String url) {
	return httpGet(url, {});
}

void httpPost(String url, JsonDocument& data, std::initializer_list<HttpHeader> headers) {
#ifdef USE_SSL
	WiFiClientSecure client;
	// client.setCACert(ROOT_CA);
	client.setInsecure(); // do not verify server certificate
#else
	WiFiClient client;
#endif
	HTTPClient http;

	// stringify
	String payload;
	/*size_t dataSize = */serializeJson(data, payload);

	// calculate signature
	uint8_t mac[32];
	if (!hmac_sha256((const uint8_t*)apiKey, strlen(apiKey), (const uint8_t*)payload.c_str(), payload.length(), mac)) {
#ifdef DEBUG
		Serial.println("HMAC failed");
#endif
		return;
	}
	String signature = toHex(mac, 32);

	http.begin(client, url);
	http.addHeader("Content-Type", "application/json");
	http.addHeader("Content-Length", String(payload.length()));
	http.addHeader("X-Signature", signature);
	for (const auto& header : headers) {
		http.addHeader(header.name, header.value);
	}
	int httpResponseCode = http.POST(payload);

	// Serial.println("post: " + payload);
	// Serial.println("post state response: " + String(httpResponseCode));
	http.end();
}

void httpPost(String url, JsonDocument& data) { httpPost(url, data, {}); }
