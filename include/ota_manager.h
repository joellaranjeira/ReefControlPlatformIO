#pragma once

#include <Arduino.h>

class OtaManager {
public:
  OtaManager();
  void begin();
  String getRemoteVersion();
  bool isRemoteVersionGreater(const String& local, const String& remote) const;
  String performUpdate(const String& version);

private:
  void fillVersionParts(const String& version, int parts[3]) const;
};
