import speech_recognition as sr
import requests
import time
import re  # Thư viện xử lý chuỗi (Regular Expression) để tìm giờ

# --- CẤU HÌNH ---
BLYNK_TOKEN = "iPIamWga2Ovce4nT1U_448lMExk3XIqC"
SERVER = "sgp1.blynk.cloud"


# Hàm tạo link API nhanh
def make_url(pin, value):
    return f"https://{SERVER}/external/api/update?token={BLYNK_TOKEN}&{pin}={value}"


# Hàm gửi lệnh lên Blynk (có in log)
def send_blynk(pin, value, msg):
    try:
        print(f"--> {msg}...")
        requests.get(make_url(pin, value))
    except:
        print("Lỗi kết nối mạng khi gửi lệnh!")


# Hàm trích xuất thời gian từ câu nói
# Ví dụ: "mười tám giờ ba mươi" -> "18:30"
# Ví dụ: "7 giờ 15" -> "07:15"
def extract_time_from_text(text):
    # Tìm các mẫu câu như: "18 giờ 30", "7:15", "18h30"
    # Google thường trả về số (18) hoặc chữ (mười tám).
    # Ở đây ta ưu tiên bắt dạng số mà Google Speech thường trả về.

    # Pattern: Tìm 1 hoặc 2 chữ số, theo sau là chữ "giờ" hoặc ":", theo sau là 1 hoặc 2 chữ số
    match = re.search(r'(\d{1,2})\s*(?:giờ|:|h)\s*(\d{1,2})', text)

    if match:
        hour = int(match.group(1))
        minute = int(match.group(2))

        # Định dạng lại thành HH:MM (ví dụ 7:5 -> 07:05)
        return f"{hour:02d}:{minute:02d}"

    return None


# Khởi tạo mic
r = sr.Recognizer()
mic = sr.Microphone()

print("--- KHỞI ĐỘNG TRỢ LÝ ẢO SMART HOME (CÓ HẸN GIỜ) ---")
print("Đang cân chỉnh tiếng ồn nền...")
with mic as source:
    r.adjust_for_ambient_noise(source)
print("Đã sẵn sàng! Hãy nói lệnh...")
print("Ví dụ: 'Hẹn giờ bật đèn 2 lúc 18 giờ 30'")

