#include "wifi_manager.h"
#include "config.h"
#include <WiFi.h>

const unsigned long WiFiManager::reconnectIntervalMs_ = WIFI_RECONNECT_INTERVAL_MS;

void WiFiManager::begin() {
  Serial.print("Conectando em WiFi: ");
  Serial.println(SECRET_SSID);
  WiFi.mode(WIFI_STA);
  WiFi.begin(SECRET_SSID, SECRET_OPTIONAL_PASS);
  lastReconnectMs_ = millis();
}

void WiFiManager::loop() {
  if (WiFi.status() == WL_CONNECTED) {
    return;
  }

  if (millis() - lastReconnectMs_ < reconnectIntervalMs_) {
    return;
  }

  Serial.println("Reconectando WiFi...");
  WiFi.disconnect();
  WiFi.begin(SECRET_SSID, SECRET_OPTIONAL_PASS);
  lastReconnectMs_ = millis();
}

bool WiFiManager::isConnected() const {
  return WiFi.status() == WL_CONNECTED;
}

String WiFiManager::localIp() const {
  return WiFi.localIP().toString();
}
