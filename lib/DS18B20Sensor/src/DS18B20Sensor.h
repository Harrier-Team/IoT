#pragma once

#include <Arduino.h>
#include <DallasTemperature.h>
#include <OneWire.h>

class DS18B20Sensor {
public:
  explicit DS18B20Sensor(uint8_t oneWirePin);

  bool begin();
  bool isReady() const;
  bool readTemperatureC(float& temperatureC);

private:
  OneWire oneWire_;
  DallasTemperature sensors_;
  DeviceAddress deviceAddress_{};
  bool initialized_ = false;
  bool ready_ = false;
};
