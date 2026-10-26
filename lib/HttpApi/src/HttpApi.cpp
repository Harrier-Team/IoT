#include "HttpApi.h"

#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <math.h>
#include <WiFi.h>

HttpApi::HttpApi(const char* baseUrl, uint16_t timeoutMs)
    : baseUrl_(baseUrl), timeoutMs_(timeoutMs) {}

HttpApiResponse HttpApi::postTemperature(const String& espMacAddress,
                                         const SensorData* readings,
                                         size_t count) const {
  HttpApiResponse response;

  if (WiFi.status() != WL_CONNECTED) {
    response.errorMessage = "Wi-Fi desconectado";
    return response;
  }

  if (baseUrl_ == nullptr || baseUrl_[0] == '\0' ||
      espMacAddress.isEmpty() || readings == nullptr || count == 0) {
    response.errorMessage = "Dados da leitura inválidos";
    return response;
  }

  for (size_t i = 0; i < count; ++i) {
    if (readings[i].NumeroEscova == 0 ||
        !isfinite(readings[i].TemperaturaCelsius) ||
        readings[i].TemperaturaCelsius < -55.0f ||
        readings[i].TemperaturaCelsius > 125.0f) {
      response.errorMessage = "Número de escova ou temperatura fora do intervalo válido";
      return response;
    }
  }

  JsonDocument document;
  document["espMacAddress"] = espMacAddress;
  JsonArray sensores = document["sensores"].to<JsonArray>();
  for (size_t i = 0; i < count; ++i) {
    const SensorData& data = readings[i];
    JsonObject leitura = sensores.add<JsonObject>();
    leitura["numeroEscova"] = data.NumeroEscova;
    leitura["temperatura"] = data.TemperaturaCelsius;
  }

  String payload;
  if (serializeJson(document, payload) == 0) {
    response.errorMessage = "Não foi possível serializar o JSON";
    return response;
  }

  HTTPClient http;
  http.setConnectTimeout(timeoutMs_);
  http.setTimeout(timeoutMs_);
  if (!http.begin(baseUrl_)) {
    response.errorMessage = "Não foi possível iniciar HTTPClient para a URL configurada";
    return response;
  }

  http.addHeader("Content-Type", "application/json");
  const unsigned long requestStartedAt = millis();
  response.statusCode = http.POST(payload);
  response.durationMs = millis() - requestStartedAt;
  if (response.statusCode > 0) {
    response.body = http.getString();
  } else {
    response.errorMessage = http.errorToString(response.statusCode);
  }
  http.end();

  return response;
}
