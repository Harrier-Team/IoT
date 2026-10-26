#include "WiFiConnection.h"

#include <WiFi.h>

WiFiConnection::WiFiConnection(const char* ssid, const char* password,
                               unsigned long retryIntervalMs)
    : ssid_(ssid),
      password_(password),
      retryIntervalMs_(retryIntervalMs) {}

void WiFiConnection::begin() {
  if (started_) {
    return;
  }

  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);
  WiFi.begin(ssid_, password_);
  lastAttemptMs_ = millis();
  started_ = true;
}

void WiFiConnection::update() {
  if (!started_) {
    begin();
    return;
  }

  if (isConnected()) {
    return;
  }

  const unsigned long now = millis();
  if (now - lastAttemptMs_ < retryIntervalMs_) {
    return;
  }

  lastAttemptMs_ = now;
  WiFi.reconnect();
}

bool WiFiConnection::isConnected() const {
  return started_ && WiFi.status() == WL_CONNECTED;
}
