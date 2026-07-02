# Modular FPV UAV Payload Backplane — Codex Handoff Notes

## Project Goal

Design a modular payload FPV UAV where payloads can be swapped using a standardized connector/backplane system. Payloads may include RGB camera, thermal camera, telemetry module, LiDAR, environmental sensors, gas sensor, drop mechanism, etc.

The project architecture is intentionally split into:

- Flight controller: flies the UAV.
- ESCs and motors: propulsion.
- FPV system: low-latency pilot video.
- Companion computer: Raspberry Pi/Jetson-style high-level payload computer.
- Backplane board: payload power, identification, routing, and protection.
- Payload modules: mission-specific electronics with EEPROM descriptor.

The flight controller should remain mostly independent from payloads.

---

## Core Architecture

```text
Battery
  |
  +--> Propulsion power --> ESCs --> Motors
  |
  +--> Flight electronics power --> Flight controller, RC receiver, GPS, telemetry
  |
  +--> Payload electronics power --> Payload backplane --> Modular payloads
```

Recommended responsibility split:

```text
Flight Controller:
- Stabilization
- Motor control
- GPS/nav/failsafes
- MAVLink communication

Raspberry Pi / Companion Computer:
- Payload manager
- Camera/video processing
- AI/computer vision
- Data streaming
- Ground-station communication
- MAVLink high-level mission commands

Arduino Uno R4 WiFi / Backplane MCU:
- EEPROM read prototype
- Detect pin monitoring
- Load-switch control
- Current monitoring
- Fault response in future V2

Backplane:
- Regulated payload rails
- Load switches/eFuses
- EEPROM ID rail
- I2C pullups
- Data routing
- Connector interface
- Current monitoring
- ESD/protection
```

---

## Current Prototype Hardware

Current prototype uses:

- Arduino Uno R4 WiFi as temporary backplane controller.
- AT24LC256 EEPROM as the payload identity device.
- DHT22 sensor as a test payload.
- I2C scanner confirmed EEPROM responds at `0x50`.

AT24LC256 wiring:

```text
Pin 1 A0  -> GND
Pin 2 A1  -> GND
Pin 3 A2  -> GND
Pin 4 VSS -> GND
Pin 5 SDA -> Arduino SDA
Pin 6 SCL -> Arduino SCL
Pin 7 WP  -> GND for writable mode
Pin 8 VCC -> 5V
```

I2C pullups:

```text
SDA line -> 10k -> 5V
SCL line -> 10k -> 5V
```

DHT22 wiring:

```text
DHT22 VCC  -> 5V
DHT22 DATA -> Arduino D2
DHT22 GND  -> GND
DATA line  -> 10k pullup -> 5V
```

Capacitor:

```text
100 uF electrolytic across 5V and GND
Long lead = +5V
Short lead / stripe = GND
```

For final PCB also add 0.1 uF ceramic decoupling capacitors close to each IC.

---

## EEPROM / Payload Descriptor Concept

The EEPROM does not store live sensor data. It stores an identity/configuration descriptor.

A payload descriptor allows the backplane/companion computer to ask:

```text
What payload is connected?
What voltage does it need?
How much current can it draw?
What communication interface does it use?
What driver/software should be loaded?
```

This is similar in concept to USB descriptors, PCIe configuration space, or smart battery identification.

---

## Payload Descriptor Specification PDS-1.0

Use the first 64 bytes of EEPROM.

| Byte Address | Field | Size | Type | Meaning |
|---:|---|---:|---|---|
| 0–3 | Magic | 4 | ASCII | Must be `UAVP` |
| 4 | Descriptor Version | 1 | uint8 | Use `1` |
| 5 | Payload Type | 1 | enum | General payload category |
| 6 | Manufacturer ID | 1 | enum/custom | Maker/team ID |
| 7 | Hardware Revision | 1 | uint8 | Payload PCB/module revision |
| 8 | Required Voltage | 1 | enum | Main payload voltage |
| 9–10 | Maximum Current | 2 | uint16 big-endian | mA |
| 11 | Communication Interface | 1 | enum | USB/UART/I2C/CAN/GPIO/etc. |
| 12 | Primary Bus | 1 | enum/custom | Specific bus/pin choice |
| 13 | Flags | 1 | bitfield | Extra properties |
| 14–29 | Payload Name | 16 | ASCII null-padded | Short name |
| 30–61 | Description | 32 | ASCII null-padded | Human-readable description |
| 62 | Checksum | 1 | uint8 | XOR of bytes 0–61 |
| 63 | Reserved | 1 | uint8 | Use 0x00 |

Recommended EEPROM memory use:

```text
0x0000–0x003F: Payload descriptor
0x0040–0x7FFF: Calibration/serial/future payload data
```

---

## Enum Values

### Payload Type

