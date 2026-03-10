#include "Cam.hpp"
#include "Arduino.h"
#include "Gps.hpp"

// XIAO ESP32-S3 Sense Pin Definitions retrieved from camera webserver example code
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


// HTML Dashboard
const char index_html[] PROGMEM = R"=====(
<!DOCTYPE html>
<html>
<head> <title>Weed Detection Robot - Live View</title>
<meta name="viewport" content="width=device-width, initial-scale=1">
<!-- Leaflet Map Library -->
<link rel="stylesheet" href="https://unpkg.com/leaflet@1.9.4/dist/leaflet.css"/>
<script src="https://unpkg.com/leaflet@1.9.4/dist/leaflet.js"></script>
<style>
body{
    font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, Helvetica, Arial, sans-serif;
    background: #264D2B;
    color: #000000;
    margin: 0;
    display: flex;
    flex-direction: column;
    align-items: center;
}
.container{
    width: 100%;
    max-width: 640px;
    padding: 20px;
    box-sizing: border-box;
}
.header{
    display: flex;
    justify-content: space-between;
    align-items: center;
    margin-bottom: 20px;
    border-bottom: 1px solid #333;
    padding-bottom: 10px;
}
h1{
    font-size: 24px;
    margin: 0;
    color: #000000;
    text-transform: uppercase;
    letter-spacing: 2px;
}
.stream-box {
    width: 100%;
    background: #000;
    border-radius: 12px;
    overflow: hidden;
    box-shadow: 0 10px 30px rgba(0,0,0,0.5);
    border: 1px solid #222;
}
.stream-box img {
    width: 100%;
    display: block;
}
#map {
    width: 100%;
    height: 300px;
    margin-top: 20px;
    border-radius: 12px;
    border: 1px solid #222;
}
.data-grid {
    display: grid;
    grid-template-columns: 1fr 1fr;
    gap: 12px;
    margin-top: 20px;
}
.data-card {
    background: #000000;
    padding: 15px;
    border-radius: 10px;
    border: 1px solid #222;
    transition: border-color 0.3s;
}
.data-card:hover {
    border-color: #444;
}
.label {
    font-size: 10px;
    color: #666;
    text-transform: uppercase;
    letter-spacing: 1px;
    margin-bottom: 6px;
}
.value {
    font-size: 16px;
    font-weight: 600;
    font-family: "JetBrains Mono", monospace;
    color: #fff;
}
.status-dot {
    display: inline-block;
    width: 8px;
    height: 8px;
    border-radius: 50%;
    margin-right: 8px;
}
.status-locked {
    background: #4caf50;
    box-shadow: 0 0 10px rgba(76, 175, 80, 0.4);
}
.dots::after {
    content: '.';
    animation: dots 1.5s infinite;
}
@keyframes dots {
    0%,32% {content: '.';}
    33%,65% {content: '..';}
    66%,100% {content: '...';}
}
</style>
</head>
<body>
<div class="container">
<div class="header">
<h1>Weed Detection Robot</h1>
<div id="clock" style="font-size:12px;color:#403F3F;">ONLINE</div>
</div>
<div class="stream-box">
<img src="/stream" id="stream">
</div>
<!-- MAP -->
<div id="map"></div>
<div class="data-grid">
<div class="data-card">
<div class="label">GNSS Status</div>
<div class="value" id="gps-status">
<span class="status-dot status-searching"></span>
Finding<span class="dots"></span>
</div>
</div>
<div class="data-card">
<div class="label">GPS Time (UTC)</div>
<div class="value" id="gps-time">00:00:00</div>
</div>
<div class="data-card">
<div class="label">Altitude</div>
<div class="value" id="alt">0.00 m</div>
</div>
<div class="data-card">
<div class="label">Coordinates</div>
<div class="value" id="coords">0.000000, 0.000000</div>
</div>
</div>
</div>
<script>
var map = L.map('map').setView([0,0], 18);
L.tileLayer('https://tile.openstreetmap.org/{z}/{x}/{y}.png', {
    maxZoom: 19
}).addTo(map);
var marker = L.marker([0,0]).addTo(map);
function updateStatus() {
    fetch('/status')
    .then(response => response.json())
    .then(data => {
        if (data.valid) {
            document.getElementById('gps-status').innerHTML ='<span class="status-dot status-locked"></span>Locked';
            document.getElementById('coords').innerText = data.lat.toFixed(6) + ', ' + data.lon.toFixed(6);
            document.getElementById('alt').innerText = data.alt.toFixed(2) + ' m';
            const h = String(data.h).padStart(2,'0');
            const m = String(data.m).padStart(2,'0');
            const s = String(data.s).padStart(2,'0');
            document.getElementById('gps-time').innerText = h + ':' + m + ':' + s;
            marker.setLatLng([data.lat, data.lon]);
            map.panTo([data.lat, data.lon]);
        }
        else {
            document.getElementById('gps-status').innerHTML ='<span class="status-dot status-searching"></span>Finding<span class="dots"></span>';
            document.getElementById('coords').innerText ='WAITING FOR LOCK...';
            document.getElementById('gps-time').innerText ='00:00:00';
        }
    })
.catch(err => console.error('Fetch error:', err));
}
setInterval(updateStatus, 1000);
</script>
</body>
</html>)=====";

