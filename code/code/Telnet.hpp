#ifndef REMOTESERIAL_HPP
#define REMOTESERIAL_HPP

#include <WiFi.h>

WiFiServer telnetServer(23);
WiFiClient telnetClient;

void Telnet_Init() {
    telnetServer.begin();
    telnetServer.setNoDelay(true);
}

// Function to print to both USB and Telnet
void Wifi_Terminal_Write(const char* format, ...) {
    char loc_res[256];
    va_list arg;
    va_start(arg, format);
    vsnprintf(loc_res, sizeof(loc_res), format, arg);
    va_end(arg);

    // Print to USB
    Serial.print(loc_res);

    // Print to Wireless Telnet
    if (telnetServer.hasClient()) {
        if (!telnetClient || !telnetClient.connected()) {
            telnetClient = telnetServer.available();
        }
    }
    if (telnetClient && telnetClient.connected()) {
        telnetClient.print(loc_res);
    }
}

#endif