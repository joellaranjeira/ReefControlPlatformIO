#include "temperature_manager.h"
#include <OneWire.h>
#include <DallasTemperature.h>

static OneWire oneWire(ONE_WIRE_BUS);
static DallasTemperature sensors(&oneWire);

TemperatureManager::TemperatureManager()
    : currentTemperature_(0.0), historyIndex_(0), historyCount_(0), previousUpdateMs_(0) {
  for (int i = 0; i < MAX_HISTORY; i++) {
    historyTemp_[i] = 0.0;
    historyTime_[i] = 0;
  }
}

void TemperatureManager::begin() {
  sensors.begin();
}

void TemperatureManager::update() {
  unsigned long now = millis();
  if (now - previousUpdateMs_ < TEMPERATURE_INTERVAL_MS) {
    return;
  }

  previousUpdateMs_ = now;
  sensors.requestTemperatures();
  float temp = sensors.getTempCByIndex(0);

  if (temp == DEVICE_DISCONNECTED_C) {
    Serial.println("Erro no sensor! Verifique conexao.");
    return;
  }

  currentTemperature_ = temp;
  updateHistory(temp);
  Serial.print("Temperatura: ");
  Serial.println(temp);
}

void TemperatureManager::updateHistory(float temp) {
  historyTemp_[historyIndex_] = temp;
  historyTime_[historyIndex_] = millis() / 1000;
  historyIndex_ = (historyIndex_ + 1) % MAX_HISTORY;
  if (historyCount_ < MAX_HISTORY) {
    historyCount_++;
  }
}

float TemperatureManager::getCurrent() const {
  return currentTemperature_;
}

bool TemperatureManager::isValid() const {
  return historyCount_ > 0;
}

float TemperatureManager::getMaxTemp(unsigned long periodSeconds) const {
  unsigned long now = millis() / 1000;
  float maxTemp = -999;

  for (int i = 0; i < historyCount_; i++) {
    int index = (historyIndex_ - 1 - i + MAX_HISTORY) % MAX_HISTORY;
    if (now - historyTime_[index] <= periodSeconds) {
      if (historyTemp_[index] > maxTemp) {
        maxTemp = historyTemp_[index];
      }
    }
  }

  return maxTemp;
}

float TemperatureManager::getMinTemp(unsigned long periodSeconds) const {
  unsigned long now = millis() / 1000;
  float minTemp = 999;

  for (int i = 0; i < historyCount_; i++) {
    int index = (historyIndex_ - 1 - i + MAX_HISTORY) % MAX_HISTORY;
    if (now - historyTime_[index] <= periodSeconds) {
      if (historyTemp_[index] < minTemp) {
        minTemp = historyTemp_[index];
      }
    }
  }

  return minTemp;
}

int TemperatureManager::getRecentHistoryCount(unsigned long periodSeconds) const {
  unsigned long now = millis() / 1000;
  int count = 0;

  for (int i = 0; i < historyCount_; i++) {
    int index = (historyIndex_ - 1 - i + MAX_HISTORY) % MAX_HISTORY;
    if (now - historyTime_[index] <= periodSeconds) {
      count++;
    } else {
      break;
    }
  }

  return count;
}

float TemperatureManager::getHistoryTempAtOffset(int offsetFromOldest, unsigned long periodSeconds) const {
  int points = getRecentHistoryCount(periodSeconds);
  if (offsetFromOldest < 0 || offsetFromOldest >= points || points == 0) {
    return currentTemperature_;
  }

  int index = (historyIndex_ - points + offsetFromOldest + MAX_HISTORY) % MAX_HISTORY;
  return historyTemp_[index];
}