//handler for the html 
static esp_err_t index_handler(httpd_req_t *req) {
    httpd_resp_set_type(req, "text/html");
    return httpd_resp_send(req, index_html, strlen(index_html));
}

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

// handler for the gnss
static esp_err_t status_handler(httpd_req_t *req) {
    char json_response[160];
    if (xSemaphoreTake(GPS.mutex, (TickType_t)10) == pdTRUE) {
        snprintf(json_response, sizeof(json_response), 
                 "{\"lat\":%.6f,\"lon\":%.6f,\"alt\":%.2f,\"h\":%d,\"m\":%d,\"s\":%d,\"valid\":%s}", 
                 GPS.lat, GPS.lon, GPS.alt, GPS.hour, GPS.minute, GPS.second, GPS.val ? "true" : "false");
        xSemaphoreGive(GPS.mutex);
    } else {
        strcpy(json_response, "{\"error\":\"mutex_timeout\"}");
    }
    httpd_resp_set_type(req, "application/json");
    return httpd_resp_send(req, json_response, strlen(json_response));
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
  config.pin_sccb_sda = SIOD_GPIO_NUM;
  config.pin_sccb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.frame_size = FRAMESIZE_UXGA;
  config.pixel_format = PIXFORMAT_JPEG;  // for streaming
  config.grab_mode = CAMERA_GRAB_WHEN_EMPTY;
  config.fb_location = CAMERA_FB_IN_PSRAM;
  config.jpeg_quality = 5;
  config.fb_count = 2;

    // Initialize the camera
    esp_err_t error = esp_camera_init(&config); 
    if (error != ESP_OK) {
        Serial.printf("Cam_init failed: 0x%x\n", error);
        return;
    }

    // Manual sensor adjustments for better stability
    sensor_t * s = esp_camera_sensor_get();
    if (s->id.PID == OV3660_PID) {
    s->set_vflip(s, 1);        // flip it vertically back
    s->set_brightness(s, -3);   // reduce the brightness 
    s->set_saturation(s, -1);  // lower the saturation
    }
    if (config.pixel_format == PIXFORMAT_JPEG) {
    s->set_framesize(s, FRAMESIZE_QVGA);
    }
}

void Server_init() {
    httpd_config_t server_config = HTTPD_DEFAULT_CONFIG();
    server_config.server_port = 80;
    server_config.ctrl_port = 32768; // Avoid conflict with other services

    // Define the URI for the index
    httpd_uri_t index_uri = {
        .uri      = "/",
        .method   = HTTP_GET,
        .handler  = index_handler,
        .user_ctx = NULL
    };
    // Define the URI for the stream
    httpd_uri_t stream_uri = {
        .uri      = "/stream",
        .method   = HTTP_GET,
        .handler  = stream_handler,
        .user_ctx = NULL
    };
    // Define the URI for the status
    httpd_uri_t status_uri = {
        .uri      = "/status",
        .method   = HTTP_GET,
        .handler  = status_handler,
        .user_ctx = NULL
    };

    
    // Start the server
    if (httpd_start(&Server, &server_config) == ESP_OK) {
        httpd_register_uri_handler(Server, &index_uri);
        httpd_register_uri_handler(Server, &stream_uri);
        httpd_register_uri_handler(Server, &status_uri);
        Serial.println("Camera server started on port 80");
    } else {
        Serial.println("Failed to start camera server");
    }
}