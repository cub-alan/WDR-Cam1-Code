#include "Cam.hpp"
#include "Arduino.h"

// define all of the pins (pin names and numbers pulled from example code from esp32 CameraWebServer by adafruit)
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

static esp_err_t stream_handler(httpd_req_t *req) {
    camera_fb_t * fb = NULL;
    esp_err_t res = ESP_OK;
    size_t _jpg_buf_len = 0;
    uint8_t * _jpg_buf = NULL;
    char * part_buf[64];

    // Set the response type to MJPEG boundary
    res = httpd_resp_set_type(req, "_boundary=frame");
    if(res != ESP_OK) return res;

    // Continuous loop to stream frames
    while(true) {
        // Capture a frame from the camera
        fb = esp_camera_fb_get();
        if (!fb) {
            Serial.println("Camera capture failed");
            res = ESP_FAIL;
        } else {
            _jpg_buf_len = fb->len;
            _jpg_buf = fb->buf;
        }

        // Send the frame header
        if(res == ESP_OK){
            size_t hlen = snprintf((char *)part_buf, 64, "Content-Type: image/jpeg\r\nContent-Length: %u\r\n\r\n", _jpg_buf_len);
            res = httpd_resp_send_chunk(req, (const char *)part_buf, hlen);
        }
        
        // Send the actual JPEG data
        if(res == ESP_OK) res = httpd_resp_send_chunk(req, (const char *)_jpg_buf, _jpg_buf_len);
        
        // Send the frame boundary
        if(res == ESP_OK) res = httpd_resp_send_chunk(req, "\r\n--frame\r\n", 15);

        // Return the frame buffer to the driver for reuse
        if(fb) {
            esp_camera_fb_return(fb);
            fb = NULL;
        }
        
        // If an error occurred or the client disconnected, break the loop
        if(res != ESP_OK) break;
    }
    return res;
}

void Cam_init() {
    camera_config_t Debug;
    //set all camera settings (settings pulled from example code from esp32 CameraWebServer by adafruit)
    Debug.ledc_channel = LEDC_CHANNEL_0;
    Debug.ledc_timer = LEDC_TIMER_0;
    Debug.pin_d0 = Y2_GPIO_NUM;
    Debug.pin_d1 = Y3_GPIO_NUM;
    Debug.pin_d2 = Y4_GPIO_NUM;
    Debug.pin_d3 = Y5_GPIO_NUM;
    Debug.pin_d4 = Y6_GPIO_NUM;
    Debug.pin_d5 = Y7_GPIO_NUM;
    Debug.pin_d6 = Y8_GPIO_NUM;
    Debug.pin_d7 = Y9_GPIO_NUM;
    Debug.pin_xclk = XCLK_GPIO_NUM;
    Debug.pin_pclk = PCLK_GPIO_NUM;
    Debug.pin_vsync = VSYNC_GPIO_NUM;
    Debug.pin_href = HREF_GPIO_NUM;
    Debug.pin_sscb_sda = SIOD_GPIO_NUM;
    Debug.pin_sscb_scl = SIOC_GPIO_NUM;
    Debug.pin_pwdn = PWDN_GPIO_NUM;
    Debug.pin_reset = RESET_GPIO_NUM;
    Debug.xclk_freq_hz = 10000000;
    Debug.frame_size = FRAMESIZE_QVGA;
    Debug.pixel_format = PIXFORMAT_JPEG;
    Debug.grab_mode = CAMERA_GRAB_WHEN_EMPTY;
    Debug.fb_location = CAMERA_FB_IN_PSRAM;
    Debug.jpeg_quality = 20;
    Debug.fb_count = 1;

    esp_err_t error = esp_camera_init(&Debug); //
    if (error != ESP_OK) {
        Serial.printf("Cam_init failed: 0x%x", error);
        return;
    }
}

void Server_init() {
    httpd_config_t server = HTTPD_DEFAULT_CONFIG();
    
    // Define the URI for the stream (root "/")
    httpd_uri_t Stream = { .uri = "/", .method = HTTP_GET, .handler = stream_handler,.user_ctx = NULL };
    
    // Start the server
    if (httpd_start(&Server, &server) == ESP_OK) {
        httpd_register_uri_handler(Server, &Stream);
        Serial.println("Camera stream server started");
    }
}
