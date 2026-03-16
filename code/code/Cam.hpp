//Jacob Holwill 10859926
//

// ensure the .hpp file doesnt get defined more then once
#ifndef CAM_HPP 
#define CAM_HPP

#include "esp_camera.h"
#include "esp_http_server.h"
#include <WiFi.h>

void Cam1_init();
void Cam1_Server_Init();

#endif