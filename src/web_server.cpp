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

    float dataMinTemp = minTemp;
    float dataMaxTemp = maxTemp;
    float padding = (maxTemp - minTemp) * 0.15;
    if (padding < 0.5) padding = 0.5;
    minTemp -= padding;
    maxTemp += padding;

    float range = maxTemp - minTemp;
    int width = 760;
    int height = 280;
    int marginLeft = 52;
    int marginRight = 12;
    int marginTop = 12;
    int marginBottom = 28;
    int plotWidth = width - marginLeft - marginRight;
    int plotHeight = height - marginTop - marginBottom;
    float xStep = float(plotWidth) / (points - 1);
    String path = "";

    for (int i = 0; i < points; i++) {
      float temp = temperatureManagerPtr->getHistoryTempAtOffset(i, 86400);
      float x = marginLeft + i * xStep;
      float y = marginTop + plotHeight - ((temp - minTemp) / range) * plotHeight;
      if (i == 0) {
        path += "M" + String(x, 1) + "," + String(y, 1);
      } else {
        path += " L" + String(x, 1) + "," + String(y, 1);
      }
    }

    html += "<svg width='100%' viewBox='0 0 " + String(width) + " " + String(height) + "' style='border:1px solid #ccc;border-radius:8px;background:#f8fbff;'>";
    for (int i = 0; i <= 4; i++) {
      float y = marginTop + (plotHeight * i / 4.0);
      float labelTemp = maxTemp - (range * i / 4.0);
      html += "<line x1='" + String(marginLeft) + "' y1='" + String(y, 1) + "' x2='" + String(width - marginRight) + "' y2='" + String(y, 1) + "' stroke='#d8e2ea' stroke-width='1'/>";
      html += "<text x='" + String(marginLeft - 6) + "' y='" + String(y + 4, 1) + "' text-anchor='end' font-size='12' fill='#456'>" + String(labelTemp, 1) + "&deg;C</text>";
    }
    html += "<path d='" + path + "' fill='none' stroke='#2196f3' stroke-width='2.5' stroke-linejoin='round' stroke-linecap='round'/>";
    int markerStep = points > 24 ? (points + 23) / 24 : 1;
    for (int i = 0; i < points; i += markerStep) {
      float temp = temperatureManagerPtr->getHistoryTempAtOffset(i, 86400);
      float x = marginLeft + i * xStep;
      float y = marginTop + plotHeight - ((temp - minTemp) / range) * plotHeight;
      html += "<circle cx='" + String(x, 1) + "' cy='" + String(y, 1) + "' r='3' fill='#fff' stroke='#1976d2' stroke-width='2'><title>" + String(temp, 1) + " &deg;C</title></circle>";
    }
    float lastTemp = temperatureManagerPtr->getHistoryTempAtOffset(points - 1, 86400);
    float lastX = marginLeft + (points - 1) * xStep;
    float lastY = marginTop + plotHeight - ((lastTemp - minTemp) / range) * plotHeight;
    html += "<circle cx='" + String(lastX, 1) + "' cy='" + String(lastY, 1) + "' r='4' fill='#1976d2'><title>Atual: " + String(lastTemp, 1) + " &deg;C</title></circle>";
    html += "<text x='" + String(marginLeft) + "' y='" + String(height - 8) + "' font-size='12' fill='#456'>In&iacute;cio</text>";
    html += "<text x='" + String(width - marginRight) + "' y='" + String(height - 8) + "' text-anchor='end' font-size='12' fill='#456'>Agora</text>";
    html += "</svg>";
    html += "<div class='chart-legend'><span class='chart-label'>Mín: " + String(dataMinTemp, 1) + " °C</span><span class='chart-label'>Máx: " + String(dataMaxTemp, 1) + " °C</span><span class='chart-label'>Atual: " + String(lastTemp, 1) + " °C</span><span class='chart-label'>Leituras: " + String(points) + "</span></div>";
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
