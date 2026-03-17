//Jacob Holwill 10859926
//

#include "Gps.hpp"
#include <HardwareSerial.h>
#include <string.h>
#include <stdlib.h>

HardwareSerial GNSS(1);
GnssData GPS;

#define GNSS_RX 43
#define GNSS_TX 44
#define GNSS_BAUD 9600

bool Checksum(const char* sentence)
{
  if (*sentence != '$') return false;

  uint8_t sum = 0;
  sentence++;

  while (*sentence && *sentence != '*')
  {
    sum ^= *sentence++;
  }

  if (*sentence == '*')
  {
    int received = strtol(sentence + 1, NULL, 16);
    return sum == received;
  }

  return false;
}

void Gnss_init()
{
  GPS.mutex = xSemaphoreCreateMutex();

  GNSS.begin(GNSS_BAUD, SERIAL_8N1, GNSS_RX, GNSS_TX);

  xTaskCreatePinnedToCore(
      GnssTask,
      "GNSS Task",
      4096,
      NULL,
      1,
      NULL,
      1);
}

void GnssTask(void *param)
{
  char line[128];
  int idx = 0;

  while (true)
  {
    while (GNSS.available())
    {
      char c = GNSS.read();

      if (c == '\r') continue;

      if (c == '\n')
      {
        line[idx] = '\0';
        idx = 0;

        if (strlen(line) < 10) continue;

        if (!Checksum(line)) continue;

        if (xSemaphoreTake(GPS.mutex, portMAX_DELAY))
        {
          GPS.dataReceived = true;
          xSemaphoreGive(GPS.mutex);
        }

        if (strncmp(line, "$GNGGA", 6) != 0 && strncmp(line, "$GPGGA", 6) != 0)
          continue;

        char *fields[15];
        int field = 0;

        char *token = strtok(line, ",");

        while (token && field < 15)
        {
          fields[field++] = token;
          token = strtok(NULL, ",");
        }

        if (field < 10) continue;

        int quality = atoi(fields[6]);

        if (quality == 0) continue;

        const char* latStr = fields[2];
        const char* latDir = fields[3];
        const char* lonStr = fields[4];
        const char* lonDir = fields[5];

        const char* altStr = fields[9];
        const char* satStr = fields[7];
        const char* timeStr = fields[1];

        double latRaw = atof(latStr);
        double lonRaw = atof(lonStr);

        int latDeg = latRaw / 100;
        int lonDeg = lonRaw / 100;

        double latitude = latDeg + (latRaw - latDeg * 100) / 60.0;
        double longitude = lonDeg + (lonRaw - lonDeg * 100) / 60.0;

        if (latDir[0] == 'S') latitude = -latitude;
        if (lonDir[0] == 'W') longitude = -longitude;

        float altitude = atof(altStr);
        int satellites = atoi(satStr);

        int hour = 0, minute = 0, second = 0;

        if (strlen(timeStr) >= 6)
        {
          hour = (timeStr[0]-'0')*10 + (timeStr[1]-'0');
          minute = (timeStr[2]-'0')*10 + (timeStr[3]-'0');
          second = (timeStr[4]-'0')*10 + (timeStr[5]-'0');
        }
        if (xSemaphoreTake(GPS.mutex, portMAX_DELAY))
        {
          GPS.lat = lat;
          GPS.lon = lon;
          GPS.alt = alt;
          GPS.satellites = satellites;
          GPS.hour = hour;
          GPS.minute = min;
          GPS.second = sec;
          GPS.val = true;
          xSemaphoreGive(GPS.mutex);
        }
      }
      else
      {
        if (idx < sizeof(line) - 1)
          line[idx++] = c;
        else
          idx = 0;
      }
    }
    vTaskDelay(pdMS_TO_TICKS(10));
  }
}
static esp_err_t GPS_Status_Update(httpd_req_t *Status_Request) { // a static function that returns error for the status request
    char GNSS_Write[256]; // create a buffer for the GNSS to be able to write in
    if (xSemaphoreTake(GPS.mutex, 10) == pdTRUE) { // attempt to lock the mutex to be able to acess it and protect the GNSS data 

        const char* GNSS_Check; // ceate a check to see if the gnss is receiving anything

        if (GPS.val){ // if receiving data
            GNSS_Check = "true"; // check is true
        }
        else{ // if not receiving data
            GNSS_Check = "false"; // check is false
        }

        snprintf(GNSS_Write, sizeof(GNSS_Write),  // print the GNSS_Write buffer witht the following text while keeping the buffer size
        "{\"lat\":%.6f,\"lon\":%.6f,\"alt\":%.2f,\"sats\":%d,\"h\":%d,\"m\":%d,\"s\":%d,\"valid\":%s}", // write this to the buffer
        lat, lon, alt, satellites, hour, min, sec, GNSS_Check); // write the data to its corrosponding % in the previous text

        xSemaphoreGive(GPS.mutex); // unlock the mutex
    } 
    else { // if mutex is unable to be locked/read
        strcpy(GNSS_Write, "{\"error\":\"GPS mutex is busy\"}"); // if the mutex is being used create this error message in the buffer
    }
    httpd_resp_set_type(Status_Request, "application/json"); // tells the browser the respons of the json
    return httpd_resp_send(Status_Request, GNSS_Write, strlen(GNSS_Write)); // sends the json back to the client
}
