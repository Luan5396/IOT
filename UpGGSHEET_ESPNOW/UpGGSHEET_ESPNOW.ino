#include <ESP8266WiFi.h>
#include <espnow.h>
#include <WiFiClientSecure.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

  // Định nghĩa kích thước màn hình OLED
 #define SCREEN_WIDTH 128
 #define SCREEN_HEIGHT 64
 // Khởi tạo đối tượng màn hình OLED với địa chỉ I2C (thường là 0x3C)
 #define OLED_RESET    -1  // Không sử dụng chân reset phần cứng
 Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

  float temp = 10;
  float humi = 10;
  float gas = 10;
  float heartRate = 10;
  float oxygenLevel = 10;
// Cập nhật struct với 5 trường
struct Data {
 float t;
 float h;
 float g;
 float hr; // Nhịp tim
 float o; // Oxy máu
};

Data receivedData;


  // Thông tin kết nối Wi-Fi
 const char* ssid = "Luan";     // Tên Wi-Fi
 const char* password = "05032003@";  // Mật khẩu                                    sửa tại đây
  // Google Apps Script
 const char* host = "script.google.com";
 const int httpsPort = 443;
 String GAS_ID = "AKfycbzgixY_2pXolSGZZWhDZ-zAveBdzUhmSz4dsLQ3d6qitGVg5zlggbFPp0KQNe9iVPGhxw";


WiFiClientSecure client;
 unsigned long currentTime = millis();
 unsigned long currentTime2 = millis();

 void onDataRecv(uint8_t *mac, uint8_t *incomingData, uint8_t len) {
  memcpy(&receivedData, incomingData, sizeof(receivedData));
  
  Serial.println("Nhan du lieu: ");
  temp = receivedData.t;
  humi = receivedData.h ;
  gas = receivedData.g;
  heartRate = receivedData.hr;
  oxygenLevel = receivedData.o;
  Serial.println(temp);
  Serial.println(humi);
  Serial.println(gas);
  Serial.println(heartRate);
  Serial.println(oxygenLevel);
}


void setup() {
  Serial.begin(115200);
  
    // Khởi tạo màn hình OLED
    if (!display.begin(SSD1306_PAGEADDR, 0x3C)) {
      Serial.println(F("Không tìm thấy màn hình OLED!"));
      for (;;);
    }

    // Hiển thị ban đầu
    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);
    display.setTextSize(1); // Kích thước chữ

    
  WiFi.mode(WIFI_STA);

  if (esp_now_init() != 0) {
    Serial.println("Loi ESP-NOW init");
    return;
  }
  esp_now_set_self_role(ESP_NOW_ROLE_SLAVE);
  esp_now_register_recv_cb(onDataRecv);


  // Kết nối Wi-Fi
    Serial.print("Connecting to Wi-Fi: ");
    Serial.println(ssid);
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
    }
    Serial.println("\nWiFi connected");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
    
    // Tắt kiểm tra chứng chỉ SSL
    client.setInsecure();

}
unsigned long lastDisplayUpdate = 0;
void loop() {
  static unsigned long lastUpdate = 0;

    if (millis() - lastDisplayUpdate > 1000) {  
        lastDisplayUpdate = millis();
        
        display.clearDisplay();
        
        // Cột trái - Thông số sức khỏe
        display.setCursor(0, 0);
        display.print("HR:");
        display.print(heartRate, 0); // Số nguyên
        display.print(" bpm");

        display.setCursor(0, 16);
        display.print("SpO2:");
        display.print(oxygenLevel, 0);
        display.print("%");

        // Cột phải - Thông số môi trường
        display.setCursor(64, 0);
        display.print("T:");
        display.print(temp, 1); // 1 số thập phân
        display.print("C");

        display.setCursor(64, 16);
        display.print("H:");
        display.print(humi, 1);
        display.print("%");

        display.setCursor(64, 32);
        display.print("Gas:");
        display.print(gas, 0); // Số nguyên
        display.print("ppm");

        display.display();
    }
    // Gửi dữ liệu lên Google Sheets mỗi 10 giây
    if (millis() - currentTime2 > 3000) {
      upData();
      
      currentTime2 = millis();
    }
}


void upData() {
    Serial.print("Connecting to ");
    Serial.println(host);

    if (!client.connect(host, httpsPort)) {
      Serial.println("Connection failed");
      return;}

    // Tạo URL để gửi dữ liệu lên Google Sheets
  String Send_Data_URL = "sts=write";
  Send_Data_URL += "&temp=" + String(temp);
  Send_Data_URL += "&humi=" + String(humi);
  Send_Data_URL += "&gas=" + String(gas);  // Thêm giá trị khí vào URL
  Send_Data_URL += "&heart=" + String(heartRate);  // Thêm nhịp tim
  Send_Data_URL += "&oxygen=" + String(oxygenLevel); // Thêm oxy máu
  String url = "/macros/s/" + GAS_ID + "/exec?" + Send_Data_URL;

  Serial.println("Sending data to URL:");
  Serial.println(url);

  client.print(String("GET ") + url + " HTTP/1.1\r\n" +
              "Host: " + host + "\r\n" +
              "User-Agent: ESP8266\r\n" +
              "Connection: close\r\n\r\n");
  Serial.println("Data sent successfully!");
}
