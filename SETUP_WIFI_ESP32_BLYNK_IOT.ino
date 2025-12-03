#include "wifiConfig.h"
#define BLYNK_TEMPLATE_ID "TMPL6kyoWH4Rc"
#define BLYNK_TEMPLATE_NAME "Dieu khien thiet bi"
#define BLYNK_AUTH_TOKEN "iPIamWga2Ovce4nT1U_448lMExk3XIqC"

#include <BlynkSimpleEsp32.h>
#include <TimeLib.h>
#include <WidgetRTC.h>

bool blynkConnect = 0;
WidgetRTC rtc;

// Widget Terminal (V9)
WidgetTerminal terminal(V9);

// --- 1. CẤU HÌNH PHẦN CỨNG ---
const int ledPins[] = {14, 27, 26, 25}; 
const int numLeds = 4;
bool ledState[numLeds] = {false, false, false, false};

// --- 2. BIẾN HIỆU ỨNG SÓNG (V5) ---
bool waveEffectActive = false;           
unsigned long lastWaveStepTime = 0;      
const long waveStepInterval = 200;       
int currentWaveLed = 0;                  

// --- 3. BIẾN HẸN GIỜ ---
long timeOnSeconds = -1;  
long timeOffSeconds = -1; 
int targetLamp = 1; 

// --- 4. BIẾN SỬA LỖI (MỚI) ---
// Dùng để lưu phút đã thực hiện lệnh, tránh spam lệnh liên tục
int lastOnMinute = -1; 
int lastOffMinute = -1;


// --- HÀM LOG ---
void logToPhone(String msg) {
  Serial.println(msg);      
  terminal.println(msg);    
  terminal.flush();         
}

// --- HÀM XỬ LÝ CHUỖI THỜI GIAN ---
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
  Blynk.syncVirtual(V1, V2, V3, V4, V5, V6, V7, V8);
  rtc.begin(); 
  terminal.clear();
  terminal.println(F("--- HE THONG DA KET NOI ---"));
  terminal.flush();
}

// --- HÀM CẬP NHẬT TRẠNG THÁI ĐÈN ---
void updateLedState(int index, int value) {
  ledState[index] = (value == 1);
  
  if (!waveEffectActive) {
    if (ledState[index]) {
      digitalWrite(ledPins[index], HIGH);
    } else {
      digitalWrite(ledPins[index], LOW);
    }
  }
}

// --- CÁC NÚT ĐIỀU KHIỂN ---
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
     logToPhone("-> Che do SONG: BAT");
  } else {
     for(int i=0; i<numLeds; i++) {
       digitalWrite(ledPins[i], LOW);
       ledState[i] = false;
       Blynk.virtualWrite(i + 1, 0); // Reset nút trên app
     }
     logToPhone("-> Che do SONG: TAT");
  }
}

// --- NHẬP GIỜ BẬT (V6) ---
BLYNK_WRITE(V6) {
  String inputStr = param.asStr(); 
  long result = parseTimeStringToSeconds(inputStr);
  if (result != -1) {
    timeOnSeconds = result;
    lastOnMinute = -1; // Reset trạng thái kích hoạt khi đặt giờ mới
    logToPhone("-> Hen gio BAT: " + inputStr);
  } else {
    timeOnSeconds = -1;
    logToPhone("-> Huy hen gio BAT");
  }
}

// --- NHẬP GIỜ TẮT (V7) ---
BLYNK_WRITE(V7) {
  String inputStr = param.asStr();
  long result = parseTimeStringToSeconds(inputStr);
  if (result != -1) {
    timeOffSeconds = result;
    lastOffMinute = -1; // Reset trạng thái kích hoạt
    logToPhone("-> Hen gio TAT: " + inputStr);
  } else {
    timeOffSeconds = -1;
    logToPhone("-> Huy hen gio TAT");
  }
}

BLYNK_WRITE(V8) {
  String inputStr = param.asStr();
  int val = inputStr.toInt(); 
  if (val >= 1 && val <= 4) {
    targetLamp = val;
    logToPhone("-> Chon hen gio: Den " + String(targetLamp));
  } else {
    logToPhone("-> Loi: Chi nhap so 1-4!");
  }
}

// --- LOGIC HẸN GIỜ (ĐÃ SỬA LỖI KHÓA ĐÈN) ---
void handleTimer() {
  if (year() < 2020) return; 

  long currentMinutes = hour() * 60 + minute();
  
  // Tính phút hẹn (nếu có)
  long targetOnMinutes = (timeOnSeconds != -1) ? timeOnSeconds / 60 : -1;
  long targetOffMinutes = (timeOffSeconds != -1) ? timeOffSeconds / 60 : -1;

  int ledIndex = targetLamp - 1; 
  int vPin = targetLamp; 

  // --- KIỂM TRA GIỜ BẬT ---
  if (targetOnMinutes != -1) {
    // Chỉ kích hoạt nếu đúng giờ VÀ chưa kích hoạt trong phút này
    if (currentMinutes == targetOnMinutes && currentMinutes != lastOnMinute) {
        
        updateLedState(ledIndex, 1); // Bật đèn
        Blynk.virtualWrite(vPin, 1); // Cập nhật App
        logToPhone("-> DA DEN GIO: BAT DEN " + String(targetLamp));
        
        // Đánh dấu là đã làm xong việc cho phút này
        lastOnMinute = currentMinutes; 
    }
  }

  // --- KIỂM TRA GIỜ TẮT ---
  if (targetOffMinutes != -1) {
    // Chỉ kích hoạt nếu đúng giờ VÀ chưa kích hoạt trong phút này
    if (currentMinutes == targetOffMinutes && currentMinutes != lastOffMinute) {
        
        updateLedState(ledIndex, 0); // Tắt đèn
        Blynk.virtualWrite(vPin, 0); // Cập nhật App
        logToPhone("-> DA DEN GIO: TAT DEN " + String(targetLamp));
        
        // Đánh dấu là đã làm xong việc cho phút này
        lastOffMinute = currentMinutes;
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
      handleWaveEffect();  
    }
  }
}
