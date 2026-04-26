//Jacob Holwill 10859926
//the point of this file is to be able to use it to call server functions in main

// ensure the .hpp file doesnt get defined more then once
#ifndef SERVER_HPP 
#define SERVER_HPP

// include all nessesary librarys for the server to work
#include "esp_http_server.h"
#include "Cam.hpp"
#include "Gps.hpp"
#include <WiFi.h>
#include "Arduino.h"
#include "SD.hpp"

// initialise the ssid and password to create the web server on
extern const char* ssid; 
extern const char* password;

//initialise the Server.cpp functions
extern httpd_handle_t Server;
void Cam1_Server_Init();
void WIFI_Connect();

// create the two modes the sytem can be in if wifi is connected or not
enum SystemMode {
  MODE_STREAM,
  MODE_SD
};

extern volatile SystemMode currentMode;

#endif