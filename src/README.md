# ReefControlPlatformIO

Projeto de monitoramento de aquário marinho com ESP32.

## 📦 Funcionalidades

* Leitura de temperatura (DS18B20)
* Alertas via Telegram
* Integração com IoT

## ⚙️ Configuração

1. Copie o arquivo:
   src/secrets.example.h → src/secrets.h

2. Preencha com suas credenciais:

   * WiFi
   * Telegram
   * Arduino Cloud (se usar)

3. Compile e envie para o ESP32

## 🛠️ Tecnologias utilizadas

* ESP32
* PlatformIO
* DallasTemperature
* UniversalTelegramBot

## 📌 Observações

Este projeto é voltado para automação de aquário marinho (Reef).
