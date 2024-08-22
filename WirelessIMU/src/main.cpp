#include <Wire.h>
#include <WiFi.h>
#include <TFT_eSPI.h>
#include <ESPAsyncWebServer.h>
#include "MPU9250.h"

#define TFT_CS   5
#define TFT_RST  26
#define TFT_DC   23
#define SCREEN_WIDTH  128
#define SCREEN_HEIGHT 128

TFT_eSPI tft = TFT_eSPI();
MPU9250 imu;

// 設定熱點名稱和密碼
const char *ssid = "ESP32-AP";
const char *password = "123456789";

// 建立一個網頁伺服器
AsyncWebServer server(80);

const int sendSize = 100; // Number of samples per transmission
const int maxSamples = 4000; // Total number of samples to collect
const int samplePeriod = 10; // Collect data every 10 milliseconds

struct ImuData {
  unsigned long timestamp;
  int16_t accel_x;
  int16_t accel_y;
  int16_t accel_z;
  int16_t gyro_x;
  int16_t gyro_y;
  int16_t gyro_z;
};

ImuData raw_data[maxSamples];
int idx = 0;
unsigned long lastSampleTime = 0;

void setup() {
  Wire.begin();
  Serial.begin(115200); // Start serial communication

  // tft.init();
  // tft.fillScreen(0xFFFF);
  // tft.setTextColor(0x0000);
  // tft.setTextSize(1);

  imu.initMPU9250();
  imu.initAK8963(imu.magCalibration);

  // 設定 ESP32 為 Access Point
  WiFi.softAP(ssid, password);

  // 打印 Access Point 的 IP 地址
  IPAddress IP = WiFi.softAPIP();
  Serial.print("Access Point IP Address: ");
  Serial.println(IP);

  // 啟動網頁伺服器
  server.begin();

  // 設置處理 /imu-data 路徑的回應
  server.on("/imu-data", HTTP_GET, [](AsyncWebServerRequest *request){
    // 取得查詢參數 idx
    String idxParam = request->getParam("index") ? request->getParam("index")->value() : String(0);
    int dataIndex = idxParam.toInt();  // 轉換為整數

    int start = dataIndex * sendSize;
    int end = min(start + sendSize, maxSamples);

    String jsonData = "{\"data\":[";
    
    for (int i = start; i < end; i++) {
      if (i > start) jsonData += ",";  // Add comma between objects
      char buffer[200]; // Increase buffer size if needed

      snprintf(buffer, sizeof(buffer),
        "{\"timestamp\":%lu,"
        "\"accel_x\":%f,"
        "\"accel_y\":%f,"
        "\"accel_z\":%f,"
        "\"gyro_x\":%f,"
        "\"gyro_y\":%f,"
        "\"gyro_z\":%f}",
        raw_data[i].timestamp,
        raw_data[i].accel_x / -8192.0 + 0.03,
        raw_data[i].accel_y / -8192.0 + 0.05,
        raw_data[i].accel_z / -8192.0 + 0.01,
        raw_data[i].gyro_x / -65.5 - 2.2,
        raw_data[i].gyro_y / -65.5 + 0.2,
        raw_data[i].gyro_z / -65.5 + 1.1
      );

      jsonData += buffer;
    }
    
    jsonData += "]}";

    request->send(200, "application/json", jsonData);
  });

  // Serial.println("IMU MPU9250 Readings:");
  // Serial.println("Time(ms), AccX, AccY, AccZ, GyroX, GyroY, GyroZ, MagX, MagY, MagZ");

  Serial.println("Data collection started.");
}

void loop() {
  static unsigned long startTime = millis();
  unsigned long currentMillis = millis();

  // Collect IMU data
  if (currentMillis - lastSampleTime >= samplePeriod) {
    lastSampleTime = currentMillis;

    // Collect IMU data
    int16_t accelData[3];
    int16_t gyroData[3];

    imu.readAccelData(accelData);
    imu.readGyroData(gyroData);

    // Convert data
    raw_data[idx].timestamp = millis() - startTime;
    raw_data[idx].accel_x = accelData[0];
    raw_data[idx].accel_y = accelData[1];
    raw_data[idx].accel_z = accelData[2];
    raw_data[idx].gyro_x = gyroData[0];
    raw_data[idx].gyro_y = gyroData[1];
    raw_data[idx].gyro_z = gyroData[2];

    idx += 1;

    if (idx >= maxSamples) {
      Serial.println("Data collection complete.");
      while (true);
    }
  }
}