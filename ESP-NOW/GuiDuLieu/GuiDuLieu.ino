#include <ESP8266WiFi.h>
#include <espnow.h>

uint8_t receiverMac[] = {0x2C, 0x3A, 0xE8, 0x12, 0x76, 0xF7};
uint8_t receiverMac2[] = {0x2C, 0x3A, 0xE8, 0x12, 0x6C, 0x77};  //2c:3a:e8:12:6c:77


// Cập nhật struct với 5 trường
struct Data {
  int a;
  int b;
  int c;
  int d;
  int e;
};

Data dataToSend;

void onSend(uint8_t *mac_addr, uint8_t sendStatus) {
  Serial.print("Trang thai gui: ");
  Serial.println(sendStatus == 0 ? "THANH CONG" : "THAT BAI");
}

void setup() {
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);

  if (esp_now_init() != 0) {
    Serial.println("Loi ESP-NOW init");
    return;
  }

  esp_now_set_self_role(ESP_NOW_ROLE_CONTROLLER);
  esp_now_add_peer(receiverMac, ESP_NOW_ROLE_SLAVE, 1, NULL, 0);
  esp_now_add_peer(receiverMac2, ESP_NOW_ROLE_SLAVE, 1, NULL, 0);
  esp_now_register_send_cb(onSend);

  // Gán giá trị 5 trường
  dataToSend.a = 1;
  dataToSend.b = 2;
  dataToSend.c = 3;
  dataToSend.d = 4;
  dataToSend.e = 5;
}

void loop() {
  esp_now_send(receiverMac, (uint8_t *)&dataToSend, sizeof(dataToSend));
  Serial.println("Da gui du lieu");
  delay(1000); // Gửi mỗi 1 giây
}