```text
0  Unknown
1  Environmental sensor
2  RGB camera
3  Thermal camera
4  LiDAR / range sensor
5  Gas sensor
6  Servo / actuator
7  Radio / telemetry
8  Drop mechanism
9  Lighting / spotlight
10 Custom payload
```

### Required Voltage

```text
0 Unknown
1 3.3V
2 5V
3 12V
4 VBAT/raw battery
```

### Communication Interface

```text
0 None
1 USB
2 UART
3 I2C
4 SPI
5 CAN
6 GPIO
7 PWM
8 Single-wire sensor
```

### Primary Bus

```text
0 Not applicable
1 USB0
2 UART0
3 UART1
4 I2C0
5 I2C1
6 CAN0
7 CAN1
8 GPIO / digital pin
9 PWM output
```

### Flags

```text
Bit 0 / 0x01 = Requires calibration
Bit 1 / 0x02 = Camera payload
Bit 2 / 0x04 = Actuator payload
Bit 3 / 0x08 = Requires cooling
Bit 4 / 0x10 = Logs data onboard
Bit 5 / 0x20 = High current payload
Bit 6 / 0x40 = Reserved
Bit 7 / 0x80 = Reserved
```

---

## DHT22 Test Payload Descriptor

```text
Magic: UAVP
Descriptor version: 1
Payload type: 1 = Environmental sensor
Manufacturer ID: 1 = Team/custom
Hardware revision: 1
Required voltage: 2 = 5V
Max current: 100 mA
Communication interface: 8 = Single-wire sensor
Primary bus: 8 = GPIO/digital pin
Flags: 0x00
Payload name: DHT22_ENV
Description: DHT22 temp humidity sensor
Checksum: XOR of bytes 0–61
Reserved: 0x00
```

---

## Backplane Connector Concept

One rugged connector per payload slot, possibly plus a separate USB connector if high-speed routing becomes difficult.

Suggested 20-pin payload connector:

```text
1   GND
2   GND
3   3.3V_ID
4   I2C_SDA
5   I2C_SCL
6   DETECT
7   5V_PAYLOAD
8   5V_PAYLOAD
9   12V_PAYLOAD
10  12V_PAYLOAD
11  USB_D+
12  USB_D-
13  UART_TX_TO_PAYLOAD
14  UART_RX_FROM_PAYLOAD
15  CAN_H
16  CAN_L
17  GPIO_1
18  GPIO_2
19  PWM/SERVO
20  RESERVED
```

Important:
- Use multiple ground pins.
- Put grounds near high-speed/noisy signals.
- Use multiple pins for power if current is >1 A.
- Key and mechanically lock the connector.

Connector options discussed:
- Molex Micro-Fit 3.0: recommended for hobby/prototype robustness.
- JST-GH: good for signals, not high-current payload power.
- Samtec board-to-board: professional but harder/more expensive.
- USB-C: possible for USB only, not ideal as universal payload connector.
- M.2/RAM-style edge connector: elegant but risky for vibration unless mechanically retained.

---

## Backplane Routing Rules

### I2C

Use for:
- EEPROM payload ID
- Current monitor ICs such as INA219/INA226/INA260

Rules:
- One set of pullups per bus, owned by backplane.
- 10k for 100 kHz is okay with AT24LC256.
- Keep lines short.
- Avoid using I2C for long/noisy cable runs.
- Payload modules should avoid their own fixed pullups unless they are removable.

### USB 2.0

Use for:
- RGB cameras
- Thermal cameras
- High-data payloads

Rules:
- Route D+ and D− as a differential pair.
- Keep short, close together, and length-matched.
- Target 90 ohm differential impedance where possible.
- Add ESD protection near connectors.
- Avoid stubs.
- Avoid routing near buck converters/ESC power.
- For multiple USB payload slots, use a USB hub.

### UART

Use for:
- Simple sensors
- GPS/rangefinders
- Simple telemetry
- Small payload MCUs

Rules:
- Payload TX -> companion/backplane RX
- Payload RX -> companion/backplane TX
- Common ground required.
- Check 3.3V vs 5V logic.
- Add level shifter if needed.
- Optional 33–100 ohm series resistors.

### CAN

Use for:
- Robust payload control/telemetry
- Smart payloads
- Longer/noisier connections

Rules:
- Raspberry Pi needs CAN adapter/transceiver.
- Arduino/MCU also needs CAN transceiver unless built in.
- Route CAN_H and CAN_L as a pair.
- 120 ohm termination only at the two physical ends of the bus.
- Add TVS/ESD protection at connectors.
- Common ground/reference is recommended.

---

## V1 vs V2 Backplane

### V1: Passive smart backplane

No MCU required, or Arduino only for prototype.

Backplane includes:
- Connector
- 3.3V ID rail
- I2C pullups
- EEPROM lines
- Detect pin
- 5V/12V regulators
- Load switches
- Current monitors
- USB/UART/CAN routing
- ESD protection
- Fuses/TVS/capacitors

