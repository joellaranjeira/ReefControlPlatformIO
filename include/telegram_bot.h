#pragma once

#include "config.h"
#include <Arduino.h>
#include <UniversalTelegramBot.h>
#include <WiFiClientSecure.h>

class HardwareController;
class TemperatureManager;
class OtaManager;
class WiFiManager;

class TelegramService {
public:
  TelegramService(HardwareController& hardwareController,
                  TemperatureManager& temperatureManager,
                  OtaManager& otaManager,
                  WiFiManager& wifiManager);

  void begin();
  void loop();
  void sendAlert(float temp);
  void resetAlertState();

private:
  HardwareController& hardwareController_;
  TemperatureManager& temperatureManager_;
  OtaManager& otaManager_;
  WiFiManager& wifiManager_;
  WiFiClientSecure client_;
  UniversalTelegramBot bot_;
  bool alertaEnviado_;
  bool mensagemInicialEnviada_;
  bool solicitacaoAtualizacaoEnviada_;
  bool telegramPendentesLimpos_;
  bool reiniciando_;
  long ultimoUpdateIdProcessado_;
  unsigned long tempoTelegram_;
  unsigned long tempoOTA_;
  unsigned long tempoBoot_;
  String versaoDisponivel_;

  void processMessage(int index);
  void processCommand(const String& texto, const String& chat_id);
  void handleCallbackQuery(const String& callbackData,
                           const String& callbackQueryId,
                           const String& fromId);
  void checkForUpdates();
  void sendStartupMessage();
};
