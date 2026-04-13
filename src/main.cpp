#include "secrets.h"
#include <Arduino.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <WiFi.h>
#include <WebServer.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <HTTPUpdate.h>
#include <UniversalTelegramBot.h>

#define FW_VERSION "1.1"
#define MAX_HISTORY 720

// ================= TELEGRAM =================
#define BOT_TOKEN SECRET_BOT_TOKEN
#define CHAT_ID SECRET_CHAT_ID

WiFiClientSecure client;
UniversalTelegramBot bot(BOT_TOKEN, client);
WebServer server(80);

// ================= ESTADOS =================
bool alertaEnviado = false;
bool mensagemInicialEnviada = false;
bool solicitacaoAtualizacaoEnviada = false;
String versaoDisponivel = "";

bool buzzerManual = false;
bool buzzerLigadoManual = false;

long ultimoUpdateIdProcessado = -1;
bool reiniciando = false;

// ================= PINOS =================
const int VermelhoLed = 18;
const int VerdeLed = 26;
const int buzzer = 32;
const int canal = 0;

#define ONE_WIRE_BUS 4

// ================= SENSOR =================
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

// ================= CONTROLE TEMPO =================
unsigned long tempoAnterior = 0;
const long intervalo = 120000; // Atualiza a cada 2 minutos

unsigned long tempoTelegram = 0;
const long intervaloTelegram = 1000;

unsigned long tempoOTA = 0;
const long intervaloOTA = 3600000; // Verifica atualizacao a cada 1 hora

unsigned long tempoWiFiReconnect = 0;
const long intervaloWiFiReconnect = 30000;

unsigned long tempoBoot = 0;
const unsigned long intervaloIgnorarRestartAposBoot = 10000;
bool telegramPendentesLimpos = false;

const char* GITHUB_VERSION_URL = "https://raw.githubusercontent.com/joellaranjeira/ReefControlPlatformIO/main/version.txt";
const char* GITHUB_FIRMWARE_URL = "https://raw.githubusercontent.com/joellaranjeira/ReefControlPlatformIO/main/firmware.bin";

float ultimaTemperatura = 0;
float historicoTemp[MAX_HISTORY] = {0};
unsigned long historicoTime[MAX_HISTORY] = {0};
int historicoIndex = 0;
int historicoCount = 0;

float getMaxTemp(unsigned long periodSeconds) {
  unsigned long now = millis() / 1000;
  float maxTemp = -999;
  for (int i = 0; i < historicoCount; i++) {
    int index = (historicoIndex - 1 - i + MAX_HISTORY) % MAX_HISTORY;
    if (now - historicoTime[index] <= periodSeconds) {
      if (historicoTemp[index] > maxTemp) {
        maxTemp = historicoTemp[index];
      }
    }
  }
  return maxTemp;
}

float getMinTemp(unsigned long periodSeconds) {
  unsigned long now = millis() / 1000;
  float minTemp = 999;
  for (int i = 0; i < historicoCount; i++) {
    int index = (historicoIndex - 1 - i + MAX_HISTORY) % MAX_HISTORY;
    if (now - historicoTime[index] <= periodSeconds) {
      if (historicoTemp[index] < minTemp) {
        minTemp = historicoTemp[index];
      }
    }
  }
  return minTemp;
}

// ================= FUNCOES =================

void setupWiFi() {
  Serial.print("Conectando em WiFi: ");
  Serial.println(SECRET_SSID);
  WiFi.mode(WIFI_STA);
  WiFi.begin(SECRET_SSID, SECRET_OPTIONAL_PASS);
  tempoWiFiReconnect = millis();
}

void atualizarHistorico(float temp) {
  historicoTemp[historicoIndex] = temp;
  historicoTime[historicoIndex] = millis() / 1000;
  historicoIndex = (historicoIndex + 1) % MAX_HISTORY;
  if (historicoCount < MAX_HISTORY) {
    historicoCount++;
  }
}

void lerTemperatura() {
  unsigned long agora = millis();
  if (agora - tempoAnterior < intervalo) {
    return;
  }

  tempoAnterior = agora;
  sensors.requestTemperatures();
  float temp = sensors.getTempCByIndex(0);

  if (temp == DEVICE_DISCONNECTED_C) {
    Serial.println("Erro no sensor! Verifique conexao.");
    return;
  }

  ultimaTemperatura = temp;
  atualizarHistorico(temp);
  Serial.print("Temperatura: ");
  Serial.println(temp);
}

