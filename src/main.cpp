#include <Arduino.h>
#include "config.h"
#include "wifi_manager.h"
#include "temperature_manager.h"
#include "hardware_controller.h"
#include "ota_manager.h"
#include "telegram_bot.h"
#include "web_server.h"

TemperatureManager temperatureManager;
WiFiManager wifiManager;
HardwareController hardwareController;
OtaManager otaManager;
TelegramService telegramService(hardwareController, temperatureManager, otaManager, wifiManager);
WebServerService webServerService;

void setup() {
  Serial.begin(115200);
  delay(1500);

  hardwareController.begin();
  temperatureManager.begin();
  wifiManager.begin();
  telegramService.begin();
  webServerService.begin(&temperatureManager);
}

void loop() {
  temperatureManager.update();

  bool alarm = temperatureManager.getCurrent() > TEMPERATURE_ALERT_THRESHOLD;
  hardwareController.update(alarm);

  if (alarm) {
    telegramService.sendAlert(temperatureManager.getCurrent());
  } else {
    telegramService.resetAlertState();
  }

  wifiManager.loop();
  telegramService.loop();
  webServerService.loop();
  yield();
}
