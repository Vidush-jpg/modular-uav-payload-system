#include <UavPayload.h>

uav::DescriptorEeprom descriptorEeprom;

void setup() {
  Serial.begin(115200);
  delay(1500);
  descriptorEeprom.begin();

  Serial.println(F("PDS-1.0 DHT22 EEPROM programmer"));
  if (!descriptorEeprom.isPresent()) {
    Serial.println(F("ERROR: AT24LC256 not found at I2C address 0x50."));
    Serial.println(F("Check power, SDA/SCL, address pins, and pull-ups."));
    return;
  }

  const uav::PayloadDescriptor expected = uav::makeDht22Descriptor();
  if (!descriptorEeprom.write(expected)) {
    Serial.println(F("ERROR: write failed. Connect EEPROM WP pin 7 to GND."));
    return;
  }

  uav::PayloadDescriptor verified;
  const uav::DescriptorStatus status = descriptorEeprom.read(verified);
  if (status != uav::DescriptorStatus::Ok) {
    Serial.print(F("ERROR: read-back verification failed: "));
    Serial.println(uav::statusName(status));
    return;
  }

  Serial.println(F("EEPROM programmed and read-back verification passed."));
  uav::printDescriptor(Serial, verified);
  Serial.println(F("Power off, then upload the decoder or payload manager."));
}

void loop() {}