void controlarBuzzer(bool alarme) {
  if (buzzerManual) {
    ledcWrite(canal, buzzerLigadoManual ? 128 : 0);
  } else {
    if (alarme) {
      ledcWrite(canal, (millis() / 500) % 2 ? 128 : 0);
    } else {
      ledcWrite(canal, 0);
    }
  }
}

void controlarLED(bool alarme) {
  digitalWrite(VermelhoLed, alarme ? HIGH : LOW);
  digitalWrite(VerdeLed, alarme ? LOW : HIGH);
}

void enviarAlerta(float temp) {
  if (!alertaEnviado) {
    String msg = "🚨 ALERTA AQUÁRIO!\nTemperatura: " + String(temp) + " °C";
    bot.sendMessage(CHAT_ID, msg, "");
    alertaEnviado = true;
  }
}

void enviarMensagemInicial() {
  if (WiFi.status() == WL_CONNECTED && !mensagemInicialEnviada) {
    String ip = WiFi.localIP().toString();
    String msg = "✅ ReefPlusBot ON\nIP: " + ip + "\n[Dashboard](http://" + ip + ")";
    bot.sendMessage(CHAT_ID, msg, "Markdown");
    mensagemInicialEnviada = true;
  }
}

String gerarDashboardHtml() {
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
  html += "<p style='font-size:2rem;margin:8px 0;'>" + String(ultimaTemperatura, 1) + " &deg;C</p>";
  html += "<p>Atualiza automaticamente a cada 2 minutos.</p></div>";

  // Gráfico do dia
  html += "<div class='card chart-card'><h2 class='chart-title'>Gráfico de Variação do Último Dia</h2>";
  unsigned long now = millis() / 1000;
  int points = 0;
  for (int i = 0; i < historicoCount; i++) {
    int index = (historicoIndex - 1 - i + MAX_HISTORY) % MAX_HISTORY;
    if (now - historicoTime[index] <= 86400) {
      points++;
    } else {
      break;
    }
  }

  if (points < 2) {
    html += "<p>Dados insuficientes para exibir o gráfico do dia.</p>";
  } else {
    float minTemp = 999;
    float maxTemp = -999;
    for (int i = points - 1; i >= 0; i--) {
      int index = (historicoIndex - 1 - i + MAX_HISTORY) % MAX_HISTORY;
      float temp = historicoTemp[index];
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
      int index = (historicoIndex - points + i + MAX_HISTORY) % MAX_HISTORY;
      float temp = historicoTemp[index];
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
  float max1d = getMaxTemp(86400);
  float min1d = getMinTemp(86400);
  html += "<div class='stat'><span class='stat-label'>Máxima:</span><span class='stat-value'>" + (max1d > -900 ? String(max1d, 1) + " &deg;C" : "N/A") + "</span></div>";
  html += "<div class='stat'><span class='stat-label'>Mínima:</span><span class='stat-value'>" + (min1d < 900 ? String(min1d, 1) + " &deg;C" : "N/A") + "</span></div>";

  html += "<h3>Últimos 7 Dias</h3>";
  float max7d = getMaxTemp(604800);
  float min7d = getMinTemp(604800);
  html += "<div class='stat'><span class='stat-label'>Máxima:</span><span class='stat-value'>" + (max7d > -900 ? String(max7d, 1) + " &deg;C" : "N/A") + "</span></div>";
  html += "<div class='stat'><span class='stat-label'>Mínima:</span><span class='stat-value'>" + (min7d < 900 ? String(min7d, 1) + " &deg;C" : "N/A") + "</span></div>";

  html += "<h3>Últimos 15 Dias</h3>";
  float max15d = getMaxTemp(1296000);
  float min15d = getMinTemp(1296000);
  html += "<div class='stat'><span class='stat-label'>Máxima:</span><span class='stat-value'>" + (max15d > -900 ? String(max15d, 1) + " &deg;C" : "N/A") + "</span></div>";
  html += "<div class='stat'><span class='stat-label'>Mínima:</span><span class='stat-value'>" + (min15d < 900 ? String(min15d, 1) + " &deg;C" : "N/A") + "</span></div>";

  html += "<h3>Últimos 30 Dias</h3>";
  float max30d = getMaxTemp(2592000);
  float min30d = getMinTemp(2592000);
  html += "<div class='stat'><span class='stat-label'>Máxima:</span><span class='stat-value'>" + (max30d > -900 ? String(max30d, 1) + " &deg;C" : "N/A") + "</span></div>";
  html += "<div class='stat'><span class='stat-label'>Mínima:</span><span class='stat-value'>" + (min30d < 900 ? String(min30d, 1) + " &deg;C" : "N/A") + "</span></div>";
  html += "</div>";

  html += "</body></html>";
  return html;
}

void handleDashboard() {
  server.handleClient();
}

void handleRoot() {
  server.send(200, "text/html", gerarDashboardHtml());
}

void handleNotFound() {
  server.send(404, "text/plain", "Página não encontrada");
}

void iniciarServidorWeb() {
  server.on("/", handleRoot);
  server.onNotFound(handleNotFound);
  server.begin();
  Serial.println("Servidor web iniciado na porta 80");
}

String getVersaoRemota() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi não conectado para checar versão remota.");
    return "ERROR: WiFi desconectado";
  }

  HTTPClient http;
  http.begin(client, GITHUB_VERSION_URL);
  int httpCode = http.GET();
  String versao = "";

  if (httpCode == HTTP_CODE_OK) {
    versao = http.getString();
    versao.trim();
    Serial.println("Versão remota encontrada: " + versao);
  } else {
    Serial.printf("Erro HTTP ao buscar versão remota: %d\n", httpCode);
    versao = "ERROR: HTTP " + String(httpCode);
  }

  http.end();
  return versao;
}

