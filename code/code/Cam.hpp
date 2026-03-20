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

//Pin Definitions were taken from the XIAO ESP32-S3 Sense section for the camera webserver example code
#define PWDN_GPIO_NUM  -1
#define RESET_GPIO_NUM -1
#define XCLK_GPIO_NUM  10
#define SIOD_GPIO_NUM  40
#define SIOC_GPIO_NUM  39
#define Y9_GPIO_NUM    48
#define Y8_GPIO_NUM    11
#define Y7_GPIO_NUM    12
#define Y6_GPIO_NUM    14
#define Y5_GPIO_NUM    16
#define Y4_GPIO_NUM    18
#define Y3_GPIO_NUM    17
#define Y2_GPIO_NUM    15
#define VSYNC_GPIO_NUM 38
#define HREF_GPIO_NUM  47
#define PCLK_GPIO_NUM  13

//initialise the Cam.cpp functions
extern bool camera_initialized;
extern httpd_handle_t Server;
void Cam1_init();
esp_err_t Cam_Stream_Handler(httpd_req_t *req);

#endif