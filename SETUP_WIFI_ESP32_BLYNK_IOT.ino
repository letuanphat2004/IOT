#include "wifiConfig.h"
#define BLYNK_TEMPLATE_ID "TMPL6kyoWH4Rc"
#define BLYNK_TEMPLATE_NAME "Dieu khien thiet bi"
#define BLYNK_AUTH_TOKEN "iPIamWga2Ovce4nT1U_448lMExk3XIqC"

#include <BlynkSimpleEsp32.h>
#include <TimeLib.h>
#include <WidgetRTC.h>

bool blynkConnect = 0;
WidgetRTC rtc;

// --- 1. CẤU HÌNH PHẦN CỨNG ---
const int ledPins[] = {14, 27, 26, 25}; // Thứ tự: Đèn 1, 2, 3, 4
const int numLeds = 4;
bool ledBlinkingState[numLeds] = {false, false, false, false};

// --- 2. BIẾN NHẤP NHÁY & SÓNG ---
unsigned long lastBlinkTime = 0;
const long blinkInterval = 500;
bool blinkStatus = LOW;

bool waveEffectActive = false;           
unsigned long lastWaveStepTime = 0;      
const long waveStepInterval = 200;       
int currentWaveLed = 0;                  

// --- 3. BIẾN HẸN GIỜ ---
long timeOnSeconds = -1;  
long timeOffSeconds = -1; 
int targetLamp = 1; // Mặc định hẹn giờ cho Đèn 1 (nếu không điền gì)


// --- HÀM HỖ TRỢ: ĐỔI CHUỖI SANG GIÂY ---
long parseTimeStringToSeconds(String timeStr) {
  if (timeStr == "-1" || timeStr.length() < 3) return -1;
  int colonIndex = timeStr.indexOf(':');
  if (colonIndex == -1) return -1;
  String hourStr = timeStr.substring(0, colonIndex);
  String minStr = timeStr.substring(colonIndex + 1);
  int h = hourStr.toInt();
  int m = minStr.toInt();
  if (h < 0 || h > 23 || m < 0 || m > 59) return -1;
  return h * 3600 + m * 60;
}

BLYNK_CONNECTED() {
  // Đồng bộ tất cả, bao gồm cả V8
  Blynk.syncVirtual(V1, V2, V3, V4, V5, V6, V7, V8);
  rtc.begin(); 
}

// --- CÁC HÀM ĐIỀU KHIỂN (V1-V4) ---
void updateLedState(int index, int value) {
  ledBlinkingState[index] = (value == 1);
  if (!ledBlinkingState[index] && !waveEffectActive) {
    digitalWrite(ledPins[index], LOW);
  }
}

BLYNK_WRITE(V1) { updateLedState(0, param.asInt()); }
BLYNK_WRITE(V2) { updateLedState(1, param.asInt()); }
BLYNK_WRITE(V3) { updateLedState(2, param.asInt()); }
BLYNK_WRITE(V4) { updateLedState(3, param.asInt()); }

BLYNK_WRITE(V5) {
  waveEffectActive = (param.asInt() == 1);
  if (waveEffectActive) {
     for(int i=0; i<numLeds; i++) digitalWrite(ledPins[i], LOW);
     currentWaveLed = 0;
     lastWaveStepTime = millis();
  } else {
     for(int i=0; i<numLeds; i++) digitalWrite(ledPins[i], LOW);
  }
}

// --- XỬ LÝ NHẬP GIỜ (V6, V7) ---
BLYNK_WRITE(V6) {
  String inputStr = param.asStr(); 
  long result = parseTimeStringToSeconds(inputStr);
  if (result != -1) {
    timeOnSeconds = result;
    Serial.print("-> Hen gio BAT: "); Serial.println(inputStr);
  } else {
    timeOnSeconds = -1;
    Serial.println("-> Huy hen gio BAT");
  }
}

BLYNK_WRITE(V7) {
  String inputStr = param.asStr();
  long result = parseTimeStringToSeconds(inputStr);
  if (result != -1) {
    timeOffSeconds = result;
    Serial.print("-> Hen gio TAT: "); Serial.println(inputStr);
  } else {
    timeOffSeconds = -1;
    Serial.println("-> Huy hen gio TAT");
  }
}

