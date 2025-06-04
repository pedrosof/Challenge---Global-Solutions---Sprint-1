#include <Wire.h>
#include <WiFi.h>
#include <HTTPClient.h>

// === Sensores ===
const int SOIL_MOISTURE_PIN = 34;   // Sensor de umidade do solo
const int RAIN_SENSOR_PIN = 35;     // Sensor de chuva (analógico)
const int MPU_ADDR = 0x68;          // MPU6050 padrão
const int LED_ALERT_PIN = 19;       // LED azul

// === Wi-Fi ===
char* WIFI_NAME = "Wokwi-GUEST";    // Substitua conforme sua rede
char* WIFI_PASSWORD = "";
WiFiClient client;

// === Backend Flask ===
String backendURL = "http://192.168.1.100:5000/prever";  // Altere com o IP real do servidor

// === Variáveis MPU6050 ===
int16_t ax, ay, az;

void setup() {
  Serial.begin(115200);
  Wire.begin();  // I2C (SDA = 21, SCL = 22)
  WiFi.begin(WIFI_NAME, WIFI_PASSWORD);

  Serial.println("Conectando ao Wi-Fi...");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  Serial.println("Wi-Fi conectado. IP: " + String(WiFi.localIP()));

  pinMode(LED_ALERT_PIN, OUTPUT);
  digitalWrite(LED_ALERT_PIN, LOW);

  // Inicializa MPU6050
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x6B);  // registrador PWR_MGMT_1
  Wire.write(0);     // ativa o MPU
  Wire.endTransmission(true);
}

void loop() {
  // === Sensor de umidade do solo ===
  int soilRaw = analogRead(SOIL_MOISTURE_PIN);
  int soilMoisturePercentage = map(soilRaw, 0, 4095, 0, 100);

  // === Sensor de chuva ===
  int rainRaw = analogRead(RAIN_SENSOR_PIN);
  int rainPercentage = map(rainRaw, 0, 4095, 100, 0);  // inverso: +chuva = +molhado = valor baixo

  // === MPU6050: leitura de inclinação ===
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x3B);  // ACCEL_XOUT_H
  Wire.endTransmission(false);
  Wire.requestFrom(MPU_ADDR, 6, true);

  ax = Wire.read() << 8 | Wire.read();
  ay = Wire.read() << 8 | Wire.read();
  az = Wire.read() << 8 | Wire.read();

  float ax_g = ax / 16384.0;
  float inclinacao = ax_g * 90.0;

  // === Serial Monitor ===
  Serial.println("-------------");
  Serial.print("Umidade do solo: ");
  Serial.print(soilMoisturePercentage);
  Serial.println("%");

  Serial.print("Chuva: ");
  Serial.print(rainPercentage);
  Serial.println("%");

  Serial.print("Inclinação (X): ");
  Serial.print(inclinacao);
  Serial.println("°");

  // === Envio para backend ===
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    http.begin(backendURL);
    http.addHeader("Content-Type", "application/json");

    String json = "{\"umidade\": " + String(soilMoisturePercentage) +
                  ", \"inclinacao\": " + String(inclinacao, 2) +
                  ", \"chuva\": " + String(rainPercentage) + "}";

    int httpResponseCode = http.POST(json);

    if (httpResponseCode > 0) {
      String response = http.getString();
      Serial.print("Resposta do servidor: ");
      Serial.println(response);

      // Aciona LED se risco alto
      if (response.indexOf("\"risco\": \"alto\"") != -1) {
        digitalWrite(LED_ALERT_PIN, HIGH);
        Serial.println("ALERTA: RISCO DE DESLIZAMENTO!");
      } else {
        digitalWrite(LED_ALERT_PIN, LOW);
      }

    } else {
      Serial.print("Erro HTTP: ");
      Serial.println(httpResponseCode);
    }

    http.end();
  } else {
    Serial.println("Wi-Fi desconectado.");
  }

  delay(15000);  // 15 segundos
}
