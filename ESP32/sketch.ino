#include <Wire.h>
#include <WiFi.h>
#include <HTTPClient.h>

// === Sensores ===
const int SOIL_MOISTURE_PIN = 34;
const int RAIN_SENSOR_PIN = 35;
const int MPU_ADDR = 0x68;
const int LED_ACTIVITY_PIN = 19;

// === Wi-Fi ===
char* WIFI_NAME = "Wokwi-GUEST";
char* WIFI_PASSWORD = "";
WiFiClient client;

// === Backend Flask ===
String backendURL = "http://192.168.1.100:5000/prever"; // Altere conforme seu IP
String apiKey = "RLAOgaeHGa0hOumzEBV60XoNJe46BSB8ndC3awI1";  // Substitua pela sua chave real

// === MPU6050 variáveis ===
int16_t ax, ay, az;

void setup() {
  Serial.begin(115200);
  Wire.begin();  // SDA=21, SCL=22
  WiFi.begin(WIFI_NAME, WIFI_PASSWORD);

  Serial.println("Conectando ao Wi-Fi...");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWi-Fi conectado. IP: " + String(WiFi.localIP()));

  pinMode(LED_ACTIVITY_PIN, OUTPUT);
  digitalWrite(LED_ACTIVITY_PIN, LOW);

  // Inicializa MPU6050
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x6B);
  Wire.write(0);
  Wire.endTransmission(true);
}

void loop() {
  digitalWrite(LED_ACTIVITY_PIN, HIGH);  // LED indica "coletando"

  // === Leitura do sensor de umidade do solo ===
  int soilRaw = analogRead(SOIL_MOISTURE_PIN);
  int soilMoisturePercentage = map(soilRaw, 0, 4095, 0, 100);

  // === Leitura do sensor de chuva ===
  int rainRaw = analogRead(RAIN_SENSOR_PIN);
  int rainPercentage = map(rainRaw, 0, 4095, 100, 0);

  // === Leitura da inclinação via MPU6050 ===
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x3B);
  Wire.endTransmission(false);
  Wire.requestFrom(MPU_ADDR, 6, true);

  ax = Wire.read() << 8 | Wire.read();
  ay = Wire.read() << 8 | Wire.read();
  az = Wire.read() << 8 | Wire.read();

  float ax_g = ax / 16384.0;
  float inclinacao = ax_g * 90.0;

  // === Debug via Serial ===
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

  // === Envio para o backend ===
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    http.begin(backendURL);
    http.addHeader("Content-Type", "application/json");
    http.addHeader("x-api-key", apiKey);  // Envia a API Key

    String json = "{\"umidade\": " + String(soilMoisturePercentage) +
                  ", \"inclinacao\": " + String(inclinacao, 2) +
                  ", \"chuva\": " + String(rainPercentage) + "}";

    int httpResponseCode = http.POST(json);

    if (httpResponseCode > 0) {
      String response = http.getString();
      Serial.println("Resposta do servidor: " + response);
    } else {
      Serial.print("Erro HTTP: ");
      Serial.println(httpResponseCode);
    }

    http.end();
  } else {
    Serial.println("Wi-Fi desconectado.");
  }

  digitalWrite(LED_ACTIVITY_PIN, LOW);  // LED indica "esperando"
  delay(15000);  // espera antes da próxima coleta
}
