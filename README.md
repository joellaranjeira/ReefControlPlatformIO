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

## 🧱 Estrutura de arquivos

* `src/main.cpp` — orquestra a inicialização e o loop principal.
* `include/config.h` — constantes de configuração, pinos e URLs.
* `include/wifi_manager.h` / `src/wifi_manager.cpp` — gerencia conexão e reconexão WiFi.
* `include/temperature_manager.h` / `src/temperature_manager.cpp` — lê DS18B20 e mantém histórico/min/max.
* `include/hardware_controller.h` / `src/hardware_controller.cpp` — controla LEDs e buzzer, incluindo modo manual.
* `include/ota_manager.h` / `src/ota_manager.cpp` — verifica versão remota e executa OTA via GitHub.
* `include/telegram_bot.h` / `src/telegram_bot.cpp` — processa comandos Telegram, alertas e callbacks.
* `include/web_server.h` / `src/web_server.cpp` — serve o dashboard HTML e gerencia rotas.
* `src/secrets.h` — credenciais privadas de WiFi e Telegram (não commite).

## 📌 Observações

Este projeto é voltado para automação de aquário marinho (Reef).
