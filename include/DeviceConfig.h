#pragma once

#include <Arduino.h>
#include "Secrets.h"

namespace DeviceConfig {

constexpr uint8_t ds18b20DataPin = 4;
constexpr uint8_t numeroEscova = 1;
constexpr unsigned long sensorReadIntervalMs = 5000;
constexpr unsigned long telemetryIntervalMs = 10000;
constexpr unsigned long sensorRetryIntervalMs = 5000;
constexpr unsigned long wifiRetryIntervalMs = 5000;

// Use o IPv4 do computador na rede local; localhost aponta para o próprio ESP32.
constexpr char apiUrl[] = "http://192.168.15.178:5281/api/leituras";
constexpr uint16_t httpTimeoutMs = 5000;

}  // namespace DeviceConfig
