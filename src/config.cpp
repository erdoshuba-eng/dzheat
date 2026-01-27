/*
 * config.cpp
 *
 *  Created on: Jul 12, 2020
 *      Author: huba
 */

#include "config.h"
#include "ds18b20_utils.h"

const char* device = "dzheat";
// const char* ver = "4.0.0"; // 2020-07-25
// const char* ver = "5.0.0"; // 2024-09-30
const char* ver = "6.0.0"; // 2026-01-25

const char* ROOT_CA = R"(-----BEGIN CERTIFICATE-----
MIIFNTCCBB2gAwIBAgISBmaT5nKKZ1H3ArdlId/wMgZOMA0GCSqGSIb3DQEBCwUA
MDMxCzAJBgNVBAYTAlVTMRYwFAYDVQQKEw1MZXQncyBFbmNyeXB0MQwwCgYDVQQD
EwNSMTEwHhcNMjUwODAzMTg0MTI2WhcNMjUxMTAxMTg0MTI1WjAUMRIwEAYDVQQD
DAkqLnp5cmEucm8wggEiMA0GCSqGSIb3DQEBAQUAA4IBDwAwggEKAoIBAQDKfd+Z
EoPj56mg/qLikGKJ0SP3Ra4GjdXB+XgHKBmelZ1C0JiKsLLhymodcsl+jYWWM8LI
PfD6EGrUro4a50h5+/agqhkmRxlMBvSxY4MHhdVIIAvnLcylBqx9vhp1btr0ifjX
/QIma2TOalTLjruLO7ut2WldYLHr5enpNB7aY+0tPRBw0Tptz7249yh54xcuUZhI
cuoWrppZ0vyvBn5xcForT2XqVVZ+5r2NOL3dHI7nLzYOnZz4Yv4jRL1R1yNjs/B9
RYrBpl2Y0MuwPQ42v5fB/zvCWOJJtXJ49qX88aLEXzBk+R3YrB0ybgxZETPa2roB
4sBocmKySBojLeffAgMBAAGjggJgMIICXDAOBgNVHQ8BAf8EBAMCBaAwHQYDVR0l
BBYwFAYIKwYBBQUHAwEGCCsGAQUFBwMCMAwGA1UdEwEB/wQCMAAwHQYDVR0OBBYE
FLwmM9R7aaoIg+HBn6l4AuuVig7qMB8GA1UdIwQYMBaAFMXPRqTq9MPAemyVxC2w
XpIvJuO5MDMGCCsGAQUFBwEBBCcwJTAjBggrBgEFBQcwAoYXaHR0cDovL3IxMS5p
LmxlbmNyLm9yZy8wXAYDVR0RBFUwU4IJKi56eXJhLnJvghJ3d3cuZWxpZ2h0Lnp5
cmEucm+CE3d3dy5tb25pdG9yLnp5cmEucm+CFHd3dy5tb25pdG9yMi56eXJhLnJv
ggd6eXJhLnJvMBMGA1UdIAQMMAowCAYGZ4EMAQIBMC4GA1UdHwQnMCUwI6AhoB+G
HWh0dHA6Ly9yMTEuYy5sZW5jci5vcmcvNzkuY3JsMIIBAwYKKwYBBAHWeQIEAgSB
9ASB8QDvAHYA7TxL1ugGwqSiAFfbyyTiOAHfUS/txIbFcA8g3bc+P+AAAAGYcXKn
2wAABAMARzBFAiAyIkWz6fsxVeZ81kBMFTveQo1kSStxl/mE8/xQN0VOHQIhAOqS
9ea7T1SzQh6UheQkdEhfnMY10dYPKuype5soNK0UAHUA3dzKNJXX4RYF55Uy+sef
+D0cUN/bADoUEnYKLKy7yCoAAAGYcXKoGgAABAMARjBEAiBIiIkhRkNAO7FNUKzs
ccoG+DTIg1aIWIjHvEKUeEbOZgIgcJ8iLPGba0kMPIys3K31VWv6e12/4DwmlC6q
52YWRsMwDQYJKoZIhvcNAQELBQADggEBAFTlR/AtsUp6TNEoavfIkvxo6t5CsGrP
My+rIVqwFpJlDYk6tfdI/Ewcxi+9mCw42WXhdsxV77HkxI7her8lnU+hps0rdYv1
ISD/pdAN7fbS/2cGq14xyQYJmIudRIDQVT0X581RKko1kFI+RWg/pC/GUKT5VDBb
QLlB8dQmIHsAu/VLs4Ivc9V50+54nA5SatjHQr4ZEfwybyfrh9M6n8+qFEE7HWXF
PTFHjgWa6kxk9veY3jXVjf3D9sqVeDxYlQ+As6wZxTEGUXTNq6k99T0r5yriKE1C
vy+4YL4ovZrYyrel7kdzEsaYvx64kEm+FykWYkr5wuZ8xzvwCMaq3Jw=
-----END CERTIFICATE-----)";

#ifdef ENYEM
  TTemperatureSensor temperatureSensors[temperatureSensorsCount] = {
    {false, "2872016c4f200156", "fás kazán", "4e1fd9f5-6773-4607-878c-f76861de6e38", -1, 45, 85, false, 0},
    {false, "28082cc10b000011", "puffer fent", "739190cd-5865-4f1c-a702-f9764d3f0452", -1, 40, 82, false, 0},
    {false, "283660bb00000055", "puffer lent", "e46b3d9a-d829-4d33-916d-6d3162eed1a5", -1, 40, 82, false, 0}
  };
#else
  TTemperatureSensor temperatureSensors[temperatureSensorsCount] = {
    {false, "28ff283ba616050c", "fás kazán", "4e1fd9f5-6773-4607-878c-f76861de6e38", -1, 45, 85, false, 0}, // TWF
    {false, "28ffb1f0a41605aa", "puffer fent", "739190cd-5865-4f1c-a702-f9764d3f0452", -1, 40, 82, false, 0}, // TBT
    {false, "28ffc92ca0160574", "puffer lent", "e46b3d9a-d829-4d33-916d-6d3162eed1a5", -1, 40, 82, false, 0} // TBB
  };
#endif
