#include "hardware_controller.h"
#include "config.h"

HardwareController::HardwareController()
    : manualBuzzer_(false), manualBuzzerOn_(false) {}

void HardwareController::begin() {
  pinMode(PIN_LED_GREEN, OUTPUT);
  pinMode(PIN_LED_RED, OUTPUT);
  ledcSetup(BUZZER_CHANNEL, 2000, 8);
  ledcAttachPin(PIN_BUZZER, BUZZER_CHANNEL);
}

void HardwareController::update(bool alarm) {
  digitalWrite(PIN_LED_RED, alarm ? HIGH : LOW);
  digitalWrite(PIN_LED_GREEN, alarm ? LOW : HIGH);

  if (manualBuzzer_) {
    ledcWrite(BUZZER_CHANNEL, manualBuzzerOn_ ? 128 : 0);
  } else {
    ledcWrite(BUZZER_CHANNEL, alarm ? ((millis() / 500) % 2 ? 128 : 0) : 0);
  }
}

void HardwareController::setManualBuzzer(bool enabled, bool on) {
  manualBuzzer_ = enabled;
  manualBuzzerOn_ = on;
}

bool HardwareController::isManualBuzzer() const {
  return manualBuzzer_;
}

bool HardwareController::isManualBuzzerOn() const {
  return manualBuzzerOn_;
}
