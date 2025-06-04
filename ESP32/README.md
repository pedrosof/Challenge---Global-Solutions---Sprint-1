# README - Coletor IoT ESP32 ALERT-AI

Este sketch Arduino em C++ foi desenvolvido para o microcontrolador **ESP32**, como parte da solução **ALERT-AI**. O código implementa a coleta de dados ambientais e o envio periódico para um backend em Flask responsável por classificar o risco de deslizamentos de terra.

---

## Funcionalidades

* Coleta **umidade do solo** via sensor analógico
* Mede **chuva** (sensor tipo chuva resistivo)
* Lê **inclinação do terreno** via acelerômetro MPU6050
* Indica coleta ativa com um **LED azul**
* Envia os dados para o backend Flask via **HTTP POST** em JSON

---

##  Componentes Utilizados

| Componente      | Porta ESP32  | Descrição                     |
| --------------- | ------------ | ----------------------------- |
| Sensor Umidade  | GPIO34       | Analógico                     |
| Sensor de Chuva | GPIO35       | Analógico (leitura invertida) |
| MPU6050         | I2C (21, 22) | Sensor de movimento (I2C)     |
| LED Indicador   | GPIO19       | LED azul de atividade         |

---

## Requisitos

* ESP32 Dev Board
* Sensor de umidade do solo (resistivo)
* Sensor de chuva
* MPU6050 (conectado via I2C)
* LED azul (com resistor)
* Backend Flask rodando na rede local

---

## Configurações Necessárias

No código:

```cpp
char* WIFI_NAME = "Wokwi-GUEST";  // sua rede Wi-Fi
char* WIFI_PASSWORD = "";         // senha do Wi-Fi
String backendURL = "http://192.168.1.100:5000/prever"; // IP do backend
```

---

## Fluxo de Coleta e Envio

1. LED azul acende indicando coleta
2. Dados de **umidade**, **chuva** e **inclinação** são lidos
3. Um JSON é montado com os dados:

```json
{
  "umidade": 52,
  "inclinacao": 13.4,
  "chuva": 87
}
```

4. JSON é enviado para o backend via HTTP POST
5. Resposta do backend é exibida no `Serial Monitor`
6. LED é desligado e o ESP aguarda 15 segundos

---

## Observações de Segurança

* A coleta não interpreta risco diretamente. O cálculo é realizado pela IA no servidor.
* O LED serve apenas para indicar o status de coleta.

---

## Exemplo de Uso

No `Serial Monitor`:

```
Wi-Fi conectado. IP: 192.168.1.42
-------------
Umidade do solo: 47%
Chuva: 72%
Inclinação (X): 14.35°
Resposta do servidor: {"risco": "medio"}
```

---

## Frequência de Coleta

* 1 leitura a cada 15 segundos
* Ajuste o intervalo alterando a linha:

```cpp
delay(15000);
```

---

## Backend Esperado

* Servidor Flask ouvindo em `/prever`
* Retorno em JSON com o risco classificado: `baixo`, `medio`, `alto`

---

## Autor

Projeto desenvolvido por **Fábio Pedroso** para a **Global Solution FIAP - Fase 7**

