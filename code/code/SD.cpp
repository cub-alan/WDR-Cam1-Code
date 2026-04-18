#include "SD.hpp"

const int SD_Pin = 21;
bool SD_Check = false;

static File root;
static File currentFile;
static bool uploading = false;

void SD_Init() {
    if (!SD.begin(SD_Pin)) {
        Serial.println("SD Card Mount Failed");
        SD_Check = false;
        return;
    }

    if (!SD.exists("/samples")) {
        SD.mkdir("/samples");
        Serial.println("Created /samples folder");
    }

    Serial.println("SD Card Ready");
    SD_Check = true;
}

bool SD_Sample(camera_fb_t *fb, GnssData data) {
    if (!SD_Check || !fb) return false;

    char base[64];
    snprintf(base, sizeof(base), "/samples/%02d%02d%02d_%lu",
             data.hour, data.min, data.sec, millis());

    String imgPath = String(base) + ".jpg";
    File imgFile = SD.open(imgPath, FILE_WRITE);
    if (!imgFile) return false;

    imgFile.write(fb->buf, fb->len);
    imgFile.close();

    String txtPath = String(base) + ".txt";
    File txtFile = SD.open(txtPath, FILE_WRITE);
    if (!txtFile) return false;

    txtFile.printf("lat=%.6f\nlon=%.6f\nalt=%.2f\nsats=%d\n",
                   data.lat, data.lon, data.alt, data.satellites);
    txtFile.close();

    return true;
}

void SD_Upload_Begin() {
    if (!SD_Check) return;

    root = SD.open("/samples");
    if (!root || !root.isDirectory()) {
        Serial.println("Upload failed: no folder");
        return;
    }

    currentFile = root.openNextFile();
    uploading = true;

    Serial.println("Upload started");
}

bool SD_Upload_Active() {
    return uploading;
}

void SD_Upload_Task() {
    static unsigned long lastUpload = 0;

    // limit rate (important)
    if (millis() - lastUpload < 200) return;
    lastUpload = millis();

    if (!uploading) return;

    if (!currentFile) {
        root.close();
        uploading = false;
        Serial.println("Upload complete");
        return;
    }

    String filename = String(currentFile.name());
    Serial.println("Uploading: " + filename);

    currentFile.close();

    // move to next file
    currentFile = root.openNextFile();
}

#include "esp_http_server.h"

// LIST FILES
esp_err_t SD_List_Handler(httpd_req_t *req) {
    if (!SD_Check) {
        httpd_resp_send(req, "SD not ready", HTTPD_RESP_USE_STRLEN);
        return ESP_FAIL;
    }

    File root = SD.open("/samples");
    if (!root || !root.isDirectory()) {
        httpd_resp_send(req, "No directory", HTTPD_RESP_USE_STRLEN);
        return ESP_FAIL;
    }

    String response = "[";

    File file = root.openNextFile();
    bool first = true;

    while (file) {
        if (!first) response += ",";
        response += "\"" + String(file.name()) + "\"";
        first = false;

        file = root.openNextFile();
    }

    response += "]";

    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, response.c_str(), response.length());

    return ESP_OK;
}
esp_err_t SD_File_Handler(httpd_req_t *req) {
    char filepath[128];

    // Expect: /file?name=xxx.jpg
    if (httpd_req_get_url_query_str(req, filepath, sizeof(filepath)) != ESP_OK) {
        httpd_resp_send(req, "No query", HTTPD_RESP_USE_STRLEN);
        return ESP_FAIL;
    }

    char filename[64];
    if (httpd_query_key_value(filepath, "name", filename, sizeof(filename)) != ESP_OK) {
        httpd_resp_send(req, "No filename", HTTPD_RESP_USE_STRLEN);
        return ESP_FAIL;
    }

    String path = "/samples/" + String(filename);
    File file = SD.open(path);

    if (!file) {
        httpd_resp_send(req, "File not found", HTTPD_RESP_USE_STRLEN);
        return ESP_FAIL;
    }

    httpd_resp_set_type(req, "application/octet-stream");

    uint8_t buffer[1024];
    while (file.available()) {
        size_t len = file.read(buffer, sizeof(buffer));
        httpd_resp_send_chunk(req, (const char*)buffer, len);
    }

    file.close();
    httpd_resp_send_chunk(req, NULL, 0); // end response

    return ESP_OK;
}
esp_err_t SD_Delete_Handler(httpd_req_t *req) {
    char query[128];

    if (httpd_req_get_url_query_str(req, query, sizeof(query)) != ESP_OK) {
        httpd_resp_send(req, "No query", HTTPD_RESP_USE_STRLEN);
        return ESP_FAIL;
    }

    char filename[64];
    if (httpd_query_key_value(query, "name", filename, sizeof(filename)) != ESP_OK) {
        httpd_resp_send(req, "No filename", HTTPD_RESP_USE_STRLEN);
        return ESP_FAIL;
    }

    String path = "/samples/" + String(filename);

    if (SD.remove(path)) {
        httpd_resp_send(req, "Deleted", HTTPD_RESP_USE_STRLEN);
        return ESP_OK;
    } else {
        httpd_resp_send(req, "Delete failed", HTTPD_RESP_USE_STRLEN);
        return ESP_FAIL;
    }
}