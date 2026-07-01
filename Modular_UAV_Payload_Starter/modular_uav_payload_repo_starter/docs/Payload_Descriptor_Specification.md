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

