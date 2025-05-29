#include <ESP8266WiFi.h>
#include <espnow.h>

// Cập nhật struct giống ESP1
struct Data {
  int a;
  int b;
  int c;
  int d;
  int e;
};

Data receivedData;

void onDataRecv(uint8_t *mac, uint8_t *incomingData, uint8_t len) {
  memcpy(&receivedData, incomingData, sizeof(receivedData));
  
  Serial.print("Nhan du lieu: ");
  Serial.print("a = "); Serial.print(receivedData.a);
  Serial.print(", b = "); Serial.print(receivedData.b);
  Serial.print(", c = "); Serial.print(receivedData.c);
  Serial.print(", d = "); Serial.print(receivedData.d);
  Serial.print(", e = "); Serial.println(receivedData.e);
}

void setup() {
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);

  if (esp_now_init() != 0) {
    Serial.println("Loi ESP-NOW init");
    return;
  }

  esp_now_set_self_role(ESP_NOW_ROLE_SLAVE);
  esp_now_register_recv_cb(onDataRecv);
}

void loop() {}