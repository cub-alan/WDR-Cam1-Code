//Jacob Holwill 10859926
//the point of this file is to be able to use it to call Cam functions in main

// ensure the .hpp file doesnt get defined more then once
#ifndef SERVER_HPP 
#define SERVER_HPP

#include "esp_http_server.h"

// set the ssid and password to create the web server on (using my phone hotspot)
const char* ssid = "iPhone"; 
const char* password = "12345678";

//initialise the Server.cpp functions
void Cam1_Server_Init();
void WIFI_Connect();

#endif