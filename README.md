# STM32 Binary Clock Lab

This repository is a development sandbox for a binary clock built around
different STM32 microcontrollers and peripheral combinations. It contains the
current clock firmware, earlier hardware ports, reusable device drivers, board
experiments, KiCad sources, and end-user documentation.

The project is intentionally broader than a single finished product: it is used
to test display concepts, sensors, RTC implementations, power management, alarm
behavior, and small-memory firmware optimizations on real hardware.

## MCU variants

- **STM32L010F4P6** - current primary target and the most complete low-power build.
- **STM32F030F4P6** - compact CubeMX/HAL port used during early hardware tests.
- **STM32F407G-DISC1** - original prototype and development platform.

The default CMake configuration currently builds the STM32L010F4P6 firmware.
Board-specific notes and wiring are kept under [`Core/Src/boards`](Core/Src/boards).

## Hardware and drivers

The repository includes implementations or experiments for:

- a 16-LED binary display driven by chained 74HC595 shift registers;
- a 128x32 OLED display;
- an internal STM32 RTC clocked from a 32.768 kHz LSE crystal;
- the external DS1302 RTC used by earlier board versions;
- AHT10 and DHT11 temperature/humidity sensors;
- BH1750 and analog LDR light sensing with automatic display brightness;
- an ADC resistor ladder for five control buttons;
- active-buzzer alarm output and three persistent alarm slots;
- PIR motion detection, idle display shutdown, battery measurement, and standby
  power handling.

Separate branches also preserve display and peripheral experiments such as the
MAX721x 8x8 LED matrix version.

## Firmware layout

- `Core/Src/app` - clock behavior, alarms, display rendering, environment data,
  automatic modes, and user-interface state.
- `Core/Src/boards` - MCU/board-specific GPIO and peripheral implementations.
- `Core/Src/drivers` - reusable drivers for buttons, buzzer, DHT11, DS1302,
  light sensing, and shift registers.
- `Drivers` - STM32 CMSIS and HAL sources.
- `docs` - user guide, technical reference, diagrams, and promotional assets.
- `Clocl i2C l0 16 -2 595` - KiCad schematic and PCB sources for the current
  STM32L010-based hardware.

## Build

The current target requires CMake, Ninja, and the GNU Arm Embedded toolchain.

```bash
cmake --preset Release
cmake --build --preset Release
```

Debug and Release presets place their output under `build/`.

## Documentation

- [User manual](docs/clock-plus-user-manual.md)
- [Technical reference](docs/clock-plus-technical-reference.md)
- [STM32L010 board notes](Core/Src/boards/l010f4p6/README.md)
- [STM32F030 board notes](Core/Src/boards/f030f4p6/README.md)
- [STM32F407 board notes](Core/Src/boards/f407_disc1/README.md)

This is an experimental hardware/firmware repository. Verify the selected board
configuration, supply voltage, pin mapping, and programming setup before using
it on assembled hardware.
