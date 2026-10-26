#pragma once

#include <Arduino.h>
#include "SensorData.h"

struct HttpApiResponse {
  int statusCode = -1;
  unsigned long durationMs = 0;
  String body;
  String errorMessage;

  bool isHttpSuccess() const {
    return statusCode >= 200 && statusCode < 300;
  }
};

class HttpApi {
public:
  explicit HttpApi(const char* baseUrl, uint16_t timeoutMs = 5000);

  HttpApiResponse postTemperature(const String& espMacAddress,
                                  const SensorData* readings,
                                  size_t count) const;

private:
  const char* baseUrl_;
  uint16_t timeoutMs_;
};
