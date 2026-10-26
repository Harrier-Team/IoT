#pragma once

#include <Arduino.h>

class WiFiConnection {
public:
  WiFiConnection(const char* ssid, const char* password,
                 unsigned long retryIntervalMs);

  void begin();
  void update();
  bool isConnected() const;

private:
  const char* ssid_;
  const char* password_;
  unsigned long retryIntervalMs_;
  unsigned long lastAttemptMs_ = 0;
  bool started_ = false;
};
