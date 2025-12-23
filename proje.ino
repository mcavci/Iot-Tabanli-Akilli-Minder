#include <ESP8266WiFi.h>
#include <FirebaseESP8266.h>
#include <ThingSpeak.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>
#include <Wire.h>
#include <Adafruit_ADS1X15.h>
#include <WiFiManager.h> 
#include <DNSServer.h>
#include <ESP8266WebServer.h>

// ============================================================
//  mustafa can avcı
// KULLANICI AYARLARI
// ============================================================

// --- MANUEL WIFI AYARLARI ---
#define MANUEL_SSID "***"      
#define MANUEL_PASS "***"    

#define FIREBASE_HOST "***" 
#define FIREBASE_AUTH "***"

unsigned long myChannelNumber =  ***;      
const char * myWriteAPIKey = "***";

#define BOT_TOKEN "***"
#define CHAT_ID "***"

// --- SENSÖR AYARLARI (GÜNCELLENDİ) ---
Adafruit_ADS1115 ads; 
// ESKİSİ 3000 İDİ, ŞİMDİ 800 YAPTIK Kİ %100'E RAHAT ÇIKSIN
const int MAX_DEGER = 1500;  
const int OTURMA_ESIGI = 5; // Hafif dokunmayı algılamasın, en az %5 baskı olsun

// --- SURE AYARLARI ---
unsigned long hareketsizlikLimiti = 30000; // Varsayılan 30 sn
const unsigned long THINGSPEAK_SURESI = 20000; 
const unsigned long FIREBASE_SURESI = 1000;    
const unsigned long KALKMA_TOLERANS_SURESI = 3000; 
const unsigned long TELEGRAM_KONTROL_SURESI = 1000; 

// NESNELER
FirebaseData firebaseData;
FirebaseConfig config;
FirebaseAuth auth;
WiFiClient clientThingSpeak;      // ThingSpeak icin Normal Istemci
WiFiClientSecure clientTelegram;  // Telegram icin Guvenli Istemci                   
WiFiClientSecure clientSecure; 
UniversalTelegramBot bot(BOT_TOKEN, clientSecure);

// ZAMANLAYICILAR
unsigned long zamanThingSpeak = 0;
unsigned long zamanFirebase = 0;
unsigned long zamanTelegramKontrol = 0;
unsigned long oturmaBaslangicZamani = 0;

// YENI DEGISKENLER
unsigned long kalkmaTespitZamani = 0;
bool kalkmaSuresiBasladi = false;
bool suanOturuyor = false;
bool sureUyarisiGonderildi = false; 

// ============================================================
// TELEGRAM MESAJ ISLEME
// ============================================================
void handleNewMessages(int numNewMessages) {
  for (int i = 0; i < numNewMessages; i++) {
    String chat_id = String(bot.messages[i].chat_id);
    String text = bot.messages[i].text;

    if (text.startsWith("/sure ")) {
      String sureString = text.substring(6); 
      int saniye = sureString.toInt();
      if (saniye > 0) {
        hareketsizlikLimiti = saniye * 1000; 
        String mesaj = "✅ *SÜRE GÜNCELLENDİ!*\n\n";
        mesaj += "Limit: " + String(saniye) + " sn";
        bot.sendMessage(chat_id, mesaj, "Markdown");
        Serial.print("Telegram'dan yeni sure: "); Serial.println(hareketsizlikLimiti);
      } else {
        bot.sendMessage(chat_id, "⚠️ Hata: /sure 60 seklinde yaz", "");
      }
    }
    else if (text == "/durum") {
      String mesaj = "📊 *Durum Raporu*\n";
      mesaj += "Limit: " + String(hareketsizlikLimiti / 1000) + " sn\n";
      mesaj += "Koltuk: " + String(suanOturuyor ? "DOLU" : "BOS");
      bot.sendMessage(chat_id, mesaj, "Markdown");
    }
    else if (text == "/analiz") {
      String mesaj = "📈 *BIG DATA*\n";
      mesaj += "https://thingspeak.com/channels/3202371"; 
      bot.sendMessage(chat_id, mesaj, "Markdown");
    }
    else if (text == "/start") {
      String welcome = "🤖 *Minder Botu*\n/sure 60\n/durum\n/analiz";
      bot.sendMessage(chat_id, welcome, "Markdown");
    }
  }
}

