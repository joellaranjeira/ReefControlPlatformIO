#pragma once

#include "secrets.h"
#include <Arduino.h>

#define FW_VERSION "1.2.0"
#define MAX_HISTORY 720

#define BOT_TOKEN SECRET_BOT_TOKEN
#define CHAT_ID SECRET_CHAT_ID

const int PIN_LED_RED = 18;
const int PIN_LED_GREEN = 26;
const int PIN_BUZZER = 32;
const int BUZZER_CHANNEL = 0;

#define ONE_WIRE_BUS 4

const unsigned long TEMPERATURE_INTERVAL_MS = 120000;
const unsigned long TELEGRAM_INTERVAL_MS = 1000;
const unsigned long OTA_CHECK_INTERVAL_MS = 3600000;
const unsigned long WIFI_RECONNECT_INTERVAL_MS = 30000;
const unsigned long BOOT_SAFE_RESTART_MS = 10000;
const float TEMPERATURE_ALERT_THRESHOLD = 28.0;

const char* GITHUB_VERSION_URL = "https://raw.githubusercontent.com/joellaranjeira/ReefControlPlatformIO/main/version.txt";
const char* GITHUB_FIRMWARE_URL = "https://raw.githubusercontent.com/joellaranjeira/ReefControlPlatformIO/main/firmware.bin";
