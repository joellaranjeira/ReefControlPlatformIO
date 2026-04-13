#include "thingProperties.h"
#include <OneWire.h>
#include <DallasTemperature.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>
#include <WiFi.h>

#define FW_VERSION "1.0"

// ================= TELEGRAM =================
#define BOT_TOKEN SECRET_BOT_TOKEN
#define CHAT_ID SECRET_CHAT_ID

WiFiClientSecure client;
UniversalTelegramBot bot(BOT_TOKEN, client);

// ================= ESTADOS =================
bool alertaEnviado = false;
bool mensagemInicialEnviada = false;

bool buzzerManual = false;
bool buzzerLigadoManual = false;

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
const long intervalo = 2000;

unsigned long tempoTelegram = 0;
const long intervaloTelegram = 1000;

float ultimaTemperatura = 0;

// ================= FUNÇÕES =================

// 🔔 Controle do buzzer
void controlarBuzzer(bool alarme) {
  if (buzzerManual) {
    ledcWrite(canal, buzzerLigadoManual ? 128 : 0);
  } else {
    if (alarme) {
      ledcWrite(canal, (millis()/500)%2 ? 128 : 0);
    } else {
      ledcWrite(canal, 0);
    }
  }
}

// 💡 Controle dos LEDs
void controlarLED(bool alarme) {
  digitalWrite(VermelhoLed, alarme ? HIGH : LOW);
  digitalWrite(VerdeLed, alarme ? LOW : HIGH);
}

// 📲 Enviar alerta Telegram
void enviarAlerta(float temp) {
  if (!alertaEnviado) {
    String msg = "🚨 ALERTA AQUÁRIO!\nTemperatura: " + String(temp) + " °C";
    bot.sendMessage(CHAT_ID, msg, "");
    alertaEnviado = true;
  }
}

// 📲 Mensagem inicial
void enviarMensagemInicial() {
  if (ArduinoCloud.connected() && !mensagemInicialEnviada) {
    String msg = "✅ ReefPlusBot ON\nIP: ";
    msg += WiFi.localIP().toString();
    bot.sendMessage(CHAT_ID, msg, "");
    mensagemInicialEnviada = true;
  }
}

// 📥 Processar comandos Telegram
void processarComando(String texto, String chat_id) {

  if (texto == "/temp") {
    bot.sendMessage(chat_id, "🌡️ Temperatura: " + String(ultimaTemperatura) + " °C", "");
  }

  else if (texto == "/status") {
    String status = (ultimaTemperatura > 28) ? "🔴 ALERTA" : "🟢 NORMAL";
    bot.sendMessage(chat_id, "Status: " + status, "");
  }

  else if (texto == "/led") {
    String status = (ultimaTemperatura > 28) ? "🔴 Vermelho" : "🟢 Verde";
    bot.sendMessage(chat_id, "LED ativo: " + status, "");
  }

  else if (texto == "/buzzer on") {
    buzzerManual = true;
    buzzerLigadoManual = true;
    bot.sendMessage(chat_id, "🔔 Buzzer ligado manualmente", "");
  }

  else if (texto == "/buzzer off") {
    buzzerManual = true;
    buzzerLigadoManual = false;
    bot.sendMessage(chat_id, "🔕 Buzzer desligado manualmente", "");
  }

  else if (texto == "/buzzer auto") {
    buzzerManual = false;
    bot.sendMessage(chat_id, "🤖 Buzzer em modo automático", "");
  }

  else if (texto == "/help" || texto == "/start") {
    String help = "🤖 ReefPlusBot\n\n";
    help += "/temp\n/status\n/led\n";
    help += "/buzzer on\n/buzzer off\n/buzzer auto\n/help";
    bot.sendMessage(chat_id, help, "");
  }
}

// ================= SETUP =================
void setup() {
  Serial.begin(9600);
  delay(1500);

  pinMode(VerdeLed, OUTPUT);
  pinMode(VermelhoLed, OUTPUT);

  ledcSetup(canal, 2000, 8);
  ledcAttachPin(buzzer, canal);

  sensors.begin();

  initProperties();
  ArduinoCloud.begin(ArduinoIoTPreferredConnection);

  client.setInsecure();

  setDebugMessageLevel(2);
  ArduinoCloud.printDebugInfo();
}

// ================= LOOP =================
void loop() {
  ArduinoCloud.update();

  enviarMensagemInicial();

  unsigned long agora = millis();

  // 🌡️ LEITURA SENSOR
  if (agora - tempoAnterior >= intervalo) {
    tempoAnterior = agora;

    sensors.requestTemperatures();
    float temp = sensors.getTempCByIndex(0);

    if (temp == DEVICE_DISCONNECTED_C) {
      Serial.println("Erro no sensor! Verifique conexão.");
      // Não retorna, continua o loop para não travar
    } else {
      ultimaTemperatura = temp;
      temAq = round(temp * 10) / 10.0;

      Serial.print("Temperatura: ");
      Serial.println(temp);

      bool alarme = temp > 28;

      controlarLED(alarme);
      controlarBuzzer(alarme);

      if (alarme) {
        enviarAlerta(temp);
      } else {
        alertaEnviado = false;
      }
    }
  }

  // 📲 TELEGRAM
  if (millis() - tempoTelegram > intervaloTelegram) {
    tempoTelegram = millis();

    if (WiFi.status() == WL_CONNECTED) {
      int numMensagens = bot.getUpdates(bot.last_message_received + 1);

      for (int i = 0; i < numMensagens; i++) {
        processarComando(bot.messages[i].text, bot.messages[i].chat_id);
      }
    } else {
      Serial.println("WiFi desconectado, pulando Telegram.");
    }
  }

  // Alimenta o watchdog do ESP32
  yield();
}

// ================= IOT CLOUD =================
void onDateChange() {}
/*
  Since BuzzerCloud is READ_WRITE variable, onBuzzerCloudChange() is
  executed every time a new value is received from IoT Cloud.
*/
void onBuzzerCloudChange()  {
  // Add your code here to act upon BuzzerCloud change
  if (buzzerCloud) {
    buzzerManual = true;
    buzzerLigadoManual = true;
  } else {
    buzzerManual = false; // volta automático
  }
}
