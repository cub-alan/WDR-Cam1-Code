//Jacob Holwill 10859926
// a file to set up the streaming of the camera and gnss data and to connect to the wifi

#include "Server.hpp"
#include "SD.hpp"

// set the ssid and password 
//const char* ssid = "Robot-Stu1"; 
//const char* password = "WANUT8PTODAY";

//testing WIFI
const char* ssid = "iPhone"; 
const char* password = "12345678";

extern volatile bool stream_active;

httpd_handle_t Server = NULL; // create the server handle and set it to null


esp_err_t Mode_Handler(httpd_req_t *req) {
    char query[64];

    if (httpd_req_get_url_query_str(req, query, sizeof(query)) == ESP_OK) {
        char param[16];

        if (httpd_query_key_value(query, "mode", param, sizeof(param)) == ESP_OK) {
            if (strcmp(param, "stream") == 0) {
                currentMode = MODE_STREAM;
                stream_active = true;
                Serial.println("Switched to STREAM mode");
            }
            else if (strcmp(param, "sd") == 0) {
                currentMode = MODE_SD;
                stream_active = false;
                Serial.println("Switched to SD mode");
            }
        }
    }

    httpd_resp_set_type(req, "text/plain");
    return httpd_resp_sendstr(req, "OK");
}

esp_err_t Cam_Stream_Handler(httpd_req_t *req) {

    stream_active = true;

    httpd_resp_set_type(req, "multipart/x-mixed-replace; boundary=frame");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");

    while (currentMode == MODE_STREAM) {

        camera_fb_t *fb = esp_camera_fb_get();

        if (!fb) {
            Serial.println("Camera capture failed");
            stream_active = false;
            return ESP_FAIL;
        }

        double lat = 0;
        double lon = 0;
        float alt = 0;
        int sats = 0;
        bool valid = false;

        if (xSemaphoreTake(GPS.mutex, 0)) {
            lat = GPS.lat;
            lon = GPS.lon;
            alt = GPS.alt;
            sats = GPS.satellites;
            valid = GPS.Fix_Val;
            xSemaphoreGive(GPS.mutex);
        }

        char json[192];
        snprintf(
            json,
            sizeof(json),
            "{\"lat\":%.6f,\"lon\":%.6f,\"alt\":%.2f,\"sats\":%d,\"valid\":%d}",
            lat, lon, alt, sats, valid
        );

        char header[256];
        int len = snprintf(
            header,
            sizeof(header),
            "Content-Type: image/jpeg\r\n"
            "Content-Length: %u\r\n"
            "X-GNSS: %s\r\n\r\n",
            fb->len,
            json
        );

        esp_err_t res = ESP_OK;

        res = httpd_resp_send_chunk(req, "--frame\r\n", 9);

        if (res == ESP_OK) {
            res = httpd_resp_send_chunk(req, header, len);
        }

        if (res == ESP_OK) {
            res = httpd_resp_send_chunk(req, (const char *)fb->buf, fb->len);
        }

        if (res == ESP_OK) {
            res = httpd_resp_send_chunk(req, "\r\n", 2);
        }

        esp_camera_fb_return(fb);

        if (res != ESP_OK) {
            Serial.println("Stream client disconnected");
            break;
        }

        vTaskDelay(pdMS_TO_TICKS(30));
    }

    stream_active = false;
    return ESP_OK;
}

void Cam1_Server_Init() {

  httpd_config_t Cam1_Server_Config = HTTPD_DEFAULT_CONFIG(); // create a variable to save all server info/ settings

    Cam1_Server_Config.server_port = 80; // set the webservers pot to number 80 which is standard
    Cam1_Server_Config.stack_size = 8192;
    Cam1_Server_Config.max_uri_handlers = 8;
    Cam1_Server_Config.lru_purge_enable = true;

    // create a variable and store the cameras URL in it for streaming and to asses the cams veiw
    static httpd_uri_t Stream_URI = {.uri = "/stream1", .method = HTTP_GET, .handler = Cam_Stream_Handler, .user_ctx  = NULL}; 
    static httpd_uri_t GNSS_URI = {.uri = "/gnss",.method = HTTP_GET,.handler = GPS_Status_Update,.user_ctx = NULL};

    static httpd_uri_t Mode_Check = {.uri = "/mode",.method = HTTP_GET,.handler = Mode_Handler,.user_ctx = NULL};

    static httpd_uri_t SD_List_URI = {.uri = "/sd/list",.method = HTTP_GET,.handler = SD_List_Handler,.user_ctx = NULL};
    static httpd_uri_t SD_File_URI = {.uri = "/sd/file",.method = HTTP_GET,.handler = SD_File_Handler,.user_ctx = NULL};
    static httpd_uri_t SD_Delete_URI = {.uri = "/sd/delete",.method = HTTP_GET,.handler = SD_Delete_Handler,.user_ctx = NULL};


    // Start the server
    if (httpd_start(&Server, &Cam1_Server_Config) == ESP_OK) { //check everything is set up correctly start the server
        httpd_register_uri_handler(Server, &Stream_URI); // create the cameras page on the server 
        httpd_register_uri_handler(Server, &GNSS_URI);

        httpd_register_uri_handler(Server, &Mode_Check);
        httpd_register_uri_handler(Server, &SD_List_URI);
        httpd_register_uri_handler(Server, &SD_File_URI);
        httpd_register_uri_handler(Server, &SD_Delete_URI);

    }
}

void WIFI_Connect(){
    Serial.print("Connecting to WiFi");

    WiFi.begin(ssid, password);// stat the wifi connection
    WiFi.setSleep(false); // stop sleep for impoved performance

    unsigned long WIFI_Boot_Timer = millis(); // creates a variable to store time for real time functions

    // try connecting for 5 passes
    while (WiFi.status() != WL_CONNECTED && millis() - WIFI_Boot_Timer <5000) {
        delay(500);
        Serial.print(".");
    }

    if (WiFi.status() == WL_CONNECTED) { // if connected
        Serial.println("\nWiFi connected");
        Serial.print("IP: ");
        Serial.println(WiFi.localIP()); //prin the IP to the terminal
    } else { // if not connected swap to SD usage
        Serial.println("\n WiFi failed now using SD mode");
        currentMode = MODE_SD;
        stream_active = false;
    }
}
