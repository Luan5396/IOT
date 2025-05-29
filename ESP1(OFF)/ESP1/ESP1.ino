#include <ESP8266WiFi.h>
 #include "DHTesp.h"
 #define gasPin A0
 #include <MAX30105.h>  // Thư viện SparkFun
 #include <Wire.h>
 #include <Adafruit_GFX.h>
 #include <Adafruit_SSD1306.h>
#include <espnow.h>
uint8_t receiverMac[] = {0x2C, 0x3A, 0xE8, 0x12, 0x76, 0xF7};
//uint8_t receiverMac3[] = {0x2C, 0x3A, 0xE8, 0x12, 0x6C, 0x77};  //2c:3a:e8:12:6c:77

 
  // Định nghĩa kích thước màn hình OLED
 #define SCREEN_WIDTH 128
 #define SCREEN_HEIGHT 64
 // Khởi tạo đối tượng màn hình OLED với địa chỉ I2C (thường là 0x3C)
 #define OLED_RESET    -1  // Không sử dụng chân reset phần cứng
 Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

MAX30105 particleSensor;

 // Biến toàn cục
 const byte RATE_SIZE = 4; 
 byte rates[RATE_SIZE]; 
 byte rateSpot = 0;  
 long lastBeat = 0;
  // Nhịp tim và SpO₂
 float beatsPerMinute;
 int beatAvg;

// Cảm biến DHT11
 DHTesp dht;
 int dhtPin = 0; // Chân D1 trên NodeMCU (GPIO5)
 float temp;
 float humi;
 float gas;
 float heartRate; // Nhịp tim
 float oxygenLevel; // Oxy máu
// Cập nhật struct với 5 trường
struct Data {
  float t;
  float h;
  float g;
  float hr; // Nhịp tim
  float o; // Oxy máu
};

Data dataToSend;

void onSend(uint8_t *mac_addr, uint8_t sendStatus) {
  Serial.print("Trang thai gui: ");
  Serial.println(sendStatus == 0 ? "THANH CONG" : "       THAT BAI");
}
void onSend3(uint8_t *mac_addr, uint8_t sendStatus) {
  Serial.print("Trang thai gui: ");
  Serial.println(sendStatus == 0 ? "THANH CONG 333" : "       THAT BAI333");
}

 // Biến thời gian
 unsigned long currentTime = millis();
 unsigned long currentTime2 = millis();

void setup() {
    Serial.begin(115200);
    Serial.println();
    
    // Thiết lập cảm biến DHT11
    dht.setup(dhtPin, DHTesp::DHT11);

    // Khởi tạo màn hình OLED
    if (!display.begin(SSD1306_PAGEADDR, 0x3C)) {
      Serial.println(F("Không tìm thấy màn hình OLED!"));
      for (;;);
    }

    // Hiển thị ban đầu
    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);
    display.setTextSize(1); // Kích thước chữ


    //canhbaoNhipTim
      Wire.begin(D2, D1);  // SDA, SCL

    if (!particleSensor.begin()) {
      Serial.println("Không tìm thấy cảm biến MAX30102!");
      while (1);  // Dừng chương trình nếu không tìm thấy cảm biến
    }
    particleSensor.setup(0x1F, 4, 2, 100, 411, 4096); 
    particleSensor.setPulseAmplitudeRed(0xFF);   
    particleSensor.setPulseAmplitudeIR(0xFF);  

    Serial.println("Khởi động MAX30102 thành công!");

    WiFi.mode(WIFI_STA);

  if (esp_now_init() != 0) {
    Serial.println("Loi ESP-NOW init");
    return;
  }

  esp_now_set_self_role(ESP_NOW_ROLE_CONTROLLER);
  esp_now_add_peer(receiverMac, ESP_NOW_ROLE_SLAVE, 1, NULL, 0);
  esp_now_register_send_cb(onSend);

//////  esp_now_set_self_role(ESP_NOW_ROLE_CONTROLLER);
//////  esp_now_add_peer(receiverMac3, ESP_NOW_ROLE_SLAVE, 1, NULL, 0);
//////  esp_now_register_send_cb(onSend3);
}
unsigned long lastDisplayUpdate = 0;
void loop() {
    // Đọc dữ liệu từ cảm biến mỗi 1 giây
    if (millis() - currentTime > 1000) {
      float h = dht.getHumidity();
      float t = dht.getTemperature();

      if (dht.getStatus() == DHTesp::ERROR_NONE) { // Kiểm tra cảm biến hoạt động tốt
        temp = t;
        humi = h;
//        Serial.print("Temperature: ");
//        Serial.print(temp);
//        Serial.print(" °C, Humidity: ");
//        Serial.print(humi);
//        Serial.println(" %");
      } else {
        Serial.print("DHT Error: ");
        Serial.println(dht.getStatusString());
      }
      int adcValue = analogRead(gasPin);
      float voltage = adcValue / 1024.0 * 3.3;
      float ratio = voltage / 1.4;
      float gasValue = 1000.0 * pow(10, ((log10(ratio) - 1.0278) / 0.6629));
//      Serial.println("Gas value: " + String(gasValue));
      gas=gasValue;
      currentTime = millis();
    long irValue = particleSensor.getIR();    // Đọc IR để tính nhịp tim
    long redValue = particleSensor.getRed();  // Đọc Red để tính SpO₂

    if (irValue < 50000) {
      Serial.println("Không phát hiện ngón tay.");
      oxygenLevel = 0;
        heartRate = 0;

    } else {
      // Đo nhịp tim
      long delta = millis() - lastBeat;  
      if (irValue > 50000 && delta > 250) {  
        lastBeat = millis();  
        beatsPerMinute = 60 / (delta / 1000.0);  
        heartRate = beatsPerMinute;
      
        // Lưu kết quả vào mảng
        if (beatsPerMinute >= 40 && beatsPerMinute <= 180) {
          rates[rateSpot++] = (byte)beatsPerMinute;
          rateSpot %= RATE_SIZE;
        }

        // Tính trung bình
        beatAvg = 0;
        for (byte i = 0; i < RATE_SIZE; i++) {
          beatAvg += rates[i];
        }
        beatAvg /= RATE_SIZE;
      }

      // Tính SpO₂ giả lập (cần thuật toán chính xác)
      float spO2 = 110 - 25 * ((float)redValue / (float)irValue);  
      spO2 = constrain(spO2, 60, 100);  // Giới hạn từ 90% đến 100%
      oxygenLevel = spO2;
      // In kết quả
//      Serial.print("Nhịp tim: ");
//      Serial.print(beatsPerMinute);
//      Serial.print(" BPM | Trung bình: ");
//      Serial.print(beatAvg);
//      Serial.print(" BPM | Nồng độ oxy: ");
//      Serial.print(spO2);
//      Serial.println(" %");
//      Serial.print("Giá trị IR: ");
//      Serial.println(irValue);  
    }
  }

 static unsigned long lastSend = 0;
  if (millis() - lastSend > 1000) {
    dataToSend.t = temp;
    dataToSend.h = humi;
    dataToSend.g = gas;
    dataToSend.hr = heartRate;
    dataToSend.o = oxygenLevel;
    esp_now_send(receiverMac, (uint8_t *)&dataToSend, sizeof(dataToSend));
/////    esp_now_send(receiverMac3, (uint8_t *)&dataToSend, sizeof(dataToSend));
    lastSend = millis();

    
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
}
