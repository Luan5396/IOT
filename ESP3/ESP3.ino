// Phiên bản cập nhật đáp ứng toàn bộ tài liệu chức năng, giữ nguyên logic công tắc

#define CT1_Pin D3
#define CT3_Pin D2
#define CT2_Pin D4
#define FAN_PIN1 D5
#define FAN_PIN2 D6
#define LED_NHIET_PIN D7
#define BUZZER_PIN D8

unsigned long ct1PressTime = 0;
unsigned long ct2PressTime = 0;
unsigned long ct3PressTime = 0;
bool ct1PrevState = HIGH;
bool ct2PrevState = HIGH;
bool ct3PrevState = HIGH;
bool ledState = false;
bool fan1State = false;
bool fan2State = false;
bool servoOpen = false;

const unsigned long holdThreshold = 3000;


 
// Ngưỡng cảnh báo
#define NGUONG_NHIET_DO 38
#define NGUONG_DO_AM 80
#define NGUONG_KHI 100
#define NGUONG_KHI_BUZZER 300
#define HR_MIN 50
#define HR_MAX 65
#define HR_MAX_FAN2 100
#define HR_MIN_FAN2 60
#define SPO2_MIN 80
#define SPO2_MAX 90
#define SPO2_MIN_FAN2 95


unsigned long thoiGianVuotNguongNhiet = 0;
unsigned long thoiGianVuotNguongDoAm = 0;
unsigned long thoiGianVuotNguongGas = 0;
unsigned long thoiGianVuotNguongHR = 0;
unsigned long thoiGianVuotNguongSpO2 = 0;
const unsigned long CANH_BAO_DELAY = 5000;

void setup() {
  Serial.begin(115200);

  pinMode(CT1_Pin, INPUT);
  pinMode(CT2_Pin, INPUT);
  pinMode(LED_NHIET_PIN, OUTPUT);
  pinMode(FAN_PIN1, OUTPUT);
  pinMode(FAN_PIN2, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);

  digitalWrite(BUZZER_PIN, LOW);
  digitalWrite(LED_NHIET_PIN, LOW);
  digitalWrite(FAN_PIN1, LOW);
  digitalWrite(FAN_PIN2, LOW);

  delay(1000);
}

void loop() {
  updateSensorData();
}

