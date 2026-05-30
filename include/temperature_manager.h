#pragma once

#include "config.h"
#include <Arduino.h>

enum class TemperatureAlertType {
  None,
  Low,
  High
};

class TemperatureManager {
public:
  TemperatureManager();
  void begin();
  void update();
  float getCurrent() const;
  float getMaxTemp(unsigned long periodSeconds) const;
  float getMinTemp(unsigned long periodSeconds) const;
  int getRecentHistoryCount(unsigned long periodSeconds) const;
  float getHistoryTempAtOffset(int offsetFromOldest, unsigned long periodSeconds) const;
  bool isValid() const;
  TemperatureAlertType getAlertType() const;

  float getMaxAlertThreshold() const;
  float getMinAlertThreshold() const;
  bool setMaxAlertThreshold(float value);
  bool setMinAlertThreshold(float value);

private:
  float currentTemperature_;
  float historyTemp_[MAX_HISTORY];
  unsigned long historyTime_[MAX_HISTORY];
  int historyIndex_;
  int historyCount_;
  unsigned long previousUpdateMs_;
  float maxAlertTemperature_;
  float minAlertTemperature_;
  void updateHistory(float temp);
};
