#include "SD.hpp"

const int SD_Pin = 21;
bool SD_Check = false;

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
    if (!SD_Check || !fb) {
      return false;
    }

    char base[64];
    snprintf(base, sizeof(base), "/samples/%02d%02d%02d_%lu",data.hour, data.min, data.sec, millis());

    String imgPath = String(base) + ".jpg";
    File imgFile = SD.open(imgPath, FILE_WRITE);
    if (!imgFile) {
        Serial.println("Image write failed");
        return false;
    }
    imgFile.write(fb->buf, fb->len);
    imgFile.close();

    String txtPath = String(base) + ".txt";
    File txtFile = SD.open(txtPath, FILE_WRITE);
    if (!txtFile) {
        Serial.println("GNSS write failed");
        return false;
    }

    txtFile.printf("lat=%.6f\nlon=%.6f\nalt=%.2f\nsats=%d\n",data.lat, data.lon, data.alt, data.satellites);
    txtFile.close();

    return true;
}
esp_err_t SD_List_Handler(httpd_req_t *req) {

    File root = SD.open("/samples");
    if (!root || !root.isDirectory()) {
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }

    String json = "[";

    File file = root.openNextFile();
    bool first = true;

    while (file) {
        if (!first) json += ",";
        json += "\"" + String(file.name()) + "\"";
        first = false;
        file = root.openNextFile();
    }

    json += "]";

    httpd_resp_set_type(req, "application/json");
    return httpd_resp_send(req, json.c_str(), json.length());
}
esp_err_t SD_File_Handler(httpd_req_t *req) {

    char filepath[128];

    if (httpd_req_get_url_query_len(req) > 0) {
        char query[128];
        httpd_req_get_url_query_str(req, query, sizeof(query));

        char filename[64];
        if (httpd_query_key_value(query, "name", filename, sizeof(filename)) == ESP_OK) {
            snprintf(filepath, sizeof(filepath), "/samples/%s", filename);
        } else {
            httpd_resp_send_400(req);
            return ESP_FAIL;
        }
    } else {
        httpd_resp_send_400(req);
        return ESP_FAIL;
    }

    File file = SD.open(filepath, FILE_READ);
    if (!file) {
        httpd_resp_send_404(req);
        return ESP_FAIL;
    }

    httpd_resp_set_type(req, "application/octet-stream");

    char buffer[1024];
    size_t readBytes;

    while ((readBytes = file.readBytes(buffer, sizeof(buffer))) > 0) {
        httpd_resp_send_chunk(req, buffer, readBytes);
    }

    file.close();

    return httpd_resp_send_chunk(req, NULL, 0);
}
esp_err_t SD_Delete_Handler(httpd_req_t *req) {

    char query[128];
    char filename[64];

    if (httpd_req_get_url_query_str(req, query, sizeof(query)) == ESP_OK &&
        httpd_query_key_value(query, "name", filename, sizeof(filename)) == ESP_OK) {

        String path = "/samples/" + String(filename);

        if (SD.remove(path)) {
            httpd_resp_sendstr(req, "Deleted");
            return ESP_OK;
        }
    }

    httpd_resp_send_400(req);
    return ESP_FAIL;
}