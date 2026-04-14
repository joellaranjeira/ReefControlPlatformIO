#include "telegram_bot.h"
#include "config.h"
#include "hardware_controller.h"
#include "temperature_manager.h"
#include "ota_manager.h"
#include "wifi_manager.h"
#include <WiFi.h>

TelegramService::TelegramService(HardwareController& hardwareController,
                                 TemperatureManager& temperatureManager,
                                 OtaManager& otaManager,
                                 WiFiManager& wifiManager)
    : hardwareController_(hardwareController),
      temperatureManager_(temperatureManager),
      otaManager_(otaManager),
      wifiManager_(wifiManager),
      bot_(BOT_TOKEN, client_),
      alertaEnviado_(false),
      mensagemInicialEnviada_(false),
      solicitacaoAtualizacaoEnviada_(false),
      telegramPendentesLimpos_(false),
      reiniciando_(false),
      ultimoUpdateIdProcessado_(-1),
      tempoTelegram_(0),
      tempoOTA_(0),
      tempoBoot_(0) {}

void TelegramService::begin() {
  client_.setInsecure();
  tempoBoot_ = millis();
}

void TelegramService::loop() {
  if (!wifiManager_.isConnected()) {
    return;
  }

  unsigned long agora = millis();

  if (!telegramPendentesLimpos_) {
    int numPendentes = bot_.getUpdates(0);
    telegramPendentesLimpos_ = true;
    Serial.printf("Telegram: limpei %d updates pendentes no boot\n", numPendentes);
  }

  if (agora - tempoTelegram_ >= TELEGRAM_INTERVAL_MS) {
    tempoTelegram_ = agora;
    int numMensagens = bot_.getUpdates(bot_.last_message_received + 1);
    for (int i = 0; i < numMensagens; i++) {
      processMessage(i);
    }
  }

  if (agora - tempoOTA_ >= OTA_CHECK_INTERVAL_MS) {
    tempoOTA_ = agora;
    checkForUpdates();
  }

  sendStartupMessage();
}

void TelegramService::sendStartupMessage() {
  if (!mensagemInicialEnviada_ && wifiManager_.isConnected()) {
    String ip = wifiManager_.localIp();
    String msg = "✅ ReefPlusBot ON\nIP: " + ip + "\n[Dashboard](http://" + ip + ")";
    bot_.sendMessage(CHAT_ID, msg, "Markdown");
    mensagemInicialEnviada_ = true;
  }
}

void TelegramService::sendAlert(float temp) {
  if (!alertaEnviado_ && wifiManager_.isConnected()) {
    String msg = "🚨 ALERTA AQUÁRIO!\nTemperatura: " + String(temp) + " °C";
    bot_.sendMessage(CHAT_ID, msg, "");
    alertaEnviado_ = true;
  }
}

void TelegramService::resetAlertState() {
  alertaEnviado_ = false;
}

void TelegramService::processMessage(int index) {
  String texto = bot_.messages[index].text;
  String chat_id = bot_.messages[index].chat_id;
  long update_id = bot_.messages[index].update_id;

  Serial.printf("Telegram update_id=%ld type=%s text=%s\n", update_id, bot_.messages[index].type.c_str(), texto.c_str());

  if (update_id <= ultimoUpdateIdProcessado_) {
    Serial.println("Update já processado, pulando.");
    return;
  }

  ultimoUpdateIdProcessado_ = update_id;

  if (bot_.messages[index].type == "callback_query") {
    handleCallbackQuery(texto, bot_.messages[index].query_id, bot_.messages[index].from_id);
    return;
  }

  processCommand(texto, chat_id);
}

