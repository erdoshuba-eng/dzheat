#include "utils.h"

//#ifdef esp32
//#include <WiFi.h>
//#include <HTTPClient.h>
//#else

//#ifdef esp8266
//#include <ESP8266WiFi.h>
//#include <ESP8266HTTPClient.h>
//#endif

// zyra.ro certificate
//const char* root_ca = \
//"-----BEGIN CERTIFICATE-----\n" \
//"MIIF6DCCBNCgAwIBAgIQVI9EUq6ngTpc+ErRNCJlGTANBgkqhkiG9w0BAQsFADBy\n" \
//"MQswCQYDVQQGEwJVUzELMAkGA1UECBMCVFgxEDAOBgNVBAcTB0hvdXN0b24xFTAT\n" \
//"BgNVBAoTDGNQYW5lbCwgSW5jLjEtMCsGA1UEAxMkY1BhbmVsLCBJbmMuIENlcnRp\n" \
//"ZmljYXRpb24gQXV0aG9yaXR5MB4XDTIwMDcxOTAwMDAwMFoXDTIwMTAxNzIzNTk1\n" \
//"OVowGjEYMBYGA1UEAxMPbW9uaXRvci56eXJhLnJvMIIBIjANBgkqhkiG9w0BAQEF\n" \
//"AAOCAQ8AMIIBCgKCAQEA4inEzu1Hho9hT05mzxr1jzEXYw/vSHZThbS3rUFfNNMk\n" \
//"gFZlQnJLPNKpnhzhziN+MvVDZ/PnQQrobVZu4yqH3OlhPCdbFmNg0a+qfr1fjNX/\n" \
//"+Dm4eu5IxdvUoQs31IztEAKSU2C4g7zJaptyKs4Pa4jooXV+cGMSw4hpg6EBOxHu\n" \
//"q2hHakPifomGVw+6tG4Tz7jssSZra//IDD0IPpjTVqtcmV56WyusU/DMlVR5Zrzf\n" \
//"u/hsz2Ojv8I5HhQbUccUIepDon6VBNZf2c/aO31IoN/UaT/58hnVLAten6oaAhvM\n" \
//"tBJpMxmQdKUwL4jrcY7be5U9D3cH6/ovP8sl9Z2XgwIDAQABo4IC0DCCAswwHwYD\n" \
//"VR0jBBgwFoAUfgNaZUFrp34K4bidCOodjh1qx2UwHQYDVR0OBBYEFBY2isCCRGcE\n" \
//"iRRz0gFVTQ3HOdSOMA4GA1UdDwEB/wQEAwIFoDAMBgNVHRMBAf8EAjAAMB0GA1Ud\n" \
//"JQQWMBQGCCsGAQUFBwMBBggrBgEFBQcDAjBJBgNVHSAEQjBAMDQGCysGAQQBsjEB\n" \
//"AgI0MCUwIwYIKwYBBQUHAgEWF2h0dHBzOi8vc2VjdGlnby5jb20vQ1BTMAgGBmeB\n" \
//"DAECATBMBgNVHR8ERTBDMEGgP6A9hjtodHRwOi8vY3JsLmNvbW9kb2NhLmNvbS9j\n" \
//"UGFuZWxJbmNDZXJ0aWZpY2F0aW9uQXV0aG9yaXR5LmNybDB9BggrBgEFBQcBAQRx\n" \
//"MG8wRwYIKwYBBQUHMAKGO2h0dHA6Ly9jcnQuY29tb2RvY2EuY29tL2NQYW5lbElu\n" \
//"Y0NlcnRpZmljYXRpb25BdXRob3JpdHkuY3J0MCQGCCsGAQUFBzABhhhodHRwOi8v\n" \
//"b2NzcC5jb21vZG9jYS5jb20wggECBgorBgEEAdZ5AgQCBIHzBIHwAO4AdQAHt1wb\n" \
//"5X1o//Gwxh0jFce65ld8V5S3au68YToaadOiHAAAAXNnpQJfAAAEAwBGMEQCIGA+\n" \
//"O3TeRzVEHqTLTOyexra4cfn4PKUP0EPW5uEEjLyvAiAtCgJdEV9UIk/HzucDmvA8\n" \
//"yNXh8ZErCkH0JMe84hJqbAB1AOcS8rA3fhpi+47JDGGE8ep7N8tWHREmW/Pg80vy\n" \
//"QVRuAAABc2elAoYAAAQDAEYwRAIgGk+C4k3WMXHCB2ICX9S3YYEIc0dNo2BGOuGN\n" \
//"gdPP0tUCIC0mFUlDwYRDmyQrLBRIkVGc04oJm1P3FYBf2ctEwedKMC8GA1UdEQQo\n" \
//"MCaCD21vbml0b3IuenlyYS5yb4ITd3d3Lm1vbml0b3IuenlyYS5ybzANBgkqhkiG\n" \
//"9w0BAQsFAAOCAQEAX3nPCw+vFDqn9x+YcDj1++n4ExAXzw0uVMosuUsKqDehtTBC\n" \
//"DyjkjsqiPRwXuhF1SqfC6QcHGWx9DMZ/sfFg9nPqXyEv4derEY9wBpE9oJsUs2fe\n" \
//"4ufG0BFdEHlfR6jPYpHfWy/hTzJvVb9ayfYuQ8sdXUdEfs/sY0l38c6aHitFPM6g\n" \
//"/+xjcnqbNijXuErxjNq2cYlfHmUkAP3z/LhkYD2PewJL4gBtulT32KEh7BUgBXU2\n" \
//"dRhWvpOvYG/7IZ8m1DH30hj5YBXIiP2mHPiAj5H/q7zuPtL/heKtfjJxPvyKLhFQ\n" \
//"bXqQZzFmEVeyHFAi5HwRSRFvBHQNuBbaMn34yQ==\n" \
//"-----END CERTIFICATE-----\n";

