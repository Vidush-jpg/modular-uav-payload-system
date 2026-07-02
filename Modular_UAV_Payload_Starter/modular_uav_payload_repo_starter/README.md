# Modular UAV Payload System

A modular FPV UAV payload backplane project.

Start with `docs/Codex_Handoff.md`.

## Repository layout

```text
docs/                         System specifications and architecture
prototypes/arduino_uno_r4/    Breadboard proof-of-concept firmware and guides
raspberry_pi/                 Reserved for the main companion-computer design
```

The Arduino Uno R4 implementation is intentionally isolated under
`prototypes/arduino_uno_r4`. New Raspberry Pi production software should not be
placed inside the prototype directory.

See `prototypes/arduino_uno_r4/README.md` for the prototype upload order and
source layout.
