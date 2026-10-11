# STWINBX1 Firmware Prototypes

Embedded C experiments for the STMicroelectronics STWINBX1 sensor board, including IIS3DWB sensor initialization and FIFO, DMA, and UART work.

## Repository guide

- [`Prototype_1/`](Prototype_1/) — firmware project and supporting source files.
- [Documentation for STWINBX1](Documentation%20for%20STWINBX1.pdf) — the project's detailed PDF documentation.
- The repository branches preserve development experiments; start with `main` for the default snapshot.

## Getting started

1. Clone the repository: `git clone https://github.com/YumTaha/STWINBX1.git`.
2. Read the linked PDF for project-specific hardware and setup information.
3. Open the firmware project in `Prototype_1/` with the toolchain appropriate to its project files.
4. Review the selected branch and sensor configuration before building or flashing hardware.

## Scope

This repository contains development prototypes. The README is an entry point; hardware configuration and implementation details live in the source and PDF.

## Related project

[`stdatalog`](https://github.com/YumTaha/stdatalog) provides industrial sensor data collection, BLE-triggered recording, Linux services, and USB backup workflows for STWINBX1.
