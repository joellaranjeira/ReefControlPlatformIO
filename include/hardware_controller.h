#pragma once

#include <Arduino.h>

class HardwareController {
public:
  HardwareController();
  void begin();
  void update(bool alarm);
  void setManualBuzzer(bool enabled, bool on);
  bool isManualBuzzer() const;
  bool isManualBuzzerOn() const;

private:
  bool manualBuzzer_;
  bool manualBuzzerOn_;
};
