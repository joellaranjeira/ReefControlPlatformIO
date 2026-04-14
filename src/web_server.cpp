#include "web_server.h"
#include "temperature_manager.h"
#include <WebServer.h>

static WebServer server(80);
static TemperatureManager* temperatureManagerPtr = nullptr;

static String generateDashboardHtml() {
  float currentTemperature = temperatureManagerPtr ? temperatureManagerPtr->getCurrent() : 0.0;
  String html = "<!DOCTYPE html><html><head><meta charset='UTF-8'><meta name='viewport' content='width=device-width, initial-scale=1'>";
  html += "<title>Reef Control Dashboard</title>";
  html += "<meta http-equiv='refresh' content='120'>";
  html += "<style>body{font-family:Arial,Helvetica,sans-serif;margin:0;padding:16px;background:#eef6fb;color:#0b2a49;}";
  html += "h1{margin-top:0;} .card{background:#fff;border-radius:8px;box-shadow:0 2px 8px rgba(0,0,0,0.1);padding:16px;margin-bottom:16px;}";
  html += ".history-item{padding:8px 0;border-bottom:1px solid #ddd;} .history-item:last-child{border-bottom:none;}";
  html += ".bar{display:inline-block;height:16px;background:#2196f3;border-radius:8px;vertical-align:middle;margin-left:8px;}";
  html += ".stat{display:flex;justify-content:space-between;margin-bottom:8px;} .stat-label{font-weight:bold;} .stat-value{color:#2196f3;}";
  html += ".chart-card{display:block;overflow-x:auto;} .chart-title{margin-bottom:8px;} .chart-legend{display:flex;justify-content:space-between;flex-wrap:wrap;} .chart-label{font-size:0.9rem;color:#555;}";
  html += "</style></head><body>";
  html += "<h1>Reef Control Dashboard</h1>";
  html += "<div class='card'><h2>Temperatura Atual</h2>";
  html += "<p style='font-size:2rem;margin:8px 0;'>" + String(currentTemperature, 1) + " &deg;C</p>";
  html += "<p>Atualiza automaticamente a cada 2 minutos.</p></div>";

  html += "<div class='card chart-card'><h2 class='chart-title'>Gráfico de Variação do Último Dia</h2>";
  int points = temperatureManagerPtr ? temperatureManagerPtr->getRecentHistoryCount(86400) : 0;

  if (points < 2) {
    html += "<p>Dados insuficientes para exibir o gráfico do dia.</p>";
  } else {
    float minTemp = 999;
    float maxTemp = -999;
    for (int i = 0; i < points; i++) {
      float temp = temperatureManagerPtr->getHistoryTempAtOffset(i, 86400);
      if (temp < minTemp) minTemp = temp;
      if (temp > maxTemp) maxTemp = temp;
    }

    if (minTemp == maxTemp) {
      minTemp -= 1.0;
      maxTemp += 1.0;
    }

    float range = maxTemp - minTemp;
    int width = 700;
    int height = 240;
    float xStep = float(width) / (points - 1);
    String path = "";

    for (int i = 0; i < points; i++) {
      float temp = temperatureManagerPtr->getHistoryTempAtOffset(i, 86400);
      float x = i * xStep;
      float y = height - ((temp - minTemp) / range) * (height - 20) - 10;
      if (i == 0) {
        path += "M" + String(x, 1) + "," + String(y, 1);
      } else {
        path += " L" + String(x, 1) + "," + String(y, 1);
      }
    }

    html += "<svg width='100%' viewBox='0 0 " + String(width) + " " + String(height) + "' style='border:1px solid #ccc;border-radius:8px;background:#f8fbff;'>";
    html += "<polyline fill='none' stroke='#2196f3' stroke-width='2' points='" + path + "' />";
    html += "<line x1='0' y1='10' x2='" + String(width) + "' y2='10' stroke='#ddd' stroke-width='1'/>";
    html += "<line x1='0' y1='" + String(height - 10) + "' x2='" + String(width) + "' y2='" + String(height - 10) + "' stroke='#ddd' stroke-width='1'/>";
    html += "</svg>";
    html += "<div class='chart-legend'><span class='chart-label'>Mín: " + String(minTemp, 1) + " °C</span><span class='chart-label'>Máx: " + String(maxTemp, 1) + " °C</span><span class='chart-label'>Pontos: " + String(points) + "</span></div>";
  }

  html += "</div>";
  html += "<div class='card'><h2>Estatísticas de Temperatura</h2>";
  html += "<h3>Último Dia (24h)</h3>";
  float max1d = temperatureManagerPtr ? temperatureManagerPtr->getMaxTemp(86400) : -999;
  float min1d = temperatureManagerPtr ? temperatureManagerPtr->getMinTemp(86400) : 999;
  html += "<div class='stat'><span class='stat-label'>Máxima:</span><span class='stat-value'>" + (max1d > -900 ? String(max1d, 1) + " &deg;C" : "N/A") + "</span></div>";
  html += "<div class='stat'><span class='stat-label'>Mínima:</span><span class='stat-value'>" + (min1d < 900 ? String(min1d, 1) + " &deg;C" : "N/A") + "</span></div>";

  html += "<h3>Últimos 7 Dias</h3>";
  float max7d = temperatureManagerPtr ? temperatureManagerPtr->getMaxTemp(604800) : -999;
  float min7d = temperatureManagerPtr ? temperatureManagerPtr->getMinTemp(604800) : 999;
  html += "<div class='stat'><span class='stat-label'>Máxima:</span><span class='stat-value'>" + (max7d > -900 ? String(max7d, 1) + " &deg;C" : "N/A") + "</span></div>";
  html += "<div class='stat'><span class='stat-label'>Mínima:</span><span class='stat-value'>" + (min7d < 900 ? String(min7d, 1) + " &deg;C" : "N/A") + "</span></div>";

  html += "<h3>Últimos 15 Dias</h3>";
  float max15d = temperatureManagerPtr ? temperatureManagerPtr->getMaxTemp(1296000) : -999;
  float min15d = temperatureManagerPtr ? temperatureManagerPtr->getMinTemp(1296000) : 999;
  html += "<div class='stat'><span class='stat-label'>Máxima:</span><span class='stat-value'>" + (max15d > -900 ? String(max15d, 1) + " &deg;C" : "N/A") + "</span></div>";
  html += "<div class='stat'><span class='stat-label'>Mínima:</span><span class='stat-value'>" + (min15d < 900 ? String(min15d, 1) + " &deg;C" : "N/A") + "</span></div>";

  html += "<h3>Últimos 30 Dias</h3>";
  float max30d = temperatureManagerPtr ? temperatureManagerPtr->getMaxTemp(2592000) : -999;
  float min30d = temperatureManagerPtr ? temperatureManagerPtr->getMinTemp(2592000) : 999;
  html += "<div class='stat'><span class='stat-label'>Máxima:</span><span class='stat-value'>" + (max30d > -900 ? String(max30d, 1) + " &deg;C" : "N/A") + "</span></div>";
  html += "<div class='stat'><span class='stat-label'>Mínima:</span><span class='stat-value'>" + (min30d < 900 ? String(min30d, 1) + " &deg;C" : "N/A") + "</span></div>";
  html += "</div>";

  html += "</body></html>";
  return html;
}

static void handleRoot() {
  server.send(200, "text/html", generateDashboardHtml());
}

static void handleNotFound() {
  server.send(404, "text/plain", "Página não encontrada");
}

WebServerService::WebServerService() {}

void WebServerService::begin(TemperatureManager* temperatureManager) {
  temperatureManagerPtr = temperatureManager;
  server.on("/", handleRoot);
  server.onNotFound(handleNotFound);
  server.begin();
  Serial.println("Servidor web iniciado na porta 80");
}

void WebServerService::loop() {
  server.handleClient();
}