void TelegramService::handleCallbackQuery(const String& callbackData,
                                          const String& callbackQueryId,
                                          const String& fromId) {
  if (callbackData == "update_yes") {
    bot_.answerCallbackQuery(callbackQueryId, "Atualização iniciada...", false, "", 0);
    bot_.sendMessage(fromId, "🔄 Iniciando atualização OTA para a versão " + versaoDisponivel_ + "...", "");
    String error = otaManager_.performUpdate(versaoDisponivel_);
    if (error.length() > 0) {
      bot_.sendMessage(fromId, "❌ " + error, "");
    }
  } else if (callbackData == "update_no") {
    bot_.answerCallbackQuery(callbackQueryId, "Atualização cancelada.", false, "", 0);
    bot_.sendMessage(fromId, "🛑 Atualização adiada. Você pode solicitar novamente com /update.", "");
    solicitacaoAtualizacaoEnviada_ = false;
    versaoDisponivel_.clear();
  } else {
    bot_.answerCallbackQuery(callbackQueryId, "Opção desconhecida.", false, "", 0);
  }
}

void TelegramService::processCommand(const String& texto, const String& chat_id) {
  if (texto == "/temp") {
    bot_.sendMessage(chat_id, "🌡️ Temperatura: " + String(temperatureManager_.getCurrent()) + " °C", "");
  } else if (texto == "/status") {
    String status = (temperatureManager_.getCurrent() > TEMPERATURE_ALERT_THRESHOLD) ? "🔴 ALERTA" : "🟢 NORMAL";
    bot_.sendMessage(chat_id, "Status: " + status, "");
  } else if (texto == "/led") {
    String status = (temperatureManager_.getCurrent() > TEMPERATURE_ALERT_THRESHOLD) ? "🔴 Vermelho" : "🟢 Verde";
    bot_.sendMessage(chat_id, "LED ativo: " + status, "");
  } else if (texto == "/buzzer on") {
    hardwareController_.setManualBuzzer(true, true);
    bot_.sendMessage(chat_id, "🔔 Buzzer ligado manualmente", "");
  } else if (texto == "/buzzer off") {
    hardwareController_.setManualBuzzer(true, false);
    bot_.sendMessage(chat_id, "🔕 Buzzer desligado manualmente", "");
  } else if (texto == "/buzzer auto") {
    hardwareController_.setManualBuzzer(false, false);
    bot_.sendMessage(chat_id, "🤖 Buzzer em modo automático", "");
  } else if (texto == "/update") {
    String versaoRemota = otaManager_.getRemoteVersion();
    if (versaoRemota.length() == 0 || versaoRemota.startsWith("ERROR:")) {
      String erroMsg = "⚠️ Não foi possível verificar a versão remota.";
      if (versaoRemota.startsWith("ERROR:")) {
        erroMsg += " (" + versaoRemota.substring(7) + ")";
      }
      bot_.sendMessage(chat_id, erroMsg, "");
      return;
    }

    if (otaManager_.isRemoteVersionGreater(FW_VERSION, versaoRemota)) {
      versaoDisponivel_ = versaoRemota;
      solicitacaoAtualizacaoEnviada_ = true;
      String mensagem = "🔄 Nova versão disponível: " + versaoRemota + "\nVersão atual: " + String(FW_VERSION) + "\nDeseja atualizar agora?";
      String teclado = "[[{ \"text\" : \"Sim, atualizar\", \"callback_data\" : \"update_yes\" }], [{ \"text\" : \"Não, depois\", \"callback_data\" : \"update_no\" }]]";
      bot_.sendMessage(chat_id, mensagem, "");
      bot_.sendMessageWithInlineKeyboard(chat_id, "Escolha uma opção:", "", teclado);
    } else {
      bot_.sendMessage(chat_id, "✅ Firmware já está atualizado. Versão: " + String(FW_VERSION), "");
    }
  } else if (texto == "/cancel") {
    if (solicitacaoAtualizacaoEnviada_) {
      solicitacaoAtualizacaoEnviada_ = false;
      versaoDisponivel_.clear();
      bot_.sendMessage(chat_id, "🛑 Solicitação de atualização cancelada.", "");
    } else {
      bot_.sendMessage(chat_id, "ℹ️ Não há solicitação de atualização pendente para cancelar.", "");
    }
  } else if (texto == "/version") {
    String versaoRemota = otaManager_.getRemoteVersion();
    String msg = "📋 Informações de Versão\n\n";
    msg += "Versão atual: " + String(FW_VERSION) + "\n";
    if (versaoRemota.length() > 0 && !versaoRemota.startsWith("ERROR:")) {
      msg += "Versão remota: " + versaoRemota + "\n";
      if (otaManager_.isRemoteVersionGreater(FW_VERSION, versaoRemota)) {
        msg += "🔄 Atualização disponível!";
      } else {
        msg += "✅ Firmware atualizado.";
      }
    } else {
      String erro = versaoRemota.startsWith("ERROR:") ? versaoRemota.substring(7) : "WiFi desconectado";
      msg += "Versão remota: Não disponível (" + erro + ")";
    }
    bot_.sendMessage(chat_id, msg, "");
  } else if (texto == "/restart" && !reiniciando_) {
    if (millis() - tempoBoot_ < BOOT_SAFE_RESTART_MS) {
      bot_.sendMessage(chat_id, "Ignorando comando /restart logo após o reboot. Tente novamente em alguns segundos.", "");
      Serial.println("Ignorado /restart logo após boot para evitar loop de reinício.");
      return;
    }
    reiniciando_ = true;
    bot_.sendMessage(chat_id, "Reiniciando em 3 segundos...", "");
    bot_.sendMessage(chat_id, "Restart efetuado com sucesso. Temperatura atual: " + String(temperatureManager_.getCurrent()) + " °C", "");
    delay(3000);
    ESP.restart();
  } else if (texto == "/help" || texto == "/start") {
    String help1 = "🤖 *ReefPlusBot - Controle do Aquário*\n\n";
    help1 += "📊 *Monitoramento:*\n";
    help1 += "/temp - Temperatura atual\n";
    help1 += "/status - Status geral (Normal/Alerta)\n";
    help1 += "/led - LED ativo (Verde/Vermelho)\n";
    bot_.sendMessage(chat_id, help1, "Markdown");

    String help2 = "🔊 *Buzzer:*\n";
    help2 += "/buzzer on - Ligar buzzer manualmente\n";
    help2 += "/buzzer off - Desligar buzzer manualmente\n";
    help2 += "/buzzer auto - Modo automático\n";
    bot_.sendMessage(chat_id, help2, "Markdown");

    String help3 = "🔄 *Atualização:*\n";
    help3 += "/update - Verificar e atualizar firmware\n";
    help3 += "/cancel - Cancelar atualização pendente\n";
    help3 += "/version - Ver versões atual e remota\n";
    bot_.sendMessage(chat_id, help3, "Markdown");

    String help4 = "⚙️ *Sistema:*\n";
    help4 += "/restart - Reiniciar ESP32\n";
    help4 += "/help - Mostrar esta ajuda\n";
    bot_.sendMessage(chat_id, help4, "Markdown");
  }
}

