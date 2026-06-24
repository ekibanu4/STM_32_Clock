# Binary Clock - Phase 1 STM32F407 Prototype

This repository contains the first working hardware prototype of a binary clock
built on STM32F407G-DISC1. The goal of this phase is to prove the full feature
set on a large development board before moving to a smaller STM32 and HAL-based
project structure.

## Component Base

- STM32F407G-DISC1 development board
- 2x 74HC595N shift registers for 16 LED outputs
- DS1302Z real-time clock module with quartz and backup battery
- HW-481 / DHT11 temperature and humidity module
- LDR photoresistor with voltage divider for automatic brightness
- 5-button ADC resistor ladder
- TMB12A05 active buzzer
- NPN transistor for buzzer switching
- 270 ohm resistor in series with buzzer for acceptable volume
- 1k..4.7k resistor from MCU pin to NPN base
- Binary LED panel:
  - 6 LEDs for hour/month/temperature section
  - 6 LEDs for minute/day/humidity section
  - 3 mode/slot LEDs
  - 1 alarm status LED

## Current Features

- Binary time display in 24-hour format
- Binary date display without year
- Environment display:
  - temperature on the hour section
  - humidity as a 6-LED scale on the minute section
- Automatic display cycle:
  - time: 15 seconds
  - date: 5 seconds
  - environment: 5 seconds
- Manual mode switching with the MODE button
- Time and date setting through buttons
- Automatic brightness from LDR through 74HC595 OE PWM
- 3 alarm slots stored in DS1302 battery-backed RAM
- Active buzzer alarm signal with repeated 3-beep pattern
- Alarm can be stopped by pressing any button

## Display Modes

### Time Mode

The hour LEDs show current hours in binary.
The minute LEDs show current minutes in binary.
The time mode LED is on.

Time format is 24-hour:

```text
00:00 .. 23:59
```

### Date Mode

The hour section is reused for month.
The minute section is reused for day of month.
The date mode LED is on.

Year is not tracked in this phase.

### Environment Mode

The hour section shows temperature from DHT11.
The minute section shows humidity as a scale.
The environment mode LED is on.

Temperature is clamped for display to:

```text
0..60 C
```

Humidity scale:

```text
<30%  -> 0 LEDs
40%   -> 1 LED
50%   -> 2 LEDs
60%   -> 3 LEDs
70%   -> 4 LEDs
80%   -> 5 LEDs
>=90% -> 6 LEDs
```

### Alarm Mode

Alarm mode is the fourth manual mode. It does not participate in the automatic
time/date/environment cycle.

MODE cycles modes manually:

```text
Time -> Date -> Environment -> Alarm -> Time
```

In Alarm mode:

- alarm LED is on to show that alarm mode is open
- 3 mode LEDs are reused as alarm slot indicators
- hour/minute sections show the selected alarm time

Alarm slot LEDs:

- active slot: LED is on
- selected active slot: LED blinks
- selected inactive/configured slot: LED blinks
- inactive unselected slot: LED is off

If at least one alarm is active, the alarm LED is also on in normal modes.

## Button Mechanics

The project uses 5 buttons through one ADC resistor ladder:

- MODE
- SET
- UP
- DOWN
- OFF

Any button press pauses the automatic mode cycle for about 30 seconds.
After inactivity timeout, the display returns to Time mode.

### Setting Time

In Time mode:

```text
SET       -> edit minutes
SET again -> edit hours
SET again -> edit minutes
```

While editing:

- UP increments selected value
- DOWN decrements selected value
- selected section blinks
- changing minutes also resets seconds to 0
- changes are written to DS1302 immediately

OFF exits editing and reloads current RTC time.

### Setting Date

In Date mode:

```text
SET       -> edit day
SET again -> edit month
SET again -> edit day
```

While editing:

- UP increments selected value
- DOWN decrements selected value
- selected section blinks
- changes are written to DS1302 immediately

Month length is handled for 28/30/31-day months. Leap year is not tracked.

### Setting Alarms

There are 3 alarm slots stored in DS1302 RAM.
Configured alarm time is not deleted when the slot is disabled.

In Alarm mode:

```text
UP/DOWN -> select alarm slot 1..3
SET     -> edit alarm hours
SET     -> edit alarm minutes
SET     -> edit alarm hours
OFF     -> save selected alarm and activate it
```

When not editing:

```text
OFF on active slot -> disable selected alarm
long OFF press     -> disable all alarm slots
```

Long OFF press disables alarms but does not delete configured alarm times.

Alarm trigger rule:

```text
if enabled slot hour:minute == RTC hour:minute -> start buzzer
```