void setup() {
  Serial.begin(115200);
  Serial.println("\n--- MINDER PROJESI BASLATILIYOR ---");
  
  Wire.begin(D2, D1); 
  
  if (!ads.begin(0x48)) {
    Serial.println("HATA: ADS1115 Yok!");
  } else {
    Serial.println("ADS1115 Hazir!");
  }
  ads.setGain(GAIN_TWO);    

  WiFi.mode(WIFI_STA); 
  WiFi.begin(MANUEL_SSID, MANUEL_PASS);

  int sayac = 0;
  while (WiFi.status() != WL_CONNECTED && sayac < 20) { 
    delay(500); Serial.print("."); sayac++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi Baglandi!");
    Serial.println(WiFi.localIP());
  } else {
    WiFiManager wifiManager;
    wifiManager.autoConnect("Minder_Kurulum");
    Serial.println("\nManager ile Baglandi!");
  }

  config.host = FIREBASE_HOST;
  config.signer.tokens.legacy_token = FIREBASE_AUTH;
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);

  ThingSpeak.begin(clientThingSpeak);

  clientSecure.setInsecure(); 
  clientSecure.setBufferSizes(2048, 1024); 
  
  bot.sendMessage(CHAT_ID, "✅ Sistem Hazir! (/start)", "");
}

