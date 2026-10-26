#include "DS18B20Sensor.h"

#include <math.h>

namespace {
constexpr uint8_t DS18B20_FAMILY_CODE = 0x28;
constexpr float MIN_TEMPERATURE_C = -55.0f;
constexpr float MAX_TEMPERATURE_C = 125.0f;
}

DS18B20Sensor::DS18B20Sensor(uint8_t oneWirePin)
    : oneWire_(oneWirePin), sensors_(&oneWire_) {}

bool DS18B20Sensor::begin() {
  sensors_.begin();
  initialized_ = true;
  ready_ = false;

  const uint8_t deviceCount = sensors_.getDeviceCount();
  for (uint8_t index = 0; index < deviceCount; ++index) {
    DeviceAddress address;
    if (sensors_.getAddress(address, index) &&
        address[0] == DS18B20_FAMILY_CODE) {
      for (uint8_t byteIndex = 0; byteIndex < sizeof(deviceAddress_); ++byteIndex) {
        deviceAddress_[byteIndex] = address[byteIndex];
      }

      ready_ = true;
      return true;
    }
  }

  return false;
}

bool DS18B20Sensor::isReady() const {
  return initialized_ && ready_;
}

bool DS18B20Sensor::readTemperatureC(float& temperatureC) {
  if (!isReady()) {
    return false;
  }

  sensors_.requestTemperatures();
  const float reading = sensors_.getTempC(deviceAddress_);

  if (reading == DEVICE_DISCONNECTED_C ||
      !isfinite(reading) ||
      reading < MIN_TEMPERATURE_C ||
      reading > MAX_TEMPERATURE_C) {
    ready_ = false;
    return false;
  }

  temperatureC = reading;
  return true;
}