while True:
    text = ""
    with mic as source:
        try:
            print("\nĐang nghe...")
            audio = r.listen(source, timeout=5, phrase_time_limit=8)  # Tăng thời gian nghe lên xíu

            print("Đang xử lý...")
            text = r.recognize_google(audio, language="vi-VN")
            text = text.lower()
            print(f"==> Bạn nói: '{text}'")

            # --- KIỂM TRA XEM CÓ THỜI GIAN TRONG CÂU NÓI KHÔNG ---
            time_str = extract_time_from_text(text)

            # --- XỬ LÝ LỆNH HẸN GIỜ ---
            # Nếu có từ khóa "hẹn giờ", "đặt lịch" HOẶC tìm thấy giờ giấc trong câu nói
            if "hẹn giờ" in text or "đặt lịch" in text or time_str:
                print("--- ĐANG XỬ LÝ HẸN GIỜ ---")

                # 1. Xác định ĐÈN nào (Gửi V8)
                target_lamp = 0
                if "đèn 1" in text or "đèn một" in text:
                    target_lamp = 1
                elif "đèn 2" in text or "đèn hai" in text:
                    target_lamp = 2
                elif "đèn 3" in text or "đèn ba" in text:
                    target_lamp = 3
                elif "đèn 4" in text or "đèn bốn" in text:
                    target_lamp = 4

                if target_lamp == 0:
                    print("Lỗi: Không nghe rõ muốn hẹn giờ cho đèn nào.")
                    continue  # Bỏ qua vòng lặp này

                # 2. Xác định hành động BẬT hay TẮT (Gửi V6 hoặc V7)
                pin_to_set = ""
                action_name = ""
                if "bật" in text:
                    pin_to_set = "v6"
                    action_name = "BẬT"
                elif "tắt" in text:
                    pin_to_set = "v7"
                    action_name = "TẮT"
                else:
                    print("Lỗi: Không biết là hẹn giờ Bật hay Tắt.")
                    continue

                # 3. Sử dụng thời gian đã trích xuất ở trên
                if time_str:
                    # THỰC HIỆN GỬI LỆNH THEO TRÌNH TỰ

                    # Bước A: Chọn đèn trước (V8)
                    send_blynk("v8", target_lamp, f"Chọn đèn {target_lamp}")

                    # Nghỉ 0.5 giây để ESP32 kịp xử lý việc chuyển đèn
                    time.sleep(0.5)

                    # Bước B: Gửi thời gian (V6 hoặc V7)
                    send_blynk(pin_to_set, time_str, f"Đặt giờ {action_name} lúc {time_str}")

                    print(f"==> THÀNH CÔNG: Đã hẹn {action_name} đèn {target_lamp} vào lúc {time_str}")
                else:
                    print("Lỗi: Không nghe rõ thời gian (Hãy nói rõ: '...lúc 10 giờ 30')")


            # --- CÁC LỆNH ĐIỀU KHIỂN TRỰC TIẾP (CŨ) ---
            # Chỉ chạy vào đây nếu KHÔNG có hẹn giờ

            # Xử lý ĐÈN 1
            elif "đèn 1" in text or "đèn một" in text:
                if "bật" in text:
                    send_blynk("v1", 1, "Bật đèn 1")
                elif "tắt" in text:
                    send_blynk("v1", 0, "Tắt đèn 1")

            # Xử lý ĐÈN 2
            elif "đèn 2" in text or "đèn hai" in text:
                if "bật" in text:
                    send_blynk("v2", 1, "Bật đèn 2")
                elif "tắt" in text:
                    send_blynk("v2", 0, "Tắt đèn 2")

            # Xử lý ĐÈN 3
            elif "đèn 3" in text or "đèn ba" in text:
                if "bật" in text:
                    send_blynk("v3", 1, "Bật đèn 3")
                elif "tắt" in text:
                    send_blynk("v3", 0, "Tắt đèn 3")

            # Xử lý ĐÈN 4
            elif "đèn 4" in text or "đèn bốn" in text:
                if "bật" in text:
                    send_blynk("v4", 1, "Bật đèn 4")
                elif "tắt" in text:
                    send_blynk("v4", 0, "Tắt đèn 4")

            # Xử lý SÓNG
            elif "sóng" in text:
                if "bật" in text:
                    send_blynk("v5", 1, "Bật hiệu ứng Sóng")
                elif "tắt" in text:
                    send_blynk("v5", 0, "Tắt hiệu ứng")

            # TẮT HẾT
            elif "tắt hết" in text or "đi ngủ" in text:
                print("--> Tắt toàn bộ hệ thống...")
                send_blynk("v5", 0, "Tắt sóng")
                send_blynk("v1", 0, "Tắt đèn 1")
                send_blynk("v2", 0, "Tắt đèn 2")
                send_blynk("v3", 0, "Tắt đèn 3")
                send_blynk("v4", 0, "Tắt đèn 4")

            # HỦY HẸN GIỜ (Tính năng phụ)
            elif "hủy hẹn giờ" in text or "xóa lịch" in text:
                print("--> Đang hủy toàn bộ lịch hẹn...")
                send_blynk("v6", "-1", "Hủy giờ Bật")
                send_blynk("v7", "-1", "Hủy giờ Tắt")

            else:
                print("Không tìm thấy lệnh phù hợp.")

        except sr.UnknownValueError:
            print("...")  # Không nghe rõ thì im lặng đợi lệnh tiếp
        except sr.RequestError:
            print("Lỗi kết nối mạng Google API.")
        except Exception as e:
            print(f"Có lỗi: {e}")