String b2s(bool b) {
  return b ? "true" : "false";
}

String leadingZero(uint8_t value) {
  if (value < 10) {
    return "0" + String(value);
  }
  return String(value);
}
/*
bool remoteLog(uint8_t level, String msg, String auth, String device) {
  if (!WiFi.isConnected()) return false;
//   Serial.println("call request");
  String content = "{\"level\":" + String(level) + ",\"data\":" + msg + "}";
  HTTPClient http;
  http.begin("https://monitor.zyra.ro/log?cmd=log", root_ca);
  http.addHeader("Content-Type", "application/json");
  http.addHeader("Authorization", "Basic " + auth);
  http.addHeader("X-CSRF-Token", device);
/*  http.begin("192.168.1.28");
  http.addHeader("Content-Type", "application/json");
  String content = "";* /
  int httpResponseCode = http.POST(content);
  http.end();
  if (httpResponseCode > 0) {
    return httpResponseCode == 200;
  }
  return false;
/*  String respHeader = "";
  String respContent = "";
  bool addToHeader = true;
  bool firstLine = true;
  bool bRet = true;
  WiFiClient client;
//   if (client.connect("192.168.1.28", 80)) {
  if (client.connect("https://monitor.zyra.ro", 433)) {
//     Serial.println("send request");
    String content = "{\"level\":" + String(level) + ",\"data\":" + msg + "}";
//     client.print(String("POST /monitor/log?cmd=log HTTP/1.1\r\n") +
//     "Host: 192.168.1.28\r\n" +
    client.print(String("POST /log?cmd=log HTTP/1.1\r\n") +
    "Host: monitor.zyra.ro\r\n" +
    "Content-Type: application/json\r\n" +
    "Content-Length: " + String(content.length()) + "\r\n" +
    "Authorization: Basic " + auth + "\r\n" +
    "X-CSRF-Token: " + device + "\r\n" +
    "Connection: close\r\n\r\n" + content);
    // read response
    while (client.connected() || client.available()) {
      if (client.available()) {
        String line = client.readStringUntil('\n');
        line.trim();
        if (addToHeader) {
          // the content is separated from the header by an empty line
          if (line.equals("")) { addToHeader = false; }
          else {
            Serial.println(line);
            // add lines to the response header
            respHeader += line + "\n";
            if (firstLine) {
              // decode HTTP response
              int n = line.indexOf(' ');
              line.remove(0, n + 1);
              n = line.indexOf(' ');
              int responseCode = line.substring(0, n).toInt();
              bRet = responseCode == 200;
            }
            firstLine = false;
          }
        }
        else {
          if (!respContent.equals("")) { respContent += "\n"; }
          respContent += line;
        }
      }
    }
    client.stop();
  }
  Serial.println(respHeader);
  //respContent.trim();
  //Serial.println(respContent);
  //  if (respContent.equals("ok"))
  return bRet;* /
}*/
