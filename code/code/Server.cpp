void Cam1_Server_Init() {
  httpd_config_t Cam1_Server_Config = HTTPD_DEFAULT_CONFIG(); // create a variable to save all server info/ settings
  Cam1_Server_Config.server_port = 80; // set the webservers pot to number 80 which is standard
  Cam1_Server_Config.ctrl_port = 30000; // set the controll port to avoid conflict with any other ports

  // create a variable and store the cameras URL in it for streaming and to asses the cams veiw
  httpd_uri_t Cam1_Stream_URL = {.uri = "/stream", .method = HTTP_GET, .handler = Cam1_Stream_Update, .user_ctx = NULL}; 

  // create a variable and store the debugging/status URL in it to be able to acess relevent info
  httpd_uri_t Cam1_Status_URL = {.uri = "/status", .method = HTTP_GET, .handler = Cam1_Status_Update, .user_ctx = NULL};
    
  // Start the server
  if (httpd_start(&Server, &Cam1_Server_Config) == ESP_OK) { //check everything is set up correctly start the server
    httpd_register_uri_handler(Server, &Cam1_Stream_URL); // create the cameras page on the server 
    httpd_register_uri_handler(Server, &Cam1_Status_URL); // create the status page on the server
    Serial.println("Camera 1 server began"); // print to the terminal so you now the server is ready
  }
}
void WIFI_Connect(){
  
  Serial.println("");
  Serial.print("Connecting to WIFI");

  WiFi.begin(ssid, password);
  WiFi.setAutoReconnect(true);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");

}