void preencherPartesVersao(const String& versao, int partes[3]) {
  String valor = versao;
  valor.trim();
  int parte = 0;
  String token = "";

  for (unsigned int i = 0; i <= valor.length() && parte < 3; i++) {
    if (i == valor.length() || valor[i] == '.') {
      partes[parte++] = token.toInt();
      token = "";
    } else if (isDigit(valor[i])) {
      token += valor[i];
    }
  }
}

bool versaoRemotaMaior(const String& local, const String& remota) {
  int localPartes[3] = {0, 0, 0};
  int remotaPartes[3] = {0, 0, 0};

  preencherPartesVersao(local, localPartes);
  preencherPartesVersao(remota, remotaPartes);

  for (int i = 0; i < 3; i++) {
    if (remotaPartes[i] > localPartes[i]) {
      return true;
    }
    if (remotaPartes[i] < localPartes[i]) {
      return false;
    }
  }

  return false;
}

void executarAtualizacao(const String& versao, const String& chat_id) {
  if (WiFi.status() != WL_CONNECTED) {
    bot.sendMessage(chat_id, "⚠️ WiFi não conectado. Não é possível atualizar agora.", "");
    return;
  }

  bot.sendMessage(chat_id, "🔄 Iniciando atualização OTA para a versão " + versao + "... Aguarde.", "");
  t_httpUpdate_return ret = httpUpdate.update(client, GITHUB_FIRMWARE_URL);

  switch (ret) {
    case HTTP_UPDATE_FAILED:
      bot.sendMessage(chat_id, "❌ Falha na atualização OTA: " + String(httpUpdate.getLastError()) + " - " + String(httpUpdate.getLastErrorString().c_str()) + ". Tente novamente mais tarde.", "");
      Serial.printf("OTA falhou: %d - %s\n", httpUpdate.getLastError(), httpUpdate.getLastErrorString().c_str());
      break;
    case HTTP_UPDATE_NO_UPDATES:
      bot.sendMessage(chat_id, "✅ Nenhuma atualização encontrada no servidor.", "");
      Serial.println("Nenhuma atualização disponível.");
      break;
    case HTTP_UPDATE_OK:
      bot.sendMessage(chat_id, "✅ Atualização concluída com sucesso! Reiniciando o dispositivo...", "");
      Serial.println("OTA concluído com sucesso. Reiniciando...");
      break;
  }
}

void verificarAtualizacao() {
  String versaoRemota = getVersaoRemota();
  if (versaoRemota.length() == 0 || versaoRemota.startsWith("ERROR:")) {
    Serial.println("Erro ao verificar versão remota: " + versaoRemota);
    return;
  }

  if (!versaoRemotaMaior(FW_VERSION, versaoRemota)) {
    Serial.println("Firmware já está atualizado.");
    return;
  }

  if (!solicitacaoAtualizacaoEnviada || versaoDisponivel != versaoRemota) {
    versaoDisponivel = versaoRemota;
    solicitacaoAtualizacaoEnviada = true;

    String mensagem = "🔄 Nova versão disponível: " + versaoRemota + "\nVersão atual: " + String(FW_VERSION) + "\nDeseja atualizar agora?";
    String teclado = "[[{ \"text\" : \"Sim, atualizar\", \"callback_data\" : \"update_yes\" }], [{ \"text\" : \"Não, depois\", \"callback_data\" : \"update_no\" }]]";

    bot.sendMessage(CHAT_ID, mensagem, "");
    bot.sendMessageWithInlineKeyboard(CHAT_ID, "Escolha uma opção:", "", teclado);
    Serial.println("Solicitação de atualização enviada via Telegram.");
  }
}

