#include <UavPayload.h>

uav::DescriptorEeprom descriptorEeprom;

void setup() {
  Serial.begin(115200);
  delay(1500);
  descriptorEeprom.begin();

  Serial.println(F("PDS-1.0 payload descriptor decoder"));
  uav::PayloadDescriptor descriptor;
  const uav::DescriptorStatus status = descriptorEeprom.read(descriptor);
  if (status != uav::DescriptorStatus::Ok) {
    Serial.print(F("Descriptor rejected: "));
    Serial.println(uav::statusName(status));
    return;
  }

  uav::printDescriptor(Serial, descriptor);
}

void loop() {}
