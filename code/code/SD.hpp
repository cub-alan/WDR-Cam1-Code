//Jacob Holwill 10859926
//the point of this file is to be able to use it to call SD card functions in main

#ifndef SD_HPP
#define SD_HPP

//include all nessesary librarys
#include <Arduino.h>
#include "FS.h"
#include "SD.h"
#include "esp_http_server.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

//initialise all functions for the SD card
void SD_Init();
void SD_Task(void *param);
void SD_Start_Sending();
void SD_Stop_Sending();

esp_err_t SD_List_Handler(httpd_req_t *req);
esp_err_t SD_File_Handler(httpd_req_t *req);
esp_err_t SD_Delete_Handler(httpd_req_t *req);

#endif