#pragma once

class TemperatureManager;

class WebServerService {
public:
  WebServerService();
  void begin(TemperatureManager* temperatureManager);
  void loop();
};