The same alarm will not retrigger repeatedly during the same minute.

## Buzzer Pattern

The active buzzer is switched through a transistor.
The firmware does not generate audio frequency PWM because TMB12A05 is an
active buzzer.

Alarm pattern:

```text
beep beep beep ... pause ... beep beep beep ... pause
```

The alarm stops:

- when any button is pressed
- automatically after about 60 seconds

## Brightness

LDR is read through ADC.
Brightness is applied globally to all LEDs through 74HC595 OE using TIM3_CH1.

Current brightness limits:

```text
min: 5%
max: 70%
levels: 6
```

This avoids constant flicker from small light changes.

## Build

Current build command:

```sh
cmake --build build/Debug
```

The default board package is:

```text
f407_disc1
```

To configure a separate build directory for a specific board:

```sh
cmake -B build/f407 -DCLOCK_BOARD=f407_disc1
cmake --build build/f407
```

The current default `build/Debug` directory was generated by the STM32 VS Code
tooling and also uses `CLOCK_BOARD=f407_disc1`.

## Current Architecture

The application is separated from the concrete board through `board.h`.

Current split:

```text
Src/main.c                         -> main clock state machine and input flow
Src/app/core/app_config.h          -> app timing/display/alarm constants
Src/app/core/app_types.h           -> app-level modes, edit targets, alarm slots
Src/app/alarm/alarm_manager.c/.h   -> alarm slots, DS1302 RAM storage, buzzer pattern
Src/app/display/display_renderer.c/.h -> converts app state into 74HC595 display bytes
Src/app/environment/environment_manager.c/.h -> cached DHT11 temperature/humidity reads
Src/app/ui/auto_mode_scheduler.c/.h -> timed time/date/environment rotation
Src/app/ui/ui_controller.c/.h      -> buttons, edit modes, manual mode switching
Src/board.h                        -> stable hardware abstraction API
Src/drivers/buzzer.c/.h            -> active buzzer driver
Src/drivers/buttons_decoder.c/.h   -> ADC button ladder driver
Src/drivers/ds1302.c/.h            -> DS1302 RTC driver
Src/drivers/light_sensor.c/.h      -> LDR brightness driver
Src/drivers/shift_register.c/.h    -> 74HC595 driver
Src/drivers/dht11.c/.h             -> DHT11 timing driver
Src/boards/f407_disc1/board.c      -> STM32F407G-DISC1 board implementation
Src/boards/f407_disc1/board_config.h -> STM32F407G-DISC1 pinout/calibration
Src/boards/f407_disc1/README.md    -> STM32F407G-DISC1 prototype pinout notes
Src/boards/f407_disc1/stm32f407_lowlevel.h -> STM32F407 bare-metal helpers
Src/boards/f407_disc1/board.cmake  -> CMake board package hook
```

`main.c` owns the top-level application loop: reading board inputs, asking app
modules to update, applying brightness, and writing the rendered display.

`Src/app` is grouped by application concern:

- `core`: shared app types and constants
- `ui`: button/edit/mode scheduling logic
- `alarm`: alarm state, persistence, and buzzer pattern
- `environment`: cached temperature/humidity sampling
- `display`: rendering app state into shift-register bytes

`ui_controller.c` owns button edges, long OFF handling, edit mode changes,
manual mode switching, and time/date writes to RTC.

`auto_mode_scheduler.c` owns the automatic time/date/environment cycle and the
temporary pause after user input.

`alarm_manager.c` owns the three alarm slots, selected alarm slot, alarm
enable/disable commands, DS1302 RAM persistence format, trigger detection, and
the active buzzer beep pattern.

`environment_manager.c` owns periodic environment reads and keeps the last valid
temperature/humidity sample for rendering.

`display_renderer.c` owns the LED mapping rules. It receives current app state
and returns a `ClockDisplay_t` with the two shift-register bytes.

Application modules should call `Board_*` functions instead of using GPIO, ADC,
TIM, DS1302, DHT11, or 74HC595 helpers directly.

Board packages live under:

```text
Src/boards/<board_name>/
```

CMake selects the package with:

```text
CLOCK_BOARD=<board_name>
```

The selected board package contributes:

- board-specific `board.c`
- board-specific include directory
- board-specific low-level/HAL files when needed
- pinout and calibration constants through `board_config.h`

## Adding A New Board

Use the existing board as a template:

```text
Src/boards/f407_disc1/
```

For a new board, create a new folder:

```text
Src/boards/<new_board_name>/
```

Each board folder should keep its own README with prototype wiring, module
power notes, reserved debug pins, and display mapping.

Recommended naming examples:

```text
f030_small
g030_hal
c031_final
```

### Required Files

Each board package should provide:

```text
Src/boards/<new_board_name>/board.c
Src/boards/<new_board_name>/board_config.h
Src/boards/<new_board_name>/board.cmake
Src/boards/<new_board_name>/README.md
```

If the board uses bare-metal register helpers, also add a low-level file:

```text
Src/boards/<new_board_name>/stm32xxxx_lowlevel.h
```

If the board uses HAL, add HAL/Cube generated files as needed inside the board
package or a clearly named support folder.

### board.c

`board.c` must implement the stable API from:

```text
Src/board.h
```

Required functions:

```c
void Board_Init(void);
ClockButton_t Board_ReadButton(void);
uint8_t Board_ReadBrightness(void);
void Board_SetBrightness(uint8_t brightness);
void Board_WriteDisplay(ClockDisplay_t display);
void Board_ReadDateTime(ClockDateTime_t *dateTime);
void Board_WriteTime(uint8_t hours, uint8_t minutes, uint8_t seconds);
void Board_WriteDate(uint8_t day, uint8_t month);
uint8_t Board_ReadEnvironment(ClockEnvironment_t *environment);
void Board_ReadAlarmStorage(uint8_t *data, uint8_t size);
void Board_WriteAlarmStorage(const uint8_t *data, uint8_t size);
void Board_SetBuzzer(uint8_t isEnabled);
void Board_DelayLoop(void);
```

The application should not care whether these functions use bare-metal
registers, HAL, LL, or another implementation.

### board_config.h

`board_config.h` contains the concrete wiring and calibration for one board.

For the current F407 board it includes:

```text
74HC595 pins
buttons ADC channel and thresholds
light sensor ADC channel and brightness calibration
DS1302 pins
DHT11 pin
buzzer pin
main loop delay constant
```

For a new board, keep the same logical constants where possible so existing
drivers can compile without changes. If a new board needs a different driver
implementation, provide it from that board package or split the driver behind a
cleaner interface.

### board.cmake

`board.cmake` is the CMake hook for one board. Minimal example:

```cmake
set(CLOCK_BOARD_NAME "My New Board")

list(APPEND sources_SRCS
    ${CMAKE_CURRENT_LIST_DIR}/board.c
)

list(APPEND include_DIRS
    ${CMAKE_CURRENT_LIST_DIR}
)
```

If the board has extra source files, add them there:

```cmake
list(APPEND sources_SRCS
    ${CMAKE_CURRENT_LIST_DIR}/board.c
    ${CMAKE_CURRENT_LIST_DIR}/my_hal_support.c
)
```

If it needs extra include paths:

```cmake
list(APPEND include_DIRS
    ${CMAKE_CURRENT_LIST_DIR}
    ${CMAKE_CURRENT_LIST_DIR}/Core/Inc
)
```

### Build Command

Configure and build with:

```sh
cmake -B build/<new_board_name> -DCLOCK_BOARD=<new_board_name>
cmake --build build/<new_board_name>
```

Example:

```sh
cmake -B build/f030 -DCLOCK_BOARD=f030_small
cmake --build build/f030
```

### Porting Checklist

When adding a board:

1. Copy `Src/boards/f407_disc1` to a new board folder.
2. Update `board.cmake` name and source list.
3. Update `board_config.h` pinout and ADC/PWM calibration.
4. Replace low-level helpers if the MCU family changes.
5. Confirm `PA13/PA14` or equivalent SWD pins are not used for app hardware.
6. Build with `-DCLOCK_BOARD=<new_board_name>`.
7. Document the board pinout in `Src/boards/<new_board_name>/README.md`.
8. Verify LEDs first with a static display.
9. Verify buttons ADC thresholds.
10. Verify brightness PWM/OE.
11. Verify RTC read/write and DS1302 RAM alarm storage.
12. Verify DHT11 timing on the target clock.
13. Verify buzzer output.

### Important Rule

Do not include board-specific headers from `main.c`.

Good:

```c
#include "board.h"
```

Avoid:

```c
#include "board_config.h"
#include "stm32f407_lowlevel.h"
```

Only the board package and low-level drivers should know about concrete pins and
MCU registers.

## Future Direction

The board package layer, basic drivers, and main app modules are now split out.
Next cleanup steps:

- make driver APIs less dependent on `board_config.h` where useful
- consider a small shared numeric helper module for wrap/min/max utilities
- add a second board package for the future smaller MCU
- add a HAL-based board package when the final MCU is selected
- keep `board.h` stable so the application logic survives board migration

This will make the migration to HAL and a smaller STM32 much safer.