void TelegramService::checkForUpdates() {
  String versaoRemota = otaManager_.getRemoteVersion();
  if (versaoRemota.length() == 0 || versaoRemota.startsWith("ERROR:")) {
    Serial.println("Erro ao verificar versão remota: " + versaoRemota);
    return;
  }

  if (!otaManager_.isRemoteVersionGreater(FW_VERSION, versaoRemota)) {
    Serial.println("Firmware já está atualizado.");
    return;
  }

  if (!solicitacaoAtualizacaoEnviada_ || versaoDisponivel_ != versaoRemota) {
    versaoDisponivel_ = versaoRemota;
    solicitacaoAtualizacaoEnviada_ = true;

    String mensagem = "🔄 Nova versão disponível: " + versaoRemota + "\nVersão atual: " + String(FW_VERSION) + "\nDeseja atualizar agora?";
    String teclado = "[[{ \"text\" : \"Sim, atualizar\", \"callback_data\" : \"update_yes\" }], [{ \"text\" : \"Não, depois\", \"callback_data\" : \"update_no\" }]]";

    bot_.sendMessage(CHAT_ID, mensagem, "");
    bot_.sendMessageWithInlineKeyboard(CHAT_ID, "Escolha uma opção:", "", teclado);
    Serial.println("Solicitação de atualização enviada via Telegram.");
  }
}