void loop() {
  unsigned long simdikiZaman = millis();

  // 1. TELEGRAM
  if (simdikiZaman - zamanTelegramKontrol > TELEGRAM_KONTROL_SURESI) {
    int numNewMessages = bot.getUpdates(bot.last_message_received + 1);
    while (numNewMessages) {
      handleNewMessages(numNewMessages);
      numNewMessages = bot.getUpdates(bot.last_message_received + 1);
    }
    zamanTelegramKontrol = simdikiZaman;
  }

  // 2. SENSOR OKUMA (KALIBRASYON DUZELTİLDİ)
  int16_t ham[4];
  int yuzde[4];

  for (int i = 0; i < 4; i++) {
    int cop = ads.readADC_SingleEnded(i); 
    delay(5); 
    ham[i] = ads.readADC_SingleEnded(i); 
    if (ham[i] < 0) ham[i] = 0;
    
    // YENİ HASSASİYET AYARI (Max 800 ise %100 olsun)
    yuzde[i] = map(ham[i], 0, MAX_DEGER, 0, 100);
    yuzde[i] = constrain(yuzde[i], 0, 100);
  }

  // Toleransı düşürdük ki ufak kaymaları yakalasın
  int TOLERANS = 10; // Eskiden 30 idi, şimdi daha hassas
  
  int solArka = yuzde[0]; 
  int sagArka = yuzde[1];
  int solOn = yuzde[2];   
  int sagOn = yuzde[3];
  int ortalamaYuk = (solArka + sagArka + solOn + sagOn) / 4;

  // 3. DURUS ANALIZI
  String durusDurumu = "Normal";
  int durusKodu = 1; 

  if (ortalamaYuk > OTURMA_ESIGI) {
    kalkmaSuresiBasladi = false; 
    
    int solTaraf = solArka + solOn;
    int sagTaraf = sagArka + sagOn;
    int onTaraf  = solOn + sagOn;
    int arkaTaraf = solArka + sagArka;

    // Analiz Mantığı
    if (sagTaraf > solTaraf + TOLERANS) { 
        durusDurumu = "Saga Yatik"; 
        durusKodu = 2; 
    } 
    else if (solTaraf > sagTaraf + TOLERANS) { 
        durusDurumu = "Sola Yatik"; 
        durusKodu = 3; 
    } 
    else if (onTaraf > arkaTaraf + TOLERANS) { 
        durusDurumu = "Kambur (One Egik)"; 
        durusKodu = 4; 
    }
    else if (arkaTaraf > onTaraf + TOLERANS) { 
        durusDurumu = "Rahat (Geriye Yasli)"; 
        durusKodu = 5; 
    }
    else { 
        durusDurumu = "Dik (Normal)"; 
        durusKodu = 1; 
    }
    
  } else {
    durusDurumu = "Koltuk Bos";
    durusKodu = 0;
  }

  // SERIAL MONITOR (DETAYLI)
  Serial.print("SolA:"); Serial.print(solArka);
  Serial.print(" SagA:"); Serial.print(sagArka);
  Serial.print(" SolO:"); Serial.print(solOn);
  Serial.print(" SagO:"); Serial.print(sagOn);
  Serial.print(" | ORT:"); Serial.print(ortalamaYuk);
  Serial.print(" | KOD:"); Serial.println(durusKodu); // Kodu buraya ekledik

  // 4. FIREBASE
  if (simdikiZaman - zamanFirebase > FIREBASE_SURESI) {
    Firebase.setInt(firebaseData, "/KoltukDurumu/OrtalamaYuk", ortalamaYuk);
    Firebase.setString(firebaseData, "/KoltukDurumu/Analiz", durusDurumu);
    Firebase.setInt(firebaseData, "/KoltukDurumu/DurusKodu", durusKodu); // Firebase'e de kodu ekledim
    String durumYazisi = (suanOturuyor) ? "Dolu" : "Bos";
    Firebase.setString(firebaseData, "/KoltukDurumu/Durum", durumYazisi);
    zamanFirebase = simdikiZaman;
  }

  // 5. UYARI MANTIGI
  if (ortalamaYuk > OTURMA_ESIGI) {
    if (!suanOturuyor) {
      oturmaBaslangicZamani = simdikiZaman;
      suanOturuyor = true;
      sureUyarisiGonderildi = false;
      kalkmaSuresiBasladi = false;
      
      String ilkMesaj = "🟢 *OTURMA ALGILANDI*\nSayaç: " + String(hareketsizlikLimiti / 1000) + " sn";
      bot.sendMessage(CHAT_ID, ilkMesaj, "Markdown");
    }

    if ((simdikiZaman - oturmaBaslangicZamani > hareketsizlikLimiti) && !sureUyarisiGonderildi) {
      String mesaj = "⚠️ *SÜRE DOLDU!* ⚠️\n\n";
      mesaj += "⏳ Süre bitti, lütfen hareket et.\n";
      mesaj += "📊 *Pozisyon:* " + durusDurumu + " (Kod: " + String(durusKodu) + ")";
      bot.sendMessage(CHAT_ID, mesaj, "Markdown");
      sureUyarisiGonderildi = true; 
    }

  } else {
    if (suanOturuyor) {
      if (!kalkmaSuresiBasladi) {
        kalkmaTespitZamani = simdikiZaman;
        kalkmaSuresiBasladi = true;
      }
      if (simdikiZaman - kalkmaTespitZamani > KALKMA_TOLERANS_SURESI) {
         suanOturuyor = false;
         sureUyarisiGonderildi = false; 
         kalkmaSuresiBasladi = false; 
         bot.sendMessage(CHAT_ID, "🔴 *KULLANICI KALKTI*", "Markdown");
      }
    }
  }

 // 6. THINGSPEAK (STRING CONVERSION FIX)
  if (simdikiZaman - zamanThingSpeak > THINGSPEAK_SURESI) {
    
    // BURASI KRITIK: String() icine aliyoruz ki hata olmasin
    ThingSpeak.setField(1, String(ortalamaYuk));
    ThingSpeak.setField(2, String(durusKodu));
    ThingSpeak.setField(3, String(suanOturuyor ? 1 : 0));
    
    int x = ThingSpeak.writeFields(myChannelNumber, myWriteAPIKey);
    
    Serial.print("TS Gonderiliyor... Kodlar: [");
    Serial.print(ortalamaYuk); Serial.print(", ");
    Serial.print(durusKodu); Serial.print(", ");
    Serial.print(suanOturuyor ? 1 : 0);
    Serial.print("] -> Sonuc: ");
    
    if(x == 200) {
        Serial.println("BASARILI (200)");
    } else {
        Serial.print("HATA KODU: "); Serial.println(x);
    }
    
    zamanThingSpeak = simdikiZaman;
  }
  delay(10); 
}