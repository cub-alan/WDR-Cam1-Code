//Jacob Holwill 10859926
//the point of this file is to be able to use it to call Cam functions in main

// ensure the .hpp file doesnt get defined more then once
#ifndef CAM_HPP 
#define CAM_HPP

//include all nessesary librarys for the Cam
#include "esp_camera.h"
#include "esp_http_server.h"
#include <WiFi.h>
#include "Arduino.h"

//initialise the Cam.cpp functions
extern bool camera_initialized;
extern httpd_handle_t Server;
void Cam1_init();
esp_err_t Cam1_Stream_Update(httpd_req_t *req);

#endif