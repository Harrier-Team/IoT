#include <Arduino.h>
#include <DS18B20Sensor.h>
#include <DeviceConfig.h>
#include <HttpApi.h>
#include <WiFiConnection.h>
#include <WiFi.h>

DS18B20Sensor temperatureSensor(DeviceConfig::ds18b20DataPin);
WiFiConnection wifiConnection(
    Secrets::wifiSsid,
    Secrets::wifiPassword,
    DeviceConfig::wifiRetryIntervalMs);
HttpApi telemetryApi(DeviceConfig::apiUrl, DeviceConfig::httpTimeoutMs);

unsigned long lastSensorBeginAttemptMs = 0;
unsigned long lastSensorReadMs = 0;
unsigned long lastTelemetryAttemptMs = 0;
unsigned long lastWifiStatusLogMs = 0;
float lastTemperatureCelsius = 0.0f;
bool hasTemperatureReading = false;
bool previousWifiConnected = false;

void setup() {
  Serial.begin(115200);
  delay(500);

  Serial.println();
  Serial.println("================================");
  Serial.println("Hackatohm ESP32 iniciando");
  Serial.println("Serial configurada para 115200 baud");
  Serial.printf("[Config] API: %s\n", DeviceConfig::apiUrl);
  Serial.printf("[Config] SSID configurado: %s\n",
                Secrets::wifiSsid[0] != '\0' ? "sim" : "NAO");
  Serial.println("================================");

  const unsigned long now = millis();
  lastSensorBeginAttemptMs = now;
  lastSensorReadMs = now;
  lastTelemetryAttemptMs = now;
  lastWifiStatusLogMs = now;

  if (!temperatureSensor.begin()) {
    Serial.printf("[Sensor] DS18B20 não encontrado no GPIO %u; nova busca em %lu ms.\n",
                  DeviceConfig::ds18b20DataPin,
                  DeviceConfig::sensorRetryIntervalMs);
  } else {
    Serial.println("[Sensor] DS18B20 pronto.");
  }

  Serial.printf("[ESP32] MAC: %s\n", WiFi.macAddress().c_str());
  wifiConnection.begin();
  Serial.printf("[Wi-Fi] Iniciando conexão; status inicial=%d.\n",
                static_cast<int>(WiFi.status()));
}

void loop() {
  wifiConnection.update();
  const bool wifiConnected = wifiConnection.isConnected();
  if (wifiConnected != previousWifiConnected) {
    previousWifiConnected = wifiConnected;
    if (wifiConnected) {
      Serial.print("[Wi-Fi] Conectado. IP do ESP32: ");
      Serial.println(WiFi.localIP());
      Serial.printf("[Wi-Fi] MAC: %s\n", WiFi.macAddress().c_str());
      if (hasTemperatureReading) {
        Serial.printf("[Sensor] Última leitura: %.2f C.\n", lastTemperatureCelsius);
      }
    } else {
      Serial.printf("[Wi-Fi] Conexão perdida; status=%d. Telemetria pausada.\n",
                    static_cast<int>(WiFi.status()));
    }
  }

  const unsigned long now = millis();
  if (!wifiConnected &&
      now - lastWifiStatusLogMs >= DeviceConfig::wifiRetryIntervalMs) {
    lastWifiStatusLogMs = now;
    Serial.printf("[Wi-Fi] Ainda conectando; status=%d. Próxima tentativa automática em até %lu ms.\n",
                  static_cast<int>(WiFi.status()),
                  DeviceConfig::wifiRetryIntervalMs);
  }

  if (!temperatureSensor.isReady()) {
    if (now - lastSensorBeginAttemptMs >= DeviceConfig::sensorRetryIntervalMs) {
      lastSensorBeginAttemptMs = now;
      if (temperatureSensor.begin()) {
        Serial.println("[Sensor] DS18B20 detectado e pronto.");
      } else {
        Serial.printf("[Sensor] DS18B20 ainda não encontrado no GPIO %u.\n",
                      DeviceConfig::ds18b20DataPin);
      }
    }
  } else if (now - lastSensorReadMs >= DeviceConfig::sensorReadIntervalMs) {
    lastSensorReadMs = now;
    if (temperatureSensor.readTemperatureC(lastTemperatureCelsius)) {
      hasTemperatureReading = true;
      if (wifiConnected) {
        Serial.printf("[Sensor] Temperatura: %.2f C.\n", lastTemperatureCelsius);
      }
    } else {
      hasTemperatureReading = false;
      lastSensorBeginAttemptMs = now;
      Serial.println("[Sensor] Falha na leitura; tentando localizar o DS18B20 novamente.");
    }
  }

  if (wifiConnected &&
      hasTemperatureReading &&
      now - lastTelemetryAttemptMs >= DeviceConfig::telemetryIntervalMs) {
    lastTelemetryAttemptMs = now;

    Serial.println();
    Serial.println("[Telemetria] Iniciando POST.");
    Serial.print("[Telemetria] Destino: POST ");
    Serial.println(DeviceConfig::apiUrl);
    Serial.print("[Telemetria] MAC do ESP32: ");
    Serial.println(WiFi.macAddress());
    Serial.printf("[Telemetria] Leituras no lote: 1; escova #%u; temperatura %.2f C\n",
                DeviceConfig::numeroEscova,
                lastTemperatureCelsius);

    SensorData sensorData = {DeviceConfig::numeroEscova, lastTemperatureCelsius};
    const HttpApiResponse response = telemetryApi.postTemperature(
        WiFi.macAddress(),
        &sensorData,
        1);

    if (response.isHttpSuccess()) {
      Serial.printf("[Telemetria] API aceitou a requisição: HTTP %d (%lu ms).\n",
                    response.statusCode,
                    response.durationMs);
      if (!response.body.isEmpty()) {
        Serial.print("[Telemetria] Resposta: ");
        Serial.println(response.body);
      }
    } else if (response.statusCode > 0) {
      Serial.printf("[Telemetria] API respondeu HTTP %d (%lu ms); dados não confirmados como salvos.\n",
                    response.statusCode,
                    response.durationMs);
      if (!response.body.isEmpty()) {
        Serial.print("[Telemetria] Detalhe da API: ");
        Serial.println(response.body);
      }
    } else {
      Serial.printf("[Telemetria] Falha de transporte HTTP (%d): %s\n",
                    response.statusCode,
                    response.errorMessage.c_str());
    }
  } else if (wifiConnected &&
             !hasTemperatureReading &&
             now - lastTelemetryAttemptMs >= DeviceConfig::telemetryIntervalMs) {
    lastTelemetryAttemptMs = now;
    Serial.println("[Telemetria] Envio aguardando leitura válida do DS18B20.");
  } else if (!wifiConnected &&
             now - lastTelemetryAttemptMs >= DeviceConfig::telemetryIntervalMs) {
    lastTelemetryAttemptMs = now;
    Serial.println("[Telemetria] Envio pausado: ESP32 ainda não está conectado ao Wi-Fi.");
  }
}