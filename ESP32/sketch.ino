#include <Wire.h>
#include <WiFi.h>
#include "ThingSpeak.h"

// === Sensores e atuadores ===
const int SOIL_MOISTURE_PIN = 34;
const int SPRINKLER_CONTROL_PIN = 19;
const int MPU_ADDR = 0x68;  // Endereço I2C padrão do MPU6050

// === Thresholds ===
int MOISTURE_THRESHOLD_LOW = 15;
int MOISTURE_THRESHOLD_HIGH = 85;
bool SPRINKLER_ACTIVATION_STATUS = false;

// === Wi-Fi & ThingSpeak ===
char* WIFI_NAME = "Wokwi-GUEST";
char* WIFI_PASSWORD = "";
int myChannelNumber = 2564339;
char* myApiKey = "3PDPDDF1RA32WIER";
WiFiClient client;

// === Variáveis MPU6050 ===
int16_t ax, ay, az;

void setup() {
  Serial.begin(115200);
  Wire.begin();  // Inicia I2C para MPU6050
  WiFi.begin(WIFI_NAME, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(100);
  }

  Serial.println("Wi-Fi connected. IP: " + String(WiFi.localIP()));
  ThingSpeak.begin(client);

  pinMode(SPRINKLER_CONTROL_PIN, OUTPUT);

  // Inicializa MPU6050
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x6B);  // registrador PWR_MGMT_1
  Wire.write(0);     // coloca o MPU6050 em modo ativo
  Wire.endTransmission(true);
}

void loop() {
  // === Leitura do sensor de umidade ===
  int soilRaw = analogRead(SOIL_MOISTURE_PIN);
  int soilMoisturePercentage = map(soilRaw, 0, 4095, 0, 100);

  // === Leitura do acelerômetro (inclinação eixo X) ===
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x3B);  // endereço do registrador ACCEL_XOUT_H
  Wire.endTransmission(false);
  Wire.requestFrom(MPU_ADDR, 6, true);  // 6 bytes: ax, ay, az

  ax = Wire.read() << 8 | Wire.read();
  ay = Wire.read() << 8 | Wire.read();
  az = Wire.read() << 8 | Wire.read();

  // Simples interpretação do eixo X para inclinação
  float ax_g = ax / 16384.0;  // Conversão para g
  float inclinacao = ax_g * 90;  // Aproximação linear da inclinação

  // === Lógica de ativação do aspersor ===
  if (soilMoisturePercentage < MOISTURE_THRESHOLD_LOW) {
    SPRINKLER_ACTIVATION_STATUS = true;
    digitalWrite(SPRINKLER_CONTROL_PIN, HIGH);
  } else if (soilMoisturePercentage > MOISTURE_THRESHOLD_HIGH) {
    SPRINKLER_ACTIVATION_STATUS = false;
    digitalWrite(SPRINKLER_CONTROL_PIN, LOW);
  }

  // === Envia dados ao Serial Monitor ===
  Serial.println("-------------");
  Serial.print("Umidade do solo: ");
  Serial.print(soilMoisturePercentage);
  Serial.println("%");

  Serial.print("Inclinação (X): ");
  Serial.print(inclinacao);
  Serial.println("°");

  Serial.print("Aspersor: ");
  Serial.println(SPRINKLER_ACTIVATION_STATUS ? "ON" : "OFF");

  // === Envia dados ao ThingSpeak ===
  ThingSpeak.setField(1, soilMoisturePercentage);
  ThingSpeak.setField(2, inclinacao);
  ThingSpeak.setField(3, SPRINKLER_ACTIVATION_STATUS ? 1 : 0);
  ThingSpeak.writeFields(myChannelNumber, myApiKey);

  delay(15000);  // Intervalo mínimo do ThingSpeak
}
