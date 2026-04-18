//Jacob Holwill 10859926
//the point of this file is to be able to use it to call SD card functions in main

#ifndef STORAGE_HPP
#define STORAGE_HPP

//include all nessesary librarys
#include <Arduino.h>
#include "FS.h"
#include "SD.h"
#include "SPI.h"
#include "Cam.hpp"
#include "Gps.hpp"

// retrive the SD checking value
extern bool SD_Check; 

//initialise all functions for the SD card
void SD_Init();
bool SD_Sample(camera_fb_t *fb, GnssData data);

void SD_Upload_Begin();
void SD_Upload_Task();
bool SD_Upload_Active();

esp_err_t SD_List_Handler(httpd_req_t *req);
esp_err_t SD_File_Handler(httpd_req_t *req);
esp_err_t SD_Delete_Handler(httpd_req_t *req);

#endif