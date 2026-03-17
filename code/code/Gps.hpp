//Jacob Holwill 10859926
//

// ensure the .hpp file doesnt get defined more then once
#ifndef GPS_HPP
#define GPS_HPP

#include <Arduino.h>
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "esp_http_server.h"
#include <HardwareSerial.h>
#include <string.h>
#include <stdlib.h>

struct GnssData {

  float lat = 0.0;
  float lon = 0.0;
  float alt = 0.0;

  int satellites = 0;

  int hour = 0;
  int min = 0;
  int sec = 0;

  bool Fix_Val = false;
  bool data_Received = false;

  SemaphoreHandle_t mutex = NULL;
};

extern GnssData GPS;

void Gnss_init();
void GnssTask(void *param);
esp_err_t GPS_Status_Update(httpd_req_t *Status_Request);

#endif