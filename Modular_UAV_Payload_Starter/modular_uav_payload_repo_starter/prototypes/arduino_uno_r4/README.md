# Arduino Uno R4 Payload Prototype

This directory contains the breadboard proof of concept for PDS-1.0 payload
discovery. It is intentionally separated from the future Raspberry Pi/main
design.

## Contents

```text
arduino_uno_r4/
|-- README.md
|-- docs/
|   `-- DHT22_Prototype_Guide.md
`-- firmware/
    |-- eeprom_programmer_dht22/
    |   `-- eeprom_programmer_dht22.ino
    |-- payload_descriptor_decoder/
    |   `-- payload_descriptor_decoder.ino
    |-- backplane_controller/
    |   `-- backplane_controller.ino
    `-- libraries/
        `-- UavPayload/
            |-- library.properties
            `-- src/
                |-- UavPayload.h
                `-- UavPayload.cpp
```

## Upload order

1. Upload `firmware/eeprom_programmer_dht22/eeprom_programmer_dht22.ino` once to
   write and verify the DHT22 descriptor.
2. Upload `firmware/payload_descriptor_decoder/payload_descriptor_decoder.ino`
   to inspect the stored PDS-1.0 data independently.
3. Upload `firmware/backplane_controller/backplane_controller.ino` to discover
   the payload and run its DHT22 driver.

Install `firmware/libraries/UavPayload` as a local Arduino library. The payload
manager also requires Adafruit's DHT sensor library and Adafruit Unified Sensor.

See `docs/DHT22_Prototype_Guide.md` for wiring and detailed setup instructions.
