#include <Wire.h>

#define EEPROM_ADDR 0x50

struct PayloadDescriptor {
  char magic[4];
  uint8_t version;
  uint8_t payloadType;
  uint8_t manufacturerID;
  uint8_t hardwareRevision;
  uint8_t voltage;
  uint16_t maxCurrent_mA;
  uint8_t commInterface;
  uint8_t primaryBus;
  uint8_t flags;
  char name[17];
  char description[33];
  uint8_t checksum;
  uint8_t reserved;
};

uint8_t readEEPROM(uint16_t addr) {
  Wire.beginTransmission(EEPROM_ADDR);
  Wire.write((addr >> 8) & 0xFF);
  Wire.write(addr & 0xFF);
  Wire.endTransmission();
  Wire.requestFrom(EEPROM_ADDR, 1);
  if (Wire.available()) return Wire.read();
  return 0xFF;
}

void writeEEPROM(uint16_t addr, uint8_t data) {
  Wire.beginTransmission(EEPROM_ADDR);
  Wire.write((addr >> 8) & 0xFF);
  Wire.write(addr & 0xFF);
  Wire.write(data);
  Wire.endTransmission();
  delay(5);
}

uint8_t xorChecksum(uint8_t *data, int len) {
  uint8_t c = 0;
  for (int i = 0; i < len; i++) c ^= data[i];
  return c;
}

void programDHT22Descriptor() {
  uint8_t d[64] = {0};

  d[0] = 'U'; d[1] = 'A'; d[2] = 'V'; d[3] = 'P';
  d[4] = 1;      // PDS version
  d[5] = 1;      // Environmental sensor
  d[6] = 1;      // Manufacturer/team ID
  d[7] = 1;      // Hardware revision
  d[8] = 2;      // 5V
  d[9] = 0x00;   // Max current high byte
  d[10] = 0x64;  // Max current low byte = 100 mA
  d[11] = 8;     // Single-wire sensor
  d[12] = 8;     // GPIO/digital pin
  d[13] = 0x00;  // Flags

  const char name[] = "DHT22_ENV";
  for (int i = 0; i < 16 && name[i] != '\0'; i++) d[14 + i] = name[i];

  const char desc[] = "DHT22 temp humidity sensor";
  for (int i = 0; i < 32 && desc[i] != '\0'; i++) d[30 + i] = desc[i];

  d[62] = xorChecksum(d, 62);
  d[63] = 0x00;

  for (int i = 0; i < 64; i++) {
    writeEEPROM(i, d[i]);
  }
}

bool readDescriptor(PayloadDescriptor &pd) {
  uint8_t raw[64];

  for (int i = 0; i < 64; i++) {
    raw[i] = readEEPROM(i);
  }

  if (!(raw[0] == 'U' && raw[1] == 'A' && raw[2] == 'V' && raw[3] == 'P')) {
    Serial.println("Invalid magic. Not a UAV payload descriptor.");
    return false;
  }

  uint8_t calc = xorChecksum(raw, 62);
  if (calc != raw[62]) {
    Serial.print("Checksum failed. Calculated 0x");
    Serial.print(calc, HEX);
    Serial.print(", stored 0x");
    Serial.println(raw[62], HEX);
    return false;
  }

  pd.magic[0] = raw[0]; pd.magic[1] = raw[1]; pd.magic[2] = raw[2]; pd.magic[3] = raw[3];
  pd.version = raw[4];
  pd.payloadType = raw[5];
  pd.manufacturerID = raw[6];
  pd.hardwareRevision = raw[7];
  pd.voltage = raw[8];
  pd.maxCurrent_mA = ((uint16_t)raw[9] << 8) | raw[10];
  pd.commInterface = raw[11];
  pd.primaryBus = raw[12];
  pd.flags = raw[13];

  for (int i = 0; i < 16; i++) pd.name[i] = (char)raw[14 + i];
  pd.name[16] = '\0';

  for (int i = 0; i < 32; i++) pd.description[i] = (char)raw[30 + i];
  pd.description[32] = '\0';

  pd.checksum = raw[62];
  pd.reserved = raw[63];

  return true;
}

const char* voltageName(uint8_t v) {
  switch (v) {
    case 1: return "3.3V";
    case 2: return "5V";
    case 3: return "12V";
    case 4: return "VBAT";
    default: return "Unknown";
  }
}

const char* commName(uint8_t c) {
  switch (c) {
    case 1: return "USB";
    case 2: return "UART";
    case 3: return "I2C";
    case 4: return "SPI";
    case 5: return "CAN";
    case 6: return "GPIO";
    case 7: return "PWM";
    case 8: return "Single-wire";
    default: return "None/Unknown";
  }
}

void printDescriptor(const PayloadDescriptor &pd) {
  Serial.println("----- Payload Descriptor -----");
  Serial.print("Magic: ");
  Serial.write(pd.magic, 4);
  Serial.println();
  Serial.print("Version: "); Serial.println(pd.version);
  Serial.print("Payload Type: "); Serial.println(pd.payloadType);
  Serial.print("Manufacturer ID: "); Serial.println(pd.manufacturerID);
  Serial.print("HW Revision: "); Serial.println(pd.hardwareRevision);
  Serial.print("Voltage: "); Serial.println(voltageName(pd.voltage));
  Serial.print("Max Current: "); Serial.print(pd.maxCurrent_mA); Serial.println(" mA");
  Serial.print("Communication: "); Serial.println(commName(pd.commInterface));
  Serial.print("Primary Bus: "); Serial.println(pd.primaryBus);
  Serial.print("Flags: 0x"); Serial.println(pd.flags, HEX);
  Serial.print("Name: "); Serial.println(pd.name);
  Serial.print("Description: "); Serial.println(pd.description);
  Serial.print("Checksum: 0x"); Serial.println(pd.checksum, HEX);
  Serial.println("------------------------------");
}

void setup() {
  Serial.begin(115200);
  delay(1500);
  Wire.begin();

  // Uncomment once to format the EEPROM as a DHT22 payload, then comment again.
  // programDHT22Descriptor();

  PayloadDescriptor pd;
  if (readDescriptor(pd)) {
    printDescriptor(pd);
  }
}

void loop() {}
