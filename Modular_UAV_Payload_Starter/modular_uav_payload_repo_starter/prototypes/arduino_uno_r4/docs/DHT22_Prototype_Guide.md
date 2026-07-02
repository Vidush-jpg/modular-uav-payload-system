# DHT22 Payload Prototype

This prototype proves the PDS-1.0 discovery path: program a payload identity
EEPROM, validate its descriptor, select a matching driver, and read live data.

## Wiring the four-pin DHT22

Disconnect USB and all other power before changing wiring. With the bare DHT22's
vented front/grid facing you and its pins pointing down, pins run left to right:

| Pin | Function | Arduino Uno R4 connection |
|---:|---|---|
| 1 | VCC | 5V |
| 2 | DATA | D2, plus a 10k resistor from DATA to 5V |
| 3 | NC | Leave disconnected |
| 4 | GND | GND |

The third pin is intentionally unused. Three-pin DHT22 modules hide that pin and
often include the pull-up resistor; a bare four-pin sensor needs the external
pull-up.

## Wiring the AT24LC256

| EEPROM pin | Connection |
|---:|---|
| 1 A0 | GND |
| 2 A1 | GND |
| 3 A2 | GND |
| 4 VSS | GND |
| 5 SDA | Uno R4 SDA |
| 6 SCL | Uno R4 SCL |
| 7 WP | GND while programming |
| 8 VCC | 5V |

Add one 10k pull-up from SDA to 5V and one from SCL to 5V. All devices must
share ground. Place a 0.1 uF ceramic capacitor close to the EEPROM between VCC
and GND. A 10-100 uF bulk capacitor across payload 5V and GND is also useful.

## Arduino setup and upload order

Install **DHT sensor library by Adafruit** and **Adafruit Unified Sensor** using
Arduino IDE Library Manager.

The sketches share `prototypes/arduino_uno_r4/firmware/libraries/UavPayload`.
Install that folder as a local Arduino library, or compile from the repository
root with Arduino CLI using
`--libraries prototypes/arduino_uno_r4/firmware/libraries`.

Open Serial Monitor at 115200 baud and use the sketches in this order:

1. Upload `prototypes/arduino_uno_r4/firmware/eeprom_programmer_dht22/eeprom_programmer_dht22.ino` once.
   Confirm that read-back verification passes.
2. Upload `prototypes/arduino_uno_r4/firmware/payload_descriptor_decoder/payload_descriptor_decoder.ino`
   to independently inspect and validate the stored descriptor.
3. Upload `prototypes/arduino_uno_r4/firmware/backplane_controller/backplane_controller.ino`.
   It detects
   the EEPROM, validates the descriptor, selects the DHT22 driver, and reports
   measurements every two seconds.

After programming, WP may be connected to 5V to prevent accidental writes. The
decoder and payload manager only read the EEPROM.

## Descriptor storage design

EEPROM bytes 0-63 contain the portable PDS-1.0 binary format exactly as defined
in `Payload_Descriptor_Specification.md`. Text is fixed-width null-padded ASCII,
maximum current is big-endian, and byte 62 is the XOR of bytes 0-61.

Firmware decodes those bytes into `uav::PayloadDescriptor`, a convenient C++
struct. The struct is not copied directly into EEPROM: compiler padding, enum
size, and processor byte order can differ between the Uno R4 and Raspberry Pi.
Explicit encoding preserves a stable hardware contract and will make a later
Python implementation straightforward.

## Adding drivers later

Implement `begin`, `update`, and `end` functions, then register them in the
`drivers` array in `backplane_controller.ino`, keyed by payload type and
communication interface. UART, CAN, I2C, USB, and other drivers can initialize
their transport in `begin` without changing discovery or descriptor validation.

This prototype detects attachment by polling for the EEPROM at address `0x50`.
The final backplane should use its DETECT pin and controlled payload power. This
breadboard circuit is not hot-swappable: power off before connecting or removing
the payload.