// --- XỬ LÝ CHỌN ĐÈN (V8) ---
// Người dùng nhập số "1", "2", "3", "4" vào ô Text Input V8
BLYNK_WRITE(V8) {
  String inputStr = param.asStr();
  int val = inputStr.toInt(); // Chuyển chuỗi thành số
  
  // Kiểm tra xem số có hợp lệ không (chỉ chấp nhận 1, 2, 3, 4)
  if (val >= 1 && val <= 4) {
    targetLamp = val;
    Serial.print("-> Da chon den de hen gio: Den "); 
    Serial.println(targetLamp);
  } else {
    Serial.println("-> Loi: Chi duoc nhap so tu 1 den 4!");
  }
}

// --- LOGIC HẸN GIỜ (Sửa đổi để hỗ trợ chọn đèn) ---
void handleTimer() {
  if (year() < 2020) return; 

  long currentMinutes = hour() * 60 + minute();
  long targetOnMinutes = (timeOnSeconds != -1) ? timeOnSeconds / 60 : -1;
  long targetOffMinutes = (timeOffSeconds != -1) ? timeOffSeconds / 60 : -1;

  // Xác định chỉ số mảng (0-3) từ số đèn (1-4)
  int ledIndex = targetLamp - 1; 
  
  // Xác định Virtual Pin tương ứng (V1 là 1, V2 là 2...)
  // Trong Blynk library, V1 thực chất là số 1, V2 là 2...
  int vPin = targetLamp; 

  // 1. Kiểm tra giờ BẬT
  if (targetOnMinutes != -1) {
    if (currentMinutes == targetOnMinutes) {
      if (ledBlinkingState[ledIndex] == false) {
        ledBlinkingState[ledIndex] = true; 
        
        // Cập nhật trạng thái nút trên App (V1, V2, V3 hoặc V4)
        Blynk.virtualWrite(vPin, 1);  
        
        Serial.print("-> DEN GIO: BAT DEN ");
        Serial.println(targetLamp);
      }
    }
  }

  // 2. Kiểm tra giờ TẮT
  if (targetOffMinutes != -1) {
    if (currentMinutes == targetOffMinutes) {
      if (ledBlinkingState[ledIndex] == true) {
        ledBlinkingState[ledIndex] = false;   
        
        // Cập nhật trạng thái nút trên App
        Blynk.virtualWrite(vPin, 0);     
        digitalWrite(ledPins[ledIndex], LOW); 
        
        Serial.print("-> DEN GIO: TAT DEN ");
        Serial.println(targetLamp);
      }
    }
  }
}

void handleBlinking() {
  if (waveEffectActive) return; 
  bool anyBlinking = false;
  for (int i=0; i<numLeds; i++) if(ledBlinkingState[i]) anyBlinking=true;
  if (!anyBlinking) return;

  if (millis() - lastBlinkTime >= blinkInterval) {
    lastBlinkTime = millis();
    blinkStatus = !blinkStatus; 
    for (int i=0; i<numLeds; i++) {
      if(ledBlinkingState[i]) digitalWrite(ledPins[i], blinkStatus);
    }
  }
}

void handleWaveEffect() {
  if (!waveEffectActive) return;
  if (millis() - lastWaveStepTime >= waveStepInterval) {
    lastWaveStepTime = millis();
    for (int i=0; i<numLeds; i++) digitalWrite(ledPins[i], LOW);
    digitalWrite(ledPins[currentWaveLed], HIGH);
    currentWaveLed++;
    if (currentWaveLed >= numLeds) currentWaveLed = 0; 
  }
}

void setup() {
  Serial.begin(115200);
  wifiConfig.begin(); 
  for (int i = 0; i < numLeds; i++) {
    pinMode(ledPins[i], OUTPUT);
    digitalWrite(ledPins[i], LOW);
  }
  Blynk.config(BLYNK_AUTH_TOKEN, "blynk.cloud", 80);
}

void loop() {
  wifiConfig.run(); 
  if(WiFi.status() == WL_CONNECTED){
    if(blynkConnect == 0){
      if(Blynk.connect(5000)) blynkConnect = 1;
    }
    if (blynkConnect) {
      Blynk.run();
      handleTimer();       
      handleBlinking();    
      handleWaveEffect();  
    }
  }
}