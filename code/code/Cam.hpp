#ifndef CAM_HPP
#define CAM_HPP

#include "esp_camera.h"
#include "esp_http_server.h"
#include <WiFi.h>

// create the functions for the camera 
void Cam_init();
void Server_init();

#endif