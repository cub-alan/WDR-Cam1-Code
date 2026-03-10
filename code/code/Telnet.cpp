#include "Telnet.hpp"

WiFiServer telnetServer(23);
WiFiClient telnetClient;

void Telnet_Init() {
    telnetServer.begin();
    telnetServer.setNoDelay(true);
}

void Telnet_Handle() {
    // Check if a new client is trying to connect
    if (telnetServer.hasClient()) {
        if (!telnetClient || !telnetClient.connected()) {
            if (telnetClient) telnetClient.stop();
            telnetClient = telnetServer.available();
            telnetClient.println("--- Wireless Terminal Connected ---");
        } else {
            // Reject additional clients
            telnetServer.available().stop();
        }
    }
}

void Wifi_Terminal_Write(const char* format, ...) {
    char loc_res[256];
    va_list arg;
    va_start(arg, format);
    vsnprintf(loc_res, sizeof(loc_res), format, arg);
    va_end(arg);

    // Always print to Serial for debugging
    Serial.print(loc_res);

    // Print to Telnet if client is active
    if (telnetClient && telnetClient.connected()) {
        telnetClient.print(loc_res);
    }
}