#include <DHT.h>
#include <UavPayload.h>

constexpr uint8_t DHT_DATA_PIN = 2;
constexpr uint8_t DHT_SENSOR_TYPE = DHT22;
constexpr uint32_t PAYLOAD_SCAN_INTERVAL_MS = 1000;
constexpr uint32_t DHT_SAMPLE_INTERVAL_MS = 2000;

uav::DescriptorEeprom descriptorEeprom;
DHT dht(DHT_DATA_PIN, DHT_SENSOR_TYPE);

struct PayloadDriver {
  uav::PayloadType payloadType;
  uav::CommInterface commInterface;
  bool (*begin)(const uav::PayloadDescriptor &descriptor);
  void (*update)();
  void (*end)();
};

bool beginDht22(const uav::PayloadDescriptor &descriptor);
void updateDht22();
void endDht22();

// Future UART, CAN, I2C, USB, and other drivers are registered here.
PayloadDriver drivers[] = {
    {uav::PayloadType::EnvironmentalSensor, uav::CommInterface::SingleWire,
     beginDht22, updateDht22, endDht22},
};

PayloadDriver *activeDriver = nullptr;
bool payloadConnected = false;
uint32_t lastScanAt = 0;
uint32_t lastDhtSampleAt = 0;

PayloadDriver *findDriver(const uav::PayloadDescriptor &descriptor) {
  for (PayloadDriver &driver : drivers) {
    if (driver.payloadType == descriptor.payloadType &&
        driver.commInterface == descriptor.commInterface) return &driver;
  }
  return nullptr;
}

void deactivatePayload() {
  if (activeDriver != nullptr && activeDriver->end != nullptr) activeDriver->end();
  activeDriver = nullptr;
  payloadConnected = false;
  Serial.println(F("Payload removed or EEPROM unavailable."));
}

void discoverPayload() {
  const bool present = descriptorEeprom.isPresent();
  if (!present) {
    if (payloadConnected) deactivatePayload();
    return;
  }
  if (payloadConnected) return;

  uav::PayloadDescriptor descriptor;
  const uav::DescriptorStatus status = descriptorEeprom.read(descriptor);
  if (status != uav::DescriptorStatus::Ok) {
    Serial.print(F("Payload descriptor rejected: "));
    Serial.println(uav::statusName(status));
    return;
  }

  payloadConnected = true;
  uav::printDescriptor(Serial, descriptor);
  activeDriver = findDriver(descriptor);
  if (activeDriver == nullptr) {
    Serial.println(F("No compatible driver is registered for this payload."));
    return;
  }
  if (!activeDriver->begin(descriptor)) {
    Serial.println(F("Driver initialization failed."));
    activeDriver = nullptr;
    return;
  }
  Serial.println(F("Payload driver started."));
}

bool beginDht22(const uav::PayloadDescriptor &descriptor) {
  if (strcmp(descriptor.name, "DHT22_ENV") != 0) {
    Serial.println(F("Single-wire environmental payload is not DHT22_ENV."));
    return false;
  }
  dht.begin();
  lastDhtSampleAt = millis() - DHT_SAMPLE_INTERVAL_MS;
  return true;
}

void updateDht22() {
  if (millis() - lastDhtSampleAt < DHT_SAMPLE_INTERVAL_MS) return;
  lastDhtSampleAt = millis();
  const float humidity = dht.readHumidity();
  const float temperatureC = dht.readTemperature();
  if (isnan(humidity) || isnan(temperatureC)) {
    Serial.println(F("DHT22 read failed. Check wiring and data pull-up."));
    return;
  }
  Serial.print(F("DHT22: "));
  Serial.print(temperatureC, 1);
  Serial.print(F(" C, "));
  Serial.print(humidity, 1);
  Serial.println(F(" % RH"));
}

void endDht22() {
  // No shutdown operation is required for this sensor.
}

void setup() {
  Serial.begin(115200);
  delay(1500);
  descriptorEeprom.begin();
  Serial.println(F("Modular UAV payload manager started."));
  Serial.println(F("Waiting for PDS-1.0 payload EEPROM at 0x50..."));
}

void loop() {
  if (millis() - lastScanAt >= PAYLOAD_SCAN_INTERVAL_MS) {
    lastScanAt = millis();
    discoverPayload();
  }
  if (activeDriver != nullptr && activeDriver->update != nullptr) {
    activeDriver->update();
  }
}
