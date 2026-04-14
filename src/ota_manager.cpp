#include "ota_manager.h"
#include "config.h"
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <Update.h>

OtaManager::OtaManager() {}

void OtaManager::begin() {
  // Placeholder for future OTA initialization
}

String OtaManager::getRemoteVersion() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi não conectado para checar versão remota.");
    return "ERROR: WiFi desconectado";
  }

  WiFiClientSecure client;
  client.setInsecure();
  HTTPClient http;
  http.begin(client, GITHUB_VERSION_URL);

  int httpCode = http.GET();
  String version = "";

  if (httpCode == HTTP_CODE_OK) {
    version = http.getString();
    version.trim();
    Serial.println("Versão remota encontrada: " + version);
  } else {
    Serial.printf("Erro HTTP ao buscar versão remota: %d\n", httpCode);
    version = "ERROR: HTTP " + String(httpCode);
  }

  http.end();
  return version;
}

bool OtaManager::isRemoteVersionGreater(const String& local, const String& remote) const {
  int localParts[3] = {0, 0, 0};
  int remoteParts[3] = {0, 0, 0};

  fillVersionParts(local, localParts);
  fillVersionParts(remote, remoteParts);

  for (int i = 0; i < 3; i++) {
    if (remoteParts[i] > localParts[i]) {
      return true;
    }
    if (remoteParts[i] < localParts[i]) {
      return false;
    }
  }

  return false;
}

String OtaManager::performUpdate(const String& version) {
  if (WiFi.status() != WL_CONNECTED) {
    return "WiFi não conectado. Não é possível atualizar agora.";
  }

  WiFiClientSecure client;
  client.setInsecure();
  HTTPClient http;
  http.begin(client, GITHUB_FIRMWARE_URL);

  int httpCode = http.GET();
  Serial.printf("HTTP Code: %d\n", httpCode);

  if (httpCode != HTTP_CODE_OK) {
    String error = "Falha ao baixar firmware. HTTP Code: " + String(httpCode);
    http.end();
    return error;
  }

  int contentLength = http.getSize();
  Serial.printf("Tamanho do firmware: %d bytes\n", contentLength);

  if (contentLength <= 0) {
    http.end();
    return "Tamanho inválido do firmware.";
  }

  WiFiClient* stream = http.getStreamPtr();
  if (!Update.begin(contentLength)) {
    http.end();
    return "Não há espaço suficiente para OTA.";
  }

  size_t written = Update.writeStream(*stream);
  Serial.printf("Bytes escritos: %d\n", written);

  if (written != static_cast<size_t>(contentLength)) {
    http.end();
    return "Falha na escrita do firmware.";
  }

  if (!Update.end()) {
    String error = "Erro ao finalizar OTA: " + String(Update.getError());
    Serial.println(Update.getError());
    http.end();
    return error;
  }

  if (!Update.isFinished()) {
    http.end();
    return "OTA não finalizada corretamente.";
  }

  http.end();
  delay(2000);
  ESP.restart();
  return "";
}

void OtaManager::fillVersionParts(const String& version, int parts[3]) const {
  String value = version;
  value.trim();
  int part = 0;
  String token = "";

  for (unsigned int i = 0; i <= value.length() && part < 3; i++) {
    if (i == value.length() || value[i] == '.') {
      parts[part++] = token.toInt();
      token = "";
    } else if (isDigit(value[i])) {
      token += value[i];
    }
  }
}
