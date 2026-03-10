#ifndef REMOTESERIAL_HPP
#define REMOTESERIAL_HPP

#include <WiFi.h>
#include <stdarg.h>

extern WiFiServer telnetServer;
extern WiFiClient telnetClient;

void Telnet_Init();
void Telnet_Handle();
void Wifi_Terminal_Write(const char* format, ...);

#endif