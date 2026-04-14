#pragma once

#include <Arduino.h>

class WiFiManager {
public:
  void begin();
  void loop();
  bool isConnected() const;
  String localIp() const;

private:
  unsigned long lastReconnectMs_ = 0;
  static const unsigned long reconnectIntervalMs_;
};
