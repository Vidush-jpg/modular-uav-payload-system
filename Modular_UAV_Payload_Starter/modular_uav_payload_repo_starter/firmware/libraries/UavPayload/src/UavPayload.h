#pragma once

#include <Arduino.h>
#include <Wire.h>

namespace uav {

constexpr uint8_t PDS_VERSION = 1;
constexpr size_t PDS_SIZE = 64;
constexpr uint8_t DEFAULT_EEPROM_ADDRESS = 0x50;

enum class PayloadType : uint8_t {
  Unknown = 0, EnvironmentalSensor = 1, RgbCamera = 2,
  ThermalCamera = 3, Lidar = 4, GasSensor = 5,
  ServoActuator = 6, RadioTelemetry = 7, DropMechanism = 8,
  Lighting = 9, Custom = 10
};

enum class RequiredVoltage : uint8_t {
  Unknown = 0, V3_3 = 1, V5 = 2, V12 = 3, Vbat = 4
};

enum class CommInterface : uint8_t {
  None = 0, Usb = 1, Uart = 2, I2c = 3, Spi = 4,
  Can = 5, Gpio = 6, Pwm = 7, SingleWire = 8
};

enum class PrimaryBus : uint8_t {
  NotApplicable = 0, Usb0 = 1, Uart0 = 2, Uart1 = 3,
  I2c0 = 4, I2c1 = 5, Can0 = 6, Can1 = 7,
  GpioDigital = 8, PwmOutput = 9
};

struct PayloadDescriptor {
  char magic[5];
  uint8_t version;
  PayloadType payloadType;
  uint8_t manufacturerId;
  uint8_t hardwareRevision;
  RequiredVoltage requiredVoltage;
  uint16_t maxCurrentMilliAmps;
  CommInterface commInterface;
  PrimaryBus primaryBus;
  uint8_t flags;
  char name[17];
  char description[33];
  uint8_t checksum;
  uint8_t reserved;
};

enum class DescriptorStatus : uint8_t {
  Ok, EepromNotFound, ReadFailed, InvalidMagic,
  UnsupportedVersion, InvalidChecksum
};

class DescriptorEeprom {
 public:
  explicit DescriptorEeprom(TwoWire &wire = Wire,
                            uint8_t deviceAddress = DEFAULT_EEPROM_ADDRESS);
  void begin();
  bool isPresent();
  bool readRaw(uint8_t raw[PDS_SIZE]);
  bool writeRaw(const uint8_t raw[PDS_SIZE]);
  DescriptorStatus read(PayloadDescriptor &descriptor);
  bool write(const PayloadDescriptor &descriptor);

 private:
  bool readByte(uint16_t memoryAddress, uint8_t &value);
  bool writeByte(uint16_t memoryAddress, uint8_t value);
  bool waitUntilReady(uint16_t timeoutMs = 20);
  TwoWire &_wire;
  uint8_t _deviceAddress;
};

PayloadDescriptor makeDht22Descriptor();
void encodeDescriptor(const PayloadDescriptor &descriptor, uint8_t raw[PDS_SIZE]);
DescriptorStatus decodeDescriptor(const uint8_t raw[PDS_SIZE],
                                  PayloadDescriptor &descriptor);
uint8_t calculateChecksum(const uint8_t raw[PDS_SIZE]);
const char *statusName(DescriptorStatus status);
const char *payloadTypeName(PayloadType type);
const char *voltageName(RequiredVoltage voltage);
const char *commInterfaceName(CommInterface interfaceType);
const char *primaryBusName(PrimaryBus bus);
void printDescriptor(Stream &output, const PayloadDescriptor &descriptor);

}  // namespace uav
