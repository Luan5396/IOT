#include <ESP8266WiFi.h>
#include <espnow.h>
#include <WiFiClientSecure.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Firebase_ESP_Client.h>
#include "addons/TokenHelper.h"

// OLED Config
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// Cấu trúc dữ liệu ESP-NOW
struct Data {
  float t;
  float h;
  float g;
  float hr; // Nhịp tim
  float o; // Oxy máu
} receivedData;

// Biến cảm biến
float temp = 0;
float humi = 0; 
float gas = 0;
float heartRate = 0; 
float oxygenLevel = 0;

// WiFi Credentials
const char* ssid = "Luan";
const char* password = "05032003@";

// Google Sheets
const char* host = "script.google.com";
const int httpsPort = 443;
const char* GAS_ID = "AKfycbzgixY_2pXolSGZZWhDZ-zAveBdzUhmSz4dsLQ3d6qitGVg5zlggbFPp0KQNe9iVPGhxw";
BearSSL::WiFiClientSecure client;

// Firebase
///////#define API_KEY "AIzaSyDp8CSZgqR4zUr7CLiGpbRLFNFgLrHY37U"
///////#define DATABASE_URL "https://oiyt-79bf7-default-rtdb.firebaseio.com/"
#define API_KEY "AIzaSyA7rlwEH1s41Els-B-tSv7pUE9M8CmuZAQ"
#define DATABASE_URL "https://real-time-data-6-default-rtdb.firebaseio.com/"
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;
bool signupOK = false;

// Timer Variables
unsigned long prevMillisDisplay = 0;
unsigned long prevMillisFirebase = 0;
unsigned long prevMillisSheets = 0;

// Hàm xử lý nhận dữ liệu ESP-NOW
void OnDataRecv(uint8_t *mac, uint8_t *incomingData, uint8_t len) {
  memcpy(&receivedData, incomingData, sizeof(receivedData));
  temp = receivedData.t;
  humi = receivedData.h;
  gas = receivedData.g;
  heartRate = receivedData.hr;
  oxygenLevel = receivedData.o;

  Serial.printf("         Nhận dữ liệu:\n %.1fC %.1f%% %.1fppm HR%.0f SpO2%.0f\n", 
               temp, humi, gas, heartRate, oxygenLevel);
}

void setupOLED() {
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Chặn chương trình
  }
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0,0);
  display.display();
}

void connectWiFi() {
  WiFi.mode(WIFI_STA);
  Serial.printf("Connecting to %s ", ssid);
  WiFi.begin(ssid, password);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}

void setupFirebase() {
  config.api_key = API_KEY;
  config.database_url = DATABASE_URL;
  config.token_status_callback = tokenStatusCallback;

  if (Firebase.signUp(&config, &auth, "", "")) {
    signupOK = true;
    Serial.println("Firebase: OK");
  } else {
    Serial.printf("Firebase error: %s\n", config.signer.signupError.message.c_str());
  }

  Firebase.begin(&config, &auth);
  Firebase.reconnectNetwork(true);
}

void updateDisplay() {
  display.clearDisplay();
  
  // Cột trái
  display.setCursor(0, 0);
  display.printf("HR:%.0f bpm", heartRate);
  display.setCursor(0, 16);
  display.printf("SpO2:%.0f%%", oxygenLevel);  
  display.setCursor(0, 32);
  display.printf("");

  // Cột phải
  display.setCursor(64, 0);
  display.printf("T:%.1fC", temp);
  display.setCursor(64, 16);
  display.printf("H:%.1f%%", humi);
  display.setCursor(64, 32);
  display.printf("Gas:%.0fppm", gas);

  display.display();
}

void sendToFirebase() {
  if(Firebase.ready() && signupOK) {
  Serial.println("Đã gửi FIREBASE");


    Firebase.RTDB.setFloatAsync(&fbdo, "DHT_11/Temperature", temp);
    Firebase.RTDB.setFloatAsync(&fbdo, "DHT_11/Humidity", humi);
    Firebase.RTDB.setFloatAsync(&fbdo, "DHT_11/Gas", gas);
    Firebase.RTDB.setFloatAsync(&fbdo, "DHT_11/HeartRate", heartRate);
    Firebase.RTDB.setFloatAsync(&fbdo, "DHT_11/OxygenLevel", oxygenLevel);
  }else{
  Serial.println("Lỗi gửi FIREBASE");
  }
}

void sendToGoogleSheets() {
  Serial.println("Preparing to send to Google Sheets...");
  
  char url[256];
  snprintf(url, sizeof(url), 
    "/macros/s/%s/exec?sts=write&temp=%.1f&humi=%.1f&gas=%.0f&heart=%.0f&oxygen=%.0f",
    GAS_ID, temp, humi, gas, heartRate, oxygenLevel);

  client.stop();
  client.setInsecure();
  
  if (!client.connect(host, httpsPort)) {
    Serial.println("Connection failed");
    return;
  }

  client.print(String("GET ") + url + " HTTP/1.1\r\n" +
               "Host: " + host + "\r\n" +
               "Connection: close\r\n\r\n");

  // Đọc phản hồi nhanh
  while(client.connected() && !client.available()) delay(1);
  while(client.available()) client.read();

  client.stop();
  Serial.println("   Data sent to Sheets");


}

void setup() {
  Serial.begin(115200);
  setupOLED();
  connectWiFi();
  
  // ESP-NOW
  if (esp_now_init() != 0) {
    Serial.println("Error initializing ESP-NOW");
    ESP.restart();
  }
  esp_now_set_self_role(ESP_NOW_ROLE_SLAVE);
  esp_now_register_recv_cb(OnDataRecv);

  // Firebase
  setupFirebase();

  // Google Sheets client
  client.setBufferSizes(512, 512);
}
unsigned long lastDisplayUpdate = 0;
 unsigned long currentTime2 = millis();
 unsigned long currentTime3 = millis();

void loop() {
  unsigned long currentMillis = millis();

  // Update display every 1s
if (millis() - lastDisplayUpdate > 700) {  
        lastDisplayUpdate = millis();
    updateDisplay();
    //sendToFirebase();
    //sendToGoogleSheets();
    //Serial.printf("Free Heap: %u\n", ESP.getFreeHeap());
  }
      // Gửi dữ liệu lên Google Sheets mỗi 10 giây
////    if (millis() - currentTime2 > 10000) {
////      sendToGoogleSheets();
////      currentTime2 = millis();
////    }
    if (millis() - currentTime3 > 1000) {
      sendToFirebase();
      currentTime3 = millis();
    }
}