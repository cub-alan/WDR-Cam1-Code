#pragma once
#include <WiFi.h>
#include <WebServer.h>
#include <Update.h>

extern const char* ssid;
extern const char* password;

WebServer server(80);

void OTA_Init() {

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }

  server.on("/", HTTP_GET, []() {
    server.send(200, "text/html",
      "<form method='POST' action='/update' enctype='multipart/form-data'>"
      "<input type='file' name='update'>"
      "<input type='submit' value='Update'>"
      "</form>");
  });

  server.on("/update", HTTP_POST, []() {
    server.send(200, "text/plain", "Update Complete. Rebooting...");
    ESP.restart();
  }, []() {

    HTTPUpload& upload = server.upload();

    if (upload.status == UPLOAD_FILE_START) {
      Update.begin();
    }

    if (upload.status == UPLOAD_FILE_WRITE) {
      Update.write(upload.buf, upload.currentSize);
    }

    if (upload.status == UPLOAD_FILE_END) {
      Update.end(true);
    }

  });

  server.begin();
}

void OTA_Handle() {
  server.handleClient();
}