#ifndef STORAGE_HPP
#define STORAGE_HPP

#include "FS.h"
#include "SD.h"
#include "SPI.h"
#include "Cam.hpp"
#include "Gps.hpp"
#include "esp_http_server.h"

extern bool SD_Check;

void SD_Init();
bool SD_Sample(camera_fb_t *fb, GnssData data);

esp_err_t SD_List_Handler(httpd_req_t *req);
esp_err_t SD_File_Handler(httpd_req_t *req);
esp_err_t SD_Delete_Handler(httpd_req_t *req);

void SD_UploadAll();

#endif