void processarMensagem(int index) {
  String texto = bot.messages[index].text;
  String chat_id = bot.messages[index].chat_id;
  long update_id = bot.messages[index].update_id;
  int message_id = bot.messages[index].message_id;

  Serial.printf("Telegram update_id=%ld type=%s text=%s\n", update_id, bot.messages[index].type.c_str(), texto.c_str());

  // Evita reprocessar o mesmo update do Telegram
  if (update_id <= ultimoUpdateIdProcessado) {
    Serial.println("Update já processado, pulando.");
    return;
  }
  ultimoUpdateIdProcessado = update_id;

  if (bot.messages[index].type == "callback_query") {
    String callbackQueryId = bot.messages[index].query_id;
    String fromId = bot.messages[index].from_id;
    String callbackData = texto;

    if (callbackData == "update_yes") {
      bot.answerCallbackQuery(callbackQueryId, "Atualização iniciada...", false, "", 0);
      executarAtualizacao(versaoDisponivel, fromId);
    } else if (callbackData == "update_no") {
      bot.answerCallbackQuery(callbackQueryId, "Atualização cancelada.", false, "", 0);
      bot.sendMessage(fromId, "🛑 Atualização adiada. Você pode solicitar novamente com /update.", "");
      solicitacaoAtualizacaoEnviada = false;
    } else {
      bot.answerCallbackQuery(callbackQueryId, "Opção desconhecida.", false, "", 0);
    }
    return;
  }

  if (texto == "/temp") {
    bot.sendMessage(chat_id, "🌡️ Temperatura: " + String(ultimaTemperatura) + " °C", "");
  } else if (texto == "/status") {
    String status = (ultimaTemperatura > 28) ? "🔴 ALERTA" : "🟢 NORMAL";
    bot.sendMessage(chat_id, "Status: " + status, "");
  } else if (texto == "/led") {
    String status = (ultimaTemperatura > 28) ? "🔴 Vermelho" : "🟢 Verde";
    bot.sendMessage(chat_id, "LED ativo: " + status, "");
  } else if (texto == "/buzzer on") {
    buzzerManual = true;
    buzzerLigadoManual = true;
    bot.sendMessage(chat_id, "🔔 Buzzer ligado manualmente", "");
  } else if (texto == "/buzzer off") {
    buzzerManual = true;
    buzzerLigadoManual = false;
    bot.sendMessage(chat_id, "🔕 Buzzer desligado manualmente", "");
  } else if (texto == "/buzzer auto") {
    buzzerManual = false;
    bot.sendMessage(chat_id, "🤖 Buzzer em modo automático", "");
  } else if (texto == "/update") {
    String versaoRemota = getVersaoRemota();
    if (versaoRemota.length() == 0 || versaoRemota.startsWith("ERROR:")) {
      String erroMsg = "⚠️ Não foi possível verificar a versão remota.";
      if (versaoRemota.startsWith("ERROR:")) {
        erroMsg += " (" + versaoRemota.substring(7) + ")";
      }
      bot.sendMessage(chat_id, erroMsg, "");
      return;
    }
    if (versaoRemotaMaior(FW_VERSION, versaoRemota)) {
      versaoDisponivel = versaoRemota;
      solicitacaoAtualizacaoEnviada = true;
      String mensagem = "🔄 Nova versão disponível: " + versaoRemota + "\nVersão atual: " + String(FW_VERSION) + "\nDeseja atualizar agora?";
      String teclado = "[[{ \"text\" : \"Sim, atualizar\", \"callback_data\" : \"update_yes\" }], [{ \"text\" : \"Não, depois\", \"callback_data\" : \"update_no\" }]]";
      bot.sendMessage(chat_id, mensagem, "");
      bot.sendMessageWithInlineKeyboard(chat_id, "Escolha uma opção:", "", teclado);
    } else {
      bot.sendMessage(chat_id, "✅ Firmware já está atualizado. Versão: " + String(FW_VERSION), "");
    }
  } else if (texto == "/cancel") {
    if (solicitacaoAtualizacaoEnviada) {
      solicitacaoAtualizacaoEnviada = false;
      versaoDisponivel = "";
      bot.sendMessage(chat_id, "🛑 Solicitação de atualização cancelada.", "");
    } else {
      bot.sendMessage(chat_id, "ℹ️ Não há solicitação de atualização pendente para cancelar.", "");
    }
  } else if (texto == "/version") {
    String versaoRemota = getVersaoRemota();
    String msg = "📋 Informações de Versão\n\n";
    msg += "Versão atual: " + String(FW_VERSION) + "\n";
    if (versaoRemota.length() > 0 && !versaoRemota.startsWith("ERROR:")) {
      msg += "Versão remota: " + versaoRemota + "\n";
      if (versaoRemotaMaior(FW_VERSION, versaoRemota)) {
        msg += "🔄 Atualização disponível!";
      } else {
        msg += "✅ Firmware atualizado.";
      }
    } else {
      String erro = versaoRemota.startsWith("ERROR:") ? versaoRemota.substring(7) : "WiFi desconectado";
      msg += "Versão remota: Não disponível (" + erro + ")";
    }
    bot.sendMessage(chat_id, msg, "");
  } else if (texto == "/restart" && !reiniciando) {
    if (millis() - tempoBoot < intervaloIgnorarRestartAposBoot) {
      bot.sendMessage(chat_id, "Ignorando comando /restart logo após o reboot. Tente novamente em alguns segundos.", "");
      Serial.println("Ignorado /restart logo após boot para evitar loop de reinício.");
      return;
    }
    reiniciando = true;
    bot.sendMessage(chat_id, "Reiniciando em 3 segundos...", "");
    bot.sendMessage(chat_id, "Restart efetuado com sucesso. Temperatura atual: " + String(ultimaTemperatura) + " °C", "");
    delay(3000);
    ESP.restart();
  } else if (texto == "/help" || texto == "/start") {
    String help = "🤖 ReefPlusBot - Controle do Aquário\n\n";
    help += "📊 Monitoramento:\n";
    help += "/temp - Temperatura atual\n";
    help += "/status - Status geral (Normal/Alerta)\n";
    help += "/led - LED ativo (Verde/Vermelho)\n\n";
    help += "🔊 Buzzer:\n";
    help += "/buzzer on - Ligar buzzer manualmente\n";
    help += "/buzzer off - Desligar buzzer manualmente\n";
    help += "/buzzer auto - Modo automático\n\n";
    help += "🔄 Atualização:\n";
    help += "/update - Verificar e atualizar firmware\n";
    help += "/cancel - Cancelar atualização pendente\n";
    help += "/version - Ver versões atual e remota\n\n";
    help += "⚙️ Sistema:\n";
    help += "/restart - Reiniciar ESP32\n\n";
    help += "/help - Mostrar esta ajuda";
    bot.sendMessage(chat_id, help, "");
  }
}

void setup() {
  Serial.begin(9600);
  delay(1500);
  tempoBoot = millis();

  pinMode(VerdeLed, OUTPUT);
  pinMode(VermelhoLed, OUTPUT);

  ledcSetup(canal, 2000, 8);
  ledcAttachPin(buzzer, canal);

  sensors.begin();

  setupWiFi();
  iniciarServidorWeb();

  client.setInsecure();
}

void loop() {
  lerTemperatura();
  enviarMensagemInicial();

  bool alarme = (ultimaTemperatura > 28.0);
  controlarLED(alarme);
  controlarBuzzer(alarme);

  if (!alarme) {
    alertaEnviado = false;
  }

  unsigned long agora = millis();

  if (agora - tempoTelegram >= intervaloTelegram) {
    tempoTelegram = agora;

    if (WiFi.status() == WL_CONNECTED) {
      if (!telegramPendentesLimpos) {
        int numPendentes = bot.getUpdates(0);
        telegramPendentesLimpos = true;
        Serial.printf("Telegram: limpei %d updates pendentes no boot\n", numPendentes);
      }

      int numMensagens = bot.getUpdates(bot.last_message_received + 1);
      for (int i = 0; i < numMensagens; i++) {
        processarMensagem(i);
      }
    } else if (agora - tempoWiFiReconnect >= intervaloWiFiReconnect) {
      Serial.println("Reconectando WiFi...");
      WiFi.disconnect();
      WiFi.begin(SECRET_SSID, SECRET_OPTIONAL_PASS);
      tempoWiFiReconnect = agora;
    }
  }

  if (agora - tempoOTA > intervaloOTA) {
    tempoOTA = agora;
    verificarAtualizacao();
  }

  if (alarme) {
    enviarAlerta(ultimaTemperatura);
  }

  handleDashboard();
  yield();
}