void updateSensorData() {
  static float temperature = 0;
  static float humidity = 0;
  static int gasValue = 0;
  static int heartRate = 55;
  static int spo2 = 85;

if (Serial.available()) {
  String input = Serial.readStringUntil('\n');
  input.trim();
  if (input.startsWith("T")) {
    temperature = input.substring(1).toFloat();
    Serial.print("Cảm biến nhiệt độ = ");
    Serial.println(temperature);
  }
  else if (input.startsWith("H")) {
    humidity = input.substring(1).toFloat();
    Serial.print("Cảm biến độ ẩm = ");
    Serial.println(humidity);
  }
  else if (input.startsWith("G")) {
    gasValue = input.substring(1).toInt();
    Serial.print("Cảm biến khí gas = ");
    Serial.println(gasValue);
  }
  else if (input.startsWith("R")) {
    heartRate = input.substring(1).toInt();
    Serial.print("Cảm biến nhịp tim = ");
    Serial.println(heartRate);
  }
  else if (input.startsWith("S")) {
    spo2 = input.substring(1).toInt();
    Serial.print("Cảm biến SpO2 = ");
    Serial.println(spo2);
  }
}


  unsigned long now = millis();

  bool vuotNhiet = temperature > NGUONG_NHIET_DO;
  bool vuotDoAm = humidity > NGUONG_DO_AM;
  bool vuotGas = gasValue > NGUONG_KHI;
  bool vuotGasBuzzer = gasValue > NGUONG_KHI_BUZZER;
  bool vuotHR = (heartRate < HR_MIN || heartRate > HR_MAX);
  bool vuotHR_Fan2 = (heartRate < HR_MIN_FAN2 || heartRate > HR_MAX_FAN2);
  bool vuotSpO2 = (spo2 < SPO2_MIN || spo2 > SPO2_MAX);
  bool vuotSpO2_Fan2 = (spo2 < SPO2_MIN_FAN2);
  

  auto checkTime = [&](bool dk, unsigned long &t) {
    if (dk) {
      if (t == 0) t = now;
      return (now - t >= CANH_BAO_DELAY);
    } else {
      t = 0;
      return false;
    }
  };

  bool cbFan1 = checkTime(vuotNhiet || vuotDoAm, thoiGianVuotNguongNhiet);
  bool cbFan2 = checkTime(vuotGas, thoiGianVuotNguongGas) ||
                checkTime(vuotHR_Fan2, thoiGianVuotNguongHR) ||
                checkTime(vuotSpO2_Fan2, thoiGianVuotNguongSpO2);
  bool cbBuzzer = checkTime(vuotGasBuzzer, thoiGianVuotNguongGas) ||
                  checkTime(vuotHR, thoiGianVuotNguongHR) ||
                  checkTime(vuotSpO2, thoiGianVuotNguongSpO2);

  // Đèn: mức cảnh báo → SOS/nhấp nháy, ưu tiên
  if (cbFan1 || cbFan2) {
    digitalWrite(LED_NHIET_PIN, (now / 200) % 6 < 3);
  } else if (vuotNhiet || vuotDoAm || vuotHR || vuotSpO2) {
    digitalWrite(LED_NHIET_PIN, (now / 500) % 2 == 0);
  } else {
    digitalWrite(LED_NHIET_PIN, ledState ? HIGH : LOW);
  }

  digitalWrite(FAN_PIN1, cbFan1 ? HIGH : fan1State ? HIGH : LOW);
  digitalWrite(FAN_PIN2, cbFan2 ? HIGH : fan2State ? HIGH : LOW);
  digitalWrite(BUZZER_PIN, cbBuzzer ? HIGH : LOW);

  // Công tắc giữ nguyên như yêu cầu
  handleButtons();
}

void handleButtons() {
  bool ct1 = digitalRead(CT1_Pin);
  bool ct2 = digitalRead(CT2_Pin);
  bool ct3 = digitalRead(CT3_Pin);
  unsigned long now = millis();

  if (ct1 == LOW && ct1PrevState == HIGH) ct1PressTime = now;
  else if (ct1 == HIGH && ct1PrevState == LOW) {
    if (now - ct1PressTime >= holdThreshold) {
      fan1State = !fan1State;
      Serial.println(fan1State ? "FAN 1 ON" : "FAN 1 OFF");
    } else {
      fan1State = !fan1State;
      Serial.println(fan1State ? "FAN 1 ON" : "FAN 1 OFF");
    }
  }
  ct1PrevState = ct1;

  if (ct2 == LOW && ct2PrevState == HIGH) ct2PressTime = now;
  else if (ct2 == HIGH && ct2PrevState == LOW) {
    if (now - ct2PressTime >= holdThreshold) {
      fan2State = !fan2State;
      Serial.println(fan2State ? "FAN 2 ON" : "FAN 2 OFF");
    } else {
      fan2State = !fan2State;
      Serial.println(fan2State ? "FAN 2 ON" : "FAN 2 OFF");
    }
  }
  ct2PrevState = ct2;
  
  if (ct3 == LOW && ct3PrevState == HIGH) ct3PressTime = now;
  else if (ct3 == HIGH && ct3PrevState == LOW) {
    if (now - ct3PressTime >= holdThreshold) {
      ledState = !ledState;
      Serial.println(ledState ? "LED ON" : "LED OFF");
    } else {
      ledState = !ledState;
      Serial.println(ledState ? "LED ON" : "LED OFF");
    }
  }
  ct3PrevState = ct3;
}