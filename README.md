# 🪑 IoT Tabanlı Akıllı Minder & Duruş Bozukluğu Takip Sistemi

> **Duruş Bozukluğu Tespiti ve Çalışma Takip Asistanlığı**

[cite_start]Bu proje, masa başı çalışanların ve öğrencilerin duruş bozukluklarını tespit etmek, oturma alışkanlıklarını analiz etmek ve hareketsizliğe bağlı sağlık sorunlarını önlemek amacıyla geliştirilmiş bir IoT (Nesnelerin İnterneti) sistemidir[cite: 10, 11].

## 🎯 Projenin Amacı
Kullanıcının koltuktaki ağırlık dağılımını analiz ederek;
* **Yanlış duruşlarda** (sağa/sola yatık, kambur) anlık geri bildirim verir.
* [cite_start]**Uzun süreli hareketsizlikte** (30 dk+) kullanıcıyı uyararak mola vermesini sağlar[cite: 11, 42].
* Verileri buluta kaydederek uzun vadeli oturma alışkanlığı raporları sunar.

## 🌟 Özellikler
* [cite_start]**Hassas Algılama:** 16-bit ADC ile milimetrik basınç değişimi tespiti[cite: 26].
* [cite_start]**Anlık Bildirimler (Telegram):** Duruş bozukluğu veya mola zamanı geldiğinde Telegram Bot üzerinden uyarı[cite: 68].
* [cite_start]**Bulut Entegrasyonu (Firebase):** Anlık verilerin 0.5-1 sn gecikme ile senkronizasyonu[cite: 48].
* [cite_start]**Veri Analizi (ThingSpeak):** Günlük oturma süreleri ve duruş kodlarının grafiksel takibi[cite: 87].
* [cite_start]**Gelişmiş Algoritma:** "Debounce" mantığı ile anlık hareketlerden kaynaklı hatalı "kalktı" verilerinin engellenmesi[cite: 35].

## 🛠 Donanım Bileşenleri
[cite_start]Bu proje aşağıdaki donanımlar kullanılarak geliştirilmiştir[cite: 13, 14, 15, 16, 17]:
* **NodeMCU ESP8266:** Ana kontrolcü ve Wi-Fi modülü.
* **ADS1115 (16-Bit ADC):** Yüksek hassasiyetli analog-dijital dönüştürücü.
* **FSR400 (x4):** Kuvvet algılayıcı basınç sensörleri.
* **Aktif Buzzer:** Sesli uyarı birimi.
* **Güç Kaynağı:** Powerbank veya USB adaptör.

## 🔌 Devre Şeması ve Bağlantılar
[cite_start]Sensörlerin ve modüllerin NodeMCU bağlantı şeması aşağıdaki gibidir[cite: 19]:

| Bileşen | Pin | Bağlantı Yeri (NodeMCU / Güç) |
| :--- | :--- | :--- |
| **ADS1115** | VDD | 3.3V (veya 5V) |
| **ADS1115** | GND | GND |
| **ADS1115** | SCL | D1 (GPIO 5) |
| **ADS1115** | SDA | D2 (GPIO 4) |
| **FSR 1 (Sol Ön)** | Uç 1 | ADS1115 A0 |
| **FSR 2 (Sağ Ön)** | Uç 1 | ADS1115 A1 |
| **FSR 3 (Sol Arka)** | Uç 1 | ADS1115 A2 |
| **FSR 4 (Sağ Arka)** | Uç 1 | ADS1115 A3 |
| **Tüm FSR'ler** | Uç 2 | 3.3V ve 10kΩ Direnç ile GND (Gerilim Bölücü) |

## 💻 Yazılım ve Teknolojiler
* [cite_start]**Dil:** C++ (Arduino IDE) [cite: 23]
* **Platformlar:**
    * Firebase Realtime Database (Anlık veri akışı)
    * Telegram Bot API (Bildirim sistemi)
    * ThingSpeak (Grafiksel raporlama)
* **Kütüphaneler:** `UniversalTelegramBot`, `ESP8266WiFi`, `FirebaseArduino` (veya alternatifi).

## 🚀 Kurulum ve Kullanım
1.  **Donanım Kurulumu:** Devre şemasına uygun olarak sensörleri mindere yerleştirin ve bağlantıları yapın.
2.  **Kütüphaneler:** Arduino IDE üzerinden gerekli kütüphaneleri yükleyin.
3.  **Konfigürasyon:** Kod içerisindeki şu alanları kendi bilgilerinizle doldurun:
    * `WIFI_SSID` & `WIFI_PASSWORD`
    * `FIREBASE_HOST` & `FIREBASE_AUTH`
    * `BOT_TOKEN` (Telegram) & `CHAT_ID`
    * `ThingSpeak Channel ID` & `API Key`
4.  **Yükleme:** Kodu NodeMCU kartına yükleyin.
5.  **Test:** Seri port ekranından (Baud: 115200) sensör verilerini ve bağlantı durumunu kontrol edin.

## 📊 Algoritma Mantığı
[cite_start]Sistem 4 sensörden gelen veriyi karşılaştırarak duruşu analiz eder[cite: 30, 31, 32]:
* **Sağa/Sola Yatık:** Sağ ve sol sensör grupleri arasındaki fark `TOLERANS` değerini aşarsa.
* **Kambur:** Ön sensörlerin toplamı arka sensörlerden fazlaysa.
* **Dik/Rahat:** Farklar minimal ise veya arka sensör yükü dengeli ise.
* [cite_start]**Mola Uyarısı:** 30 saniye (test için) boyunca hareket olmazsa alarm tetiklenir[cite: 42].

## 👥 Emeği Geçenler
* **Mustafa Can AVCI** 
---
*Bu proje sakarya Üniversitesi Bilgisayar Mühendisliği Iot dersi kapsamında geliştirilmiştir.*
