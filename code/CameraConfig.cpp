#include "CameraConfig.hpp"
#include "esp_camera.h"
#include "board_config.h"
#include "camera_index.h"
#include <WiFi.h> 

const char *ssid = "iPhone";
const char *password = "12345678";

void startCameraServer();

void Cam_Init(void *Parametor);

void Cam_Thread(void){
  xTaskCreatePinnedToCore(Cam_Init,"Cam",12288,NULL,5,NULL,1); // (Task to run,debug name ,Stack Size in words, takes the pointer from Cam_Init , priority(1-24), weather to store handle , core running on)
}


void Cam_Init(void *Parametor){

  // this function is built from sections from the CameraWebServer example code on esp32 by espressif

  camera_config_t config;

  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer   = LEDC_TIMER_0;
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
  config.frame_size = FRAMESIZE_QVGA;
  config.pixel_format = PIXFORMAT_JPEG;
  config.fb_location = CAMERA_FB_IN_PSRAM;
  config.jpeg_quality = 10;
  config.fb_count = 1;

  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed\n");
    vTaskDelete(NULL);
    return;
  }
  WiFi.begin(ssid,password);
  WiFi.setSleep(false);

  while (WiFi.status() != WL_CONNECTED) {
    vTaskDelay(pdMS_TO_TICKS(500)); //delay this task for 
  }
  startCameraServer();
  
  printf("Camera ready IP: %s \n",WiFi.localIP().toString().c_str());

  while (1) {
    vTaskDelay(pdMS_TO_TICKS(10000));
  }
}