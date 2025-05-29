#define RELAY1 D5
#define RELAY2 D6
#define RELAY3 D7
#define RELAY4 D8

#define BUTTON1 D1
#define BUTTON2 D2
#define BUTTON3 D3  // Nút cảm ứng bật/tắt cảnh báo

bool alarmMode = false;
unsigned long lastBlinkTime = 0;
const unsigned long blinkInterval = 500; //tốc độ nháy đèn
bool blinkState = false;

void setup() {
  Serial.begin(115200);

  pinMode(RELAY1, OUTPUT);
  pinMode(RELAY2, OUTPUT);
  pinMode(RELAY3, OUTPUT);
  pinMode(RELAY4, OUTPUT);

  pinMode(BUTTON1, INPUT_PULLUP);
  pinMode(BUTTON2, INPUT_PULLUP);
  pinMode(BUTTON3, INPUT_PULLUP);

  // Ban đầu relay ở trạng thái "đèn bật"
  digitalWrite(RELAY1, LOW);
  digitalWrite(RELAY2, HIGH);
  digitalWrite(RELAY3, LOW);
  digitalWrite(RELAY4, HIGH);
}

void loop() {
  // Xử lý nút cảm ứng bật/tắt cảnh báo
  static bool touchPressed = false;
  bool touchState = digitalRead(BUTTON3);
  if (touchState == LOW && !touchPressed) {
    touchPressed = true;
    alarmMode = !alarmMode;
    Serial.println(alarmMode ? "ALARM ON" : "ALARM OFF");
  } else if (touchState == HIGH && touchPressed) {
    touchPressed = false;
  }

  // Nếu đang cảnh báo: relay 1 và 2 nhấp nháy
  if (alarmMode) {
    if (millis() - lastBlinkTime > blinkInterval) {
      lastBlinkTime = millis();
      blinkState = !blinkState;
      digitalWrite(RELAY1, blinkState ? LOW : HIGH);
      digitalWrite(RELAY2, blinkState ? HIGH : LOW);
    }
  } else {
    // Bình thường: xử lý nút cầu thang BUTTON1
    static bool button1Pressed = false;
    bool reading1 = digitalRead(BUTTON1);
    if (reading1 == LOW && !button1Pressed) {
      button1Pressed = true;
      toggleRelayPair(RELAY1, RELAY2);
    } else if (reading1 == HIGH && button1Pressed) {
      button1Pressed = false;
    }
  }

  // Xử lý nút cầu thang BUTTON2 (relay 3 & 4 luôn hoạt động bình thường)
  static bool button2Pressed = false;
  bool reading2 = digitalRead(BUTTON2);
  if (reading2 == LOW && !button2Pressed) {
    button2Pressed = true;
    toggleRelayPair(RELAY3, RELAY4);
  } else if (reading2 == HIGH && button2Pressed) {
    button2Pressed = false;
  }
}

void toggleRelayPair(int r1, int r2) {
  if (digitalRead(r1) == LOW) {
    digitalWrite(r1, HIGH);
    digitalWrite(r2, LOW);
  } else {
    digitalWrite(r1, LOW);
    digitalWrite(r2, HIGH);
  }
}