Companion computer or Arduino directly reads:
- EEPROM
- detect pin
- current sensors
- load-switch fault pins

### V2: Reflex-arc backplane

Add MCU supervisor such as Arduino Uno R4, RP2040, STM32, etc.

MCU handles:
- Detect payload
- Power sequencing
- Load switch enable/disable
- Current/fault monitoring
- Watchdog for companion computer
- Immediate short-circuit shutdown
- Fault logging
- Status reporting

Companion computer handles:
- Payload software
- Video
- AI
- Networking
- MAVLink
- Driver loading

---

## No Hot-Swap Decision

Hot-swap was considered but rejected for now because it adds risk and complexity.

Current rule:

```text
Power off -> insert payload -> power on -> read EEPROM -> enable payload rail -> start communication
```

Still include:
- keyed connector
- detect pin
- load switches
- current monitoring
- ESD protection
- do-not-remove-while-powered rule

---

## FPV and Flight Electronics

Keep pilot FPV independent from payload camera.

Recommended separation:

```text
Pilot FPV:
FPV camera -> video transmitter -> goggles

Payload RGB camera:
USB camera -> payload backplane -> Raspberry Pi -> ground laptop/network
```

The pilot should not depend on the payload camera feed.

Flight controller should communicate with companion computer via MAVLink only.

---

## RGB Camera Payload Recommendation

For V1, use a USB camera module rather than CSI/MIPI.

Reasons:
- USB is easier to route than MIPI CSI.
- Raspberry Pi supports UVC cameras easily.
- Avoids high-speed MIPI signal integrity issues.
- Keeps modular backplane simpler.

Recommended options:
- Arducam USB camera module
- ELP USB camera module
- Waveshare USB camera
- Generic UVC camera module

Payload board for USB RGB camera:
- Payload connector
- EEPROM
- Power filtering
- USB D+/D− routing
- Mounting holes
- Camera module connection

---

## Power Filtering Notes

Each payload should have local filtering:

```text
Payload power input
  |
  ferrite bead optional
  |
  10–100 uF bulk capacitor
  |
  0.1 uF ceramic near each IC
```

On breadboard:
- 100 uF across 5V and GND is okay.
- Long lead to +5V, short/stripe to GND.

On PCB:
- Add 0.1 uF near EEPROM VCC.
- Add 10 uF or 100 uF near payload power input.

---

## Arduino Prototype Lessons

I2C scanner confirmed EEPROM at `0x50`.

Important learning:
- Pullups go from SDA/SCL bus lines to VCC, not in series.
- I2C devices use open-drain/open-collector style behavior.
- Devices pull the line LOW but do not actively drive it HIGH.
- The pullup resistor creates the HIGH state.

AT24LC256 address:
- Base control code is `1010`.
- A2/A1/A0 tied GND gives I2C address `0x50`.

WP behavior:
- WP tied to GND/VSS means writes enabled.
- WP tied to VCC means writes blocked; reads still work.

---

## Code Needed Next

Create Arduino code that:

1. Programs the AT24LC256 with the PDS-1.0 DHT22 descriptor.
2. Reads and decodes any PDS-1.0 descriptor.
3. Verifies magic `UAVP`.
4. Verifies checksum.
5. Prints decoded fields to Serial.
6. If payload type is environmental sensor and comm interface is single-wire, reads DHT22 data from D2.

---

## Suggested Repository Structure

> Current organization note: all Arduino Uno R4 proof-of-concept firmware now
> lives under `prototypes/arduino_uno_r4/firmware`, with its prototype-specific
> documentation under `prototypes/arduino_uno_r4/docs`. The `raspberry_pi`
> directory is reserved for the main companion-computer implementation. The
> older tree below records the original planning proposal.

```text
Modular-UAV/
├── README.md
├── docs/
│   ├── Payload_Descriptor_Specification.md
│   ├── Backplane_Architecture.md
│   ├── Connector_Pinout.md
│   ├── Bus_Routing_Notes.md
│   └── Codex_Handoff.md
├── firmware/
│   ├── eeprom_programmer_dht22/
│   ├── payload_descriptor_decoder/
│   └── backplane_controller/
├── payloads/
│   ├── DHT22/
│   ├── RGB_Camera/
│   ├── Thermal_Camera/
│   └── LiDAR/
├── raspberry_pi/
│   ├── payload_manager/
│   ├── drivers/
│   └── streaming/
├── hardware/
│   ├── backplane/
│   ├── payload_modules/
│   └── connector_pinouts/
└── tests/
```

---

## Immediate Next Milestone

Build the DHT22 payload module:

1. EEPROM contains valid PDS-1.0 descriptor.
2. Arduino reads descriptor.
3. Arduino prints decoded payload info.
4. Arduino initializes DHT22 only if descriptor says correct payload.
5. Arduino prints temperature/humidity.

This proves:
- payload inserted
- EEPROM detected
- payload identified
- payload driver selected
- payload data read
