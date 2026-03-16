//Jacob Holwill 10859926
//

#include "Gps.hpp"
#include <HardwareSerial.h>
#include <string.h>
#include <stdlib.h>

HardwareSerial GNSS(1);
GnssData GPS;

#define GNSS_RX 44
#define GNSS_TX 43
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
          GPS.lat = latitude;
          GPS.lon = longitude;
          GPS.alt = altitude;
          GPS.satellites = satellites;
          GPS.hour = hour;
          GPS.minute = minute;
          GPS.second = second;
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
