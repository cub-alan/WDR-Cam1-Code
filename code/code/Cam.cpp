#include "Cam.hpp"
#include "Arduino.h"

// XIAO ESP32-S3 Sense Pin Definitions
#define PWDN_GPIO_NUM  -1
#define RESET_GPIO_NUM -1
#define XCLK_GPIO_NUM  10
#define SIOD_GPIO_NUM  40
#define SIOC_GPIO_NUM  39
#define Y9_GPIO_NUM    48
#define Y8_GPIO_NUM    11
#define Y7_GPIO_NUM    12
#define Y6_GPIO_NUM    14
#define Y5_GPIO_NUM    16
#define Y4_GPIO_NUM    18
#define Y3_GPIO_NUM    17
#define Y2_GPIO_NUM    15
#define VSYNC_GPIO_NUM 38
#define HREF_GPIO_NUM  47
#define PCLK_GPIO_NUM  13

httpd_handle_t Server = NULL;

// This handler serves the MJPEG stream
static esp_err_t stream_handler(httpd_req_t *req) {
    camera_fb_t * fb = NULL;
    esp_err_t res = ESP_OK;
    size_t _jpg_buf_len = 0;
    uint8_t * _jpg_buf = NULL;
    char * part_buf[64];

    res = httpd_resp_set_type(req, "multipart/x-mixed-replace;boundary=frame");
    if(res != ESP_OK) return res;

    while(true) {
        fb = esp_camera_fb_get();
        if (!fb) {
            Serial.println("Camera capture failed");
            res = ESP_FAIL;
        } else {
            _jpg_buf_len = fb->len;
            _jpg_buf = fb->buf;
        }

        if(res == ESP_OK){
            size_t hlen = snprintf((char *)part_buf, 64, "Content-Type: image/jpeg\r\nContent-Length: %u\r\n\r\n", _jpg_buf_len);
            res = httpd_resp_send_chunk(req, (const char *)part_buf, hlen);
        }
        
        if(res == ESP_OK) res = httpd_resp_send_chunk(req, (const char *)_jpg_buf, _jpg_buf_len);
        
        if(res == ESP_OK) res = httpd_resp_send_chunk(req, "\r\n--frame\r\n", 15);

        if(fb) {
            esp_camera_fb_return(fb);
            fb = NULL;
        }
        
        if(res != ESP_OK) break;
        
        // Small delay to prevent the camera from saturating the CPU/Power
        vTaskDelay(pdMS_TO_TICKS(1)); 
    }
    return res;
}

void Cam_init() {
    camera_config_t config;
    config.ledc_channel = LEDC_CHANNEL_0;
    config.ledc_timer = LEDC_TIMER_0;
    config.pin_d0 = Y2_GPIO_NUM;
    config.pin_d1 = Y3_GPIO_NUM;
    config.pin_d2 = Y4_GPIO_NUM;
    config.pin_d3 = Y5_GPIO_NUM;
    config.pin_d4 = Y6_GPIO_NUM;
    config.pin_d5 = Y7_GPIO_NUM;
    config.pin_d6 = Y8_GPIO_NUM;
    config.pin_d7 = Y9_GPIO_NUM;
    config.pin_xclk = XCLK_GPIO_NUM;
    config.pin_pclk = PCLK_GPIO_NUM;
    config.pin_vsync = VSYNC_GPIO_NUM;
    config.pin_href = HREF_GPIO_NUM;
    config.pin_sscb_sda = SIOD_GPIO_NUM;
    config.pin_sscb_scl = SIOC_GPIO_NUM;
    config.pin_pwdn = PWDN_GPIO_NUM;
    config.pin_reset = RESET_GPIO_NUM;
    
    // --- POWER OPTIMIZATION SETTINGS ---
    config.xclk_freq_hz = 10000000;       // Lower clock (10MHz) reduces current noise
    config.frame_size = FRAMESIZE_QVGA;    // Lower resolution = less WiFi data
    config.pixel_format = PIXFORMAT_JPEG;
    config.grab_mode = CAMERA_GRAB_LATEST; // Don't queue frames
    config.fb_location = CAMERA_FB_IN_PSRAM;
    config.jpeg_quality = 14;              // 10-63 (lower is higher quality). 14 is a good balance.
    config.fb_count = 1;                   // IMPORTANT: Only 1 buffer to minimize power spikes

    // Initialize the camera
    esp_err_t error = esp_camera_init(&config); 
    if (error != ESP_OK) {
        Serial.printf("Cam_init failed: 0x%x\n", error);
        return;
    }

    // Manual sensor adjustments for better stability
    sensor_t * s = esp_camera_sensor_get();
    if (s) {
        s->set_vflip(s, 0);       // Set to 1 if image is upside down
        s->set_hmirror(s, 0);    // Set to 1 if image is mirrored
        s->set_framesize(s, FRAMESIZE_QVGA); 
    }
}

void Server_init() {
    httpd_config_t server_config = HTTPD_DEFAULT_CONFIG();
    server_config.server_port = 80;
    server_config.ctrl_port = 32768; // Avoid conflict with other services

    // Define the URI for the stream
    httpd_uri_t stream_uri = {
        .uri      = "/",
        .method   = HTTP_GET,
        .handler  = stream_handler,
        .user_ctx = NULL
    };
    
    // Start the server
    if (httpd_start(&Server, &server_config) == ESP_OK) {
        httpd_register_uri_handler(Server, &stream_uri);
        Serial.println("Camera server started on port 80");
    } else {
        Serial.println("Failed to start camera server");
    }
}