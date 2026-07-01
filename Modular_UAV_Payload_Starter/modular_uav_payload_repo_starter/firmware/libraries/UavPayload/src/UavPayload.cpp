#include "UavPayload.h"

#include <string.h>

namespace uav {
namespace {

void encodeText(uint8_t *destination, size_t length, const char *source) {
  memset(destination, 0, length);
  if (source != nullptr) strncpy(reinterpret_cast<char *>(destination), source, length);
}

void decodeText(char *destination, size_t destinationSize,
                const uint8_t *source, size_t sourceSize) {
  memcpy(destination, source, sourceSize);
  destination[(sourceSize < destinationSize) ? sourceSize : destinationSize - 1] = '\0';
}

}  // namespace

DescriptorEeprom::DescriptorEeprom(TwoWire &wire, uint8_t deviceAddress)
    : _wire(wire), _deviceAddress(deviceAddress) {}

void DescriptorEeprom::begin() { _wire.begin(); }

bool DescriptorEeprom::isPresent() {
  _wire.beginTransmission(_deviceAddress);
  return _wire.endTransmission() == 0;
}

bool DescriptorEeprom::readByte(uint16_t memoryAddress, uint8_t &value) {
  _wire.beginTransmission(_deviceAddress);
  _wire.write(static_cast<uint8_t>(memoryAddress >> 8));
  _wire.write(static_cast<uint8_t>(memoryAddress & 0xFF));
  if (_wire.endTransmission(false) != 0) return false;
  if (_wire.requestFrom(_deviceAddress, static_cast<uint8_t>(1)) != 1) return false;
  value = _wire.read();
  return true;
}

bool DescriptorEeprom::writeByte(uint16_t memoryAddress, uint8_t value) {
  _wire.beginTransmission(_deviceAddress);
  _wire.write(static_cast<uint8_t>(memoryAddress >> 8));
  _wire.write(static_cast<uint8_t>(memoryAddress & 0xFF));
  _wire.write(value);
  return _wire.endTransmission() == 0 && waitUntilReady();
}

bool DescriptorEeprom::waitUntilReady(uint16_t timeoutMs) {
  const uint32_t startedAt = millis();
  do {
    if (isPresent()) return true;
    delay(1);
  } while (millis() - startedAt < timeoutMs);
  return false;
}

bool DescriptorEeprom::readRaw(uint8_t raw[PDS_SIZE]) {
  for (uint16_t address = 0; address < PDS_SIZE; ++address) {
    if (!readByte(address, raw[address])) return false;
  }
  return true;
}

bool DescriptorEeprom::writeRaw(const uint8_t raw[PDS_SIZE]) {
  for (uint16_t address = 0; address < PDS_SIZE; ++address) {
    if (!writeByte(address, raw[address])) return false;
  }
  return true;
}

DescriptorStatus DescriptorEeprom::read(PayloadDescriptor &descriptor) {
  if (!isPresent()) return DescriptorStatus::EepromNotFound;
  uint8_t raw[PDS_SIZE];
  if (!readRaw(raw)) return DescriptorStatus::ReadFailed;
  return decodeDescriptor(raw, descriptor);
}

bool DescriptorEeprom::write(const PayloadDescriptor &descriptor) {
  uint8_t raw[PDS_SIZE];
  encodeDescriptor(descriptor, raw);
  return writeRaw(raw);
}

PayloadDescriptor makeDht22Descriptor() {
  PayloadDescriptor descriptor = {};
  memcpy(descriptor.magic, "UAVP", 5);
  descriptor.version = PDS_VERSION;
  descriptor.payloadType = PayloadType::EnvironmentalSensor;
  descriptor.manufacturerId = 1;
  descriptor.hardwareRevision = 1;
  descriptor.requiredVoltage = RequiredVoltage::V5;
  descriptor.maxCurrentMilliAmps = 100;
  descriptor.commInterface = CommInterface::SingleWire;
  descriptor.primaryBus = PrimaryBus::GpioDigital;
  strncpy(descriptor.name, "DHT22_ENV", sizeof(descriptor.name) - 1);
  strncpy(descriptor.description, "DHT22 temp humidity sensor",
          sizeof(descriptor.description) - 1);
  return descriptor;
}

void encodeDescriptor(const PayloadDescriptor &descriptor, uint8_t raw[PDS_SIZE]) {
  memset(raw, 0, PDS_SIZE);
  memcpy(raw, "UAVP", 4);
  raw[4] = descriptor.version;
  raw[5] = static_cast<uint8_t>(descriptor.payloadType);
  raw[6] = descriptor.manufacturerId;
  raw[7] = descriptor.hardwareRevision;
  raw[8] = static_cast<uint8_t>(descriptor.requiredVoltage);
  raw[9] = static_cast<uint8_t>(descriptor.maxCurrentMilliAmps >> 8);
  raw[10] = static_cast<uint8_t>(descriptor.maxCurrentMilliAmps & 0xFF);
  raw[11] = static_cast<uint8_t>(descriptor.commInterface);
  raw[12] = static_cast<uint8_t>(descriptor.primaryBus);
  raw[13] = descriptor.flags;
  encodeText(raw + 14, 16, descriptor.name);
  encodeText(raw + 30, 32, descriptor.description);
  raw[62] = calculateChecksum(raw);
  raw[63] = 0;
}

DescriptorStatus decodeDescriptor(const uint8_t raw[PDS_SIZE],
                                  PayloadDescriptor &descriptor) {
  if (memcmp(raw, "UAVP", 4) != 0) return DescriptorStatus::InvalidMagic;
  if (raw[4] != PDS_VERSION) return DescriptorStatus::UnsupportedVersion;
  if (calculateChecksum(raw) != raw[62]) return DescriptorStatus::InvalidChecksum;

  memset(&descriptor, 0, sizeof(descriptor));
  memcpy(descriptor.magic, raw, 4);
  descriptor.version = raw[4];
  descriptor.payloadType = static_cast<PayloadType>(raw[5]);
  descriptor.manufacturerId = raw[6];
  descriptor.hardwareRevision = raw[7];
  descriptor.requiredVoltage = static_cast<RequiredVoltage>(raw[8]);
  descriptor.maxCurrentMilliAmps =
      (static_cast<uint16_t>(raw[9]) << 8) | raw[10];
  descriptor.commInterface = static_cast<CommInterface>(raw[11]);
  descriptor.primaryBus = static_cast<PrimaryBus>(raw[12]);
  descriptor.flags = raw[13];
  decodeText(descriptor.name, sizeof(descriptor.name), raw + 14, 16);
  decodeText(descriptor.description, sizeof(descriptor.description), raw + 30, 32);
  descriptor.checksum = raw[62];
  descriptor.reserved = raw[63];
  return DescriptorStatus::Ok;
}

uint8_t calculateChecksum(const uint8_t raw[PDS_SIZE]) {
  uint8_t checksum = 0;
  for (size_t index = 0; index < 62; ++index) checksum ^= raw[index];
  return checksum;
}

const char *statusName(DescriptorStatus status) {
  switch (status) {
    case DescriptorStatus::Ok: return "OK";
    case DescriptorStatus::EepromNotFound: return "EEPROM not found";
    case DescriptorStatus::ReadFailed: return "EEPROM read failed";
    case DescriptorStatus::InvalidMagic: return "invalid UAVP magic";
    case DescriptorStatus::UnsupportedVersion: return "unsupported PDS version";
    case DescriptorStatus::InvalidChecksum: return "invalid checksum";
  }
  return "unknown status";
}

const char *payloadTypeName(PayloadType type) {
  switch (type) {
    case PayloadType::EnvironmentalSensor: return "Environmental sensor";
    case PayloadType::RgbCamera: return "RGB camera";
    case PayloadType::ThermalCamera: return "Thermal camera";
    case PayloadType::Lidar: return "LiDAR/range sensor";
    case PayloadType::GasSensor: return "Gas sensor";
    case PayloadType::ServoActuator: return "Servo/actuator";
    case PayloadType::RadioTelemetry: return "Radio/telemetry";
    case PayloadType::DropMechanism: return "Drop mechanism";
    case PayloadType::Lighting: return "Lighting/spotlight";
    case PayloadType::Custom: return "Custom payload";
    default: return "Unknown";
  }
}

const char *voltageName(RequiredVoltage voltage) {
  switch (voltage) {
    case RequiredVoltage::V3_3: return "3.3 V";
    case RequiredVoltage::V5: return "5 V";
    case RequiredVoltage::V12: return "12 V";
    case RequiredVoltage::Vbat: return "VBAT/raw battery";
    default: return "Unknown";
  }
}

const char *commInterfaceName(CommInterface interfaceType) {
  switch (interfaceType) {
    case CommInterface::Usb: return "USB";
    case CommInterface::Uart: return "UART";
    case CommInterface::I2c: return "I2C";
    case CommInterface::Spi: return "SPI";
    case CommInterface::Can: return "CAN";
    case CommInterface::Gpio: return "GPIO";
    case CommInterface::Pwm: return "PWM";
    case CommInterface::SingleWire: return "Single-wire sensor";
    default: return "None";
  }
}

const char *primaryBusName(PrimaryBus bus) {
  switch (bus) {
    case PrimaryBus::Usb0: return "USB0";
    case PrimaryBus::Uart0: return "UART0";
    case PrimaryBus::Uart1: return "UART1";
    case PrimaryBus::I2c0: return "I2C0";
    case PrimaryBus::I2c1: return "I2C1";
    case PrimaryBus::Can0: return "CAN0";
    case PrimaryBus::Can1: return "CAN1";
    case PrimaryBus::GpioDigital: return "GPIO/digital pin";
    case PrimaryBus::PwmOutput: return "PWM output";
    default: return "Not applicable";
  }
}

void printDescriptor(Stream &output, const PayloadDescriptor &descriptor) {
  output.println(F("----- PDS-1.0 Payload Descriptor -----"));
  output.print(F("Magic: ")); output.println(descriptor.magic);
  output.print(F("Version: ")); output.println(descriptor.version);
  output.print(F("Payload type: ")); output.println(payloadTypeName(descriptor.payloadType));
  output.print(F("Manufacturer ID: ")); output.println(descriptor.manufacturerId);
  output.print(F("Hardware revision: ")); output.println(descriptor.hardwareRevision);
  output.print(F("Required voltage: ")); output.println(voltageName(descriptor.requiredVoltage));
  output.print(F("Maximum current: ")); output.print(descriptor.maxCurrentMilliAmps); output.println(F(" mA"));
  output.print(F("Communication: ")); output.println(commInterfaceName(descriptor.commInterface));
  output.print(F("Primary bus: ")); output.println(primaryBusName(descriptor.primaryBus));
  output.print(F("Flags: 0x")); output.println(descriptor.flags, HEX);
  output.print(F("Name: ")); output.println(descriptor.name);
  output.print(F("Description: ")); output.println(descriptor.description);
  output.print(F("Checksum: 0x")); output.println(descriptor.checksum, HEX);
  output.println(F("--------------------------------------"));
}

}  // namespace uav
