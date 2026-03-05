#pragma once
#include <WiFi.h>
#include <WebServer.h>
#include <Update.h>


WebServer ota_server(81);

void OTA_Init() {
  ota_server.on("/", HTTP_GET, []() {
    ota_server.send(200, "text/html",
      "<h3>ESP32 OTA Update</h3>"
      "<form method='POST' action='/update' enctype='multipart/form-data'>"
      "<input type='file' name='update'> "
      "<input type='submit' value='Update'>"
      "</form>");
  });

  ota_server.on("/update", HTTP_POST, []() {
    ota_server.send(200, "text/plain", (Update.hasError()) ? "Update Failed" : "Update Success. Rebooting...");
    delay(1000);
    ESP.restart();
  }, []() {
    HTTPUpload& upload = ota_server.upload();
    if (upload.status == UPLOAD_FILE_START) {
      Serial.printf("Update: %s\n", upload.filename.c_str());
      if (!Update.begin(UPDATE_SIZE_UNKNOWN)) Update.printError(Serial);
    } else if (upload.status == UPLOAD_FILE_WRITE) {
      if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) Update.printError(Serial);
    } else if (upload.status == UPLOAD_FILE_END) {
      if (Update.end(true)) Serial.printf("Update Success: %u\nRebooting...\n", upload.totalSize);
      else Update.printError(Serial);
    }
  });

  ota_server.begin();
  Serial.println("OTA Server started on port 81");
}

void OTA_Handle() {
  ota_server.handleClient();
}