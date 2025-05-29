#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <Firebase_ESP_Client.h>
#include "addons/TokenHelper.h"
#include <DHT.h>  

// ===== CẤU HÌNH CẢM BIẾN =====
#define DHT_SENSOR_PIN 4           // Chân DHT11 nối vào D2 (GPIO4)
#define DHT_SENSOR_TYPE DHT11
#define GAS_SENSOR_PIN A0          // Cảm biến khí gas (MQ-2/MQ-135) nối vào A0

DHT dht_sensor(DHT_SENSOR_PIN, DHT_SENSOR_TYPE);

// ===== WIFI =====
#define WIFI_SSID "Staff Only"
#define WIFI_PASSWORD "family@123"

// ===== FIREBASE =====
#define API_KEY "AIzaSyDp8CSZgqR4zUr7CLiGpbRLFNFgLrHY37U"
#define DATABASE_URL "https://oiyt-79bf7-default-rtdb.firebaseio.com/"

FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

unsigned long sendDataPrevMillis = 0;
bool signupOK = false;

void setup() {
  Serial.begin(115200);
  Serial.println("🔌 Bắt đầu kết nối Wi-Fi...");

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(300);
  }
  Serial.println("\n✅ Đã kết nối Wi-Fi!");
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());

  // Khởi động cảm biến
  dht_sensor.begin();
  Serial.println("✅ Cảm biến DHT11 sẵn sàng!");

  // Firebase config
  config.api_key = API_KEY;
  config.database_url = DATABASE_URL;
  config.token_status_callback = tokenStatusCallback;

  if (Firebase.signUp(&config, &auth, "", "")) {
    Serial.println("✅ Đăng nhập Firebase thành công!");
    signupOK = true;
  } else {
    Serial.printf("❌ Lỗi đăng nhập Firebase: %s\n", config.signer.signupError.message.c_str());
  }

  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);
}

void loop() {
  if (Firebase.ready() && signupOK && (millis() - sendDataPrevMillis > 2000 || sendDataPrevMillis == 0)) {
    sendDataPrevMillis = millis();

    // Đọc DHT11
    float temperature = dht_sensor.readTemperature();
    float humidity = dht_sensor.readHumidity();

    // Đọc khí gas
    int gasValue = analogRead(GAS_SENSOR_PIN);

    // Kiểm tra lỗi cảm biến
    if (isnan(temperature) || isnan(humidity)) {
      Serial.println("❌ Lỗi đọc DHT11!");
      return;
    }

    Serial.print("🌡 Nhiệt độ: ");
    Serial.print(temperature);
    Serial.println(" °C");

    Serial.print("💧 Độ ẩm: ");
    Serial.print(humidity);
    Serial.println(" %");

    Serial.print("🧪 Khí gas (ADC): ");
    Serial.println(gasValue);

    // Gửi lên Firebase
    if (!Firebase.RTDB.setFloat(&fbdo, "DHT_11/Temperature", temperature))
      Serial.println("❌ Lỗi gửi nhiệt độ: " + fbdo.errorReason());

    if (!Firebase.RTDB.setFloat(&fbdo, "DHT_11/Humidity", humidity))
      Serial.println("❌ Lỗi gửi độ ẩm: " + fbdo.errorReason());

    if (!Firebase.RTDB.setInt(&fbdo, "DHT_11/Gas", gasValue))
      Serial.println("❌ Lỗi gửi khí gas: " + fbdo.errorReason());
  }
}