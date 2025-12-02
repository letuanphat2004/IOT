#include "wifiConfig.h"
#define BLYNK_TEMPLATE_ID "TMPL6kyoWH4Rc"
#define BLYNK_TEMPLATE_NAME "Dieu khien thiet bi"
#define BLYNK_AUTH_TOKEN "iPIamWga2Ovce4nT1U_448lMExk3XIqC"

#include <BlynkSimpleEsp32.h>
#include <TimeLib.h>
#include <WidgetRTC.h>

bool blynkConnect = 0;
WidgetRTC rtc;

// --- THÊM: Widget Terminal để hiện chữ lên điện thoại ---
WidgetTerminal terminal(V9);

// --- 1. CẤU HÌNH PHẦN CỨNG ---
const int ledPins[] = {14, 27, 26, 25}; // Thứ tự: Đèn 1, 2, 3, 4
const int numLeds = 4;

// Mảng lưu trạng thái BẬT/TẮT của đèn (True = Sáng, False = Tắt)
bool ledState[numLeds] = {false, false, false, false};

// --- 2. BIẾN HIỆU ỨNG SÓNG (V5) ---
// (Vẫn giữ lại hiệu ứng sóng nếu bạn muốn dùng)
bool waveEffectActive = false;           
unsigned long lastWaveStepTime = 0;      
const long waveStepInterval = 200;       
int currentWaveLed = 0;                  

// --- 3. BIẾN HẸN GIỜ ---
long timeOnSeconds = -1;  
long timeOffSeconds = -1; 
int targetLamp = 1; // Mặc định hẹn giờ cho Đèn 1


// --- HÀM HỖ TRỢ: Gửi thông báo ---
void logToPhone(String msg) {
  Serial.println(msg);      
  terminal.println(msg);    
  terminal.flush();         
}

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
  Blynk.syncVirtual(V1, V2, V3, V4, V5, V6, V7, V8);
  rtc.begin(); 
  terminal.clear();
  terminal.println(F("--- HE THONG SAN SANG (CHE DO TINH) ---"));
  terminal.flush();
}

// --- CÁC HÀM ĐIỀU KHIỂN (V1-V4) - CHẾ ĐỘ SÁNG CỐ ĐỊNH ---
void updateLedState(int index, int value) {
  // Cập nhật trạng thái logic
  ledState[index] = (value == 1);
  
  // Nếu KHÔNG chạy hiệu ứng sóng thì bật/tắt đèn vật lý NGAY LẬP TỨC
  if (!waveEffectActive) {
    if (ledState[index]) {
      digitalWrite(ledPins[index], HIGH); // Bật đèn sáng luôn
    } else {
      digitalWrite(ledPins[index], LOW);  // Tắt đèn luôn
    }
  }
}

BLYNK_WRITE(V1) { updateLedState(0, param.asInt()); }
BLYNK_WRITE(V2) { updateLedState(1, param.asInt()); }
BLYNK_WRITE(V3) { updateLedState(2, param.asInt()); }
BLYNK_WRITE(V4) { updateLedState(3, param.asInt()); }

// --- ĐIỀU KHIỂN HIỆU ỨNG SÓNG (V5) ---
BLYNK_WRITE(V5) {
  waveEffectActive = (param.asInt() == 1);
  if (waveEffectActive) {
     // Khi bắt đầu sóng: Tắt hết đèn nền để chạy hiệu ứng
     for(int i=0; i<numLeds; i++) digitalWrite(ledPins[i], LOW);
     currentWaveLed = 0;
     lastWaveStepTime = millis();
     logToPhone("-> Che do SONG: BAT");
  } else {
     // Khi tắt sóng: Tắt hết đèn
     for(int i=0; i<numLeds; i++) {
       digitalWrite(ledPins[i], LOW);
       ledState[i] = false; // Reset trạng thái logic về tắt
       // Cập nhật lại nút trên App về OFF
       int vPin = i + 1; // V1, V2... tương ứng index 0, 1...
       Blynk.virtualWrite(vPin, 0);
     }
     logToPhone("-> Che do SONG: TAT");
  }
}

// --- XỬ LÝ NHẬP GIỜ (V6, V7, V8) ---
BLYNK_WRITE(V6) {
  String inputStr = param.asStr(); 
  long result = parseTimeStringToSeconds(inputStr);
  if (result != -1) {
    timeOnSeconds = result;
    logToPhone("-> Hen gio BAT: " + inputStr);
  } else {
    timeOnSeconds = -1;
    logToPhone("-> Huy hen gio BAT");
  }
}

BLYNK_WRITE(V7) {
  String inputStr = param.asStr();
  long result = parseTimeStringToSeconds(inputStr);
  if (result != -1) {
    timeOffSeconds = result;
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

// --- LOGIC HẸN GIỜ ---
void handleTimer() {
  if (year() < 2020) return; 

  long currentMinutes = hour() * 60 + minute();
  long targetOnMinutes = (timeOnSeconds != -1) ? timeOnSeconds / 60 : -1;
  long targetOffMinutes = (timeOffSeconds != -1) ? timeOffSeconds / 60 : -1;

  int ledIndex = targetLamp - 1; 
  int vPin = targetLamp; // V1, V2, V3, V4 tương ứng số 1,2,3,4

  // 1. Kiểm tra giờ BẬT
  if (targetOnMinutes != -1) {
    if (currentMinutes == targetOnMinutes) {
      if (ledState[ledIndex] == false) {
        // Gọi hàm updateLedState để bật đèn
        updateLedState(ledIndex, 1);
        // Cập nhật nút trên App
        Blynk.virtualWrite(vPin, 1);  
        logToPhone("-> DEN GIO: BAT DEN " + String(targetLamp));
      }
    }
  }

  // 2. Kiểm tra giờ TẮT
  if (targetOffMinutes != -1) {
    if (currentMinutes == targetOffMinutes) {
      if (ledState[ledIndex] == true) {
        // Gọi hàm updateLedState để tắt đèn
        updateLedState(ledIndex, 0);
        // Cập nhật nút trên App
        Blynk.virtualWrite(vPin, 0);     
        logToPhone("-> DEN GIO: TAT DEN " + String(targetLamp));
      }
    }
  }
}

// --- LOGIC HIỆU ỨNG SÓNG ---
void handleWaveEffect() {
  if (!waveEffectActive) return;
  
  if (millis() - lastWaveStepTime >= waveStepInterval) {
    lastWaveStepTime = millis();
    // Tắt đèn cũ
    for (int i=0; i<numLeds; i++) digitalWrite(ledPins[i], LOW);
    // Bật đèn mới
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
      // Đã xóa handleBlinking() vì không dùng nữa
      handleWaveEffect();  
    }
  }
}
