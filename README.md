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

The project currently builds as a bare-metal STM32F407 project.
Future phases are expected to migrate the code to HAL and a smaller STM32.

## Future Direction

The next major phase should split the project into modules and create a board
abstraction layer before migrating to a smaller MCU:

- board pin/config layer
- 74HC595 driver
- DS1302 driver
- DHT11 driver
- buttons decoder
- brightness controller
- display renderer
- alarm manager

This will make the migration to HAL and a smaller STM32 much safer.
