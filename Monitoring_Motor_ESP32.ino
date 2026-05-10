/* * Proyek: Sistem Pemeliharaan Prediktif IoT - Monitoring Motor Industri
 * Oleh: NINA (NIM: 2422017) - Teknik Komputer ITEBA
 */

#include <WiFi.h>
#include <PubSubClient.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_ADXL345_U.h>
#include <OneWire.h>
#include <DallasTemperature.h>

// Konfigurasi Pin sesuai dokumen teknis
#define ONE_WIRE_BUS 4
#define BUZZER_PIN 5 

// Threshold sesuai rancangan
const float TEMP_THRESHOLD = 70.0;
const float VIB_THRESHOLD = 2.5; 

// Konfigurasi WiFi & MQTT
const char* ssid = "WIFI_SSID_ANDA";
const char* password = "PASSWORD_WIFI_ANDA";
const char* mqtt_server = "broker.emqx.io"; 

WiFiClient espClient;
PubSubClient client(espClient);
Adafruit_ADXL345_Unified accel = Adafruit_ADXL345_Unified(12345);
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

void setup() {
  Serial.begin(115200);
  pinMode(BUZZER_PIN, OUTPUT);
  
  // Inisialisasi Sensor sesuai flowchart
  if(!accel.begin()){
    Serial.println("ADXL345 tidak terdeteksi!");
    while(1);
  }
  sensors.begin();
  
  setup_wifi();
  client.setServer(mqtt_server, 1883);
}

void setup_wifi() {
  delay(10);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) { delay(500); }
}

void loop() {
  if (!client.connected()) { reconnect(); }
  client.loop();

  // 1. Baca Data Suhu & Getaran
  sensors.requestTemperatures();
  float tempC = sensors.getTempCByIndex(0);
  
  sensors_event_t event; 
  accel.getEvent(&event);
  float vibTotal = sqrt(pow(event.acceleration.x, 2) + pow(event.acceleration.y, 2) + pow(event.acceleration.z, 2));

  // 2. Logika Thresholding (Alarm)
  if (tempC > TEMP_THRESHOLD || vibTotal > VIB_THRESHOLD) {
    digitalWrite(BUZZER_PIN, HIGH); 
    client.publish("motor/status/alert", "ANOMALI TERDETEKSI!");
  } else {
    digitalWrite(BUZZER_PIN, LOW);
  }

  // 3. Kirim data ke Cloud Dashboard
  String payload = "{\"temp\":" + String(tempC) + ",\"vibration\":" + String(vibTotal) + "}";
  client.publish("motor/data/telemetry", payload.c_str());

  Serial.println(payload);
  delay(2000); 
}

void reconnect() {
  while (!client.connected()) {
    if (client.connect("ESP32_Motor_Monitor")) {
      client.subscribe("motor/cmd");
    } else { delay(5000); }
  }
}
