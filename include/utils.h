#ifndef __UTILS_H
#define __UTILS_H

#include <Arduino.h>


#define countof(a) (sizeof(a) / sizeof(a[0]))

String b2s(bool b);
String leadingZero(uint8_t value);
//bool remoteLog(uint8_t level, String msg, String auth, String device);

#endif
