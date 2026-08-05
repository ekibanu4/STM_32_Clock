# STM32L010F4P6 Clock Board

This board package contains the STM32L010F4P6 port of the binary clock.
The CubeMX project is `l010f4p6.ioc`.

## MCU Pinout

| MCU pin | Function | Notes |
| --- | --- | --- |
| PA0 | WKUP1 | Main-power wake input, `SYS_WKUP1` in CubeMX |
| PA1 | Buttons ADC | Five-button resistor ladder, `ADC_IN1` |
| PA2 | 74HC595 SER / DS | Shift-register serial data, pin 14 |
| PA3 | 74HC595 RCLK / ST_CP | Shift-register latch clock, pin 12 |
| PA4 | 74HC595 SRCLK / SH_CP | Shift-register shift clock, pin 11 |
| PA5 | 74HC595 OE / TIM2_CH1 | Active-low output enable PWM, pin 13 |
| PA6 | Buzzer drive | Active buzzer transistor control |
| PA7 | Motion sensor input | High means motion detected, low means idle; no internal pull |
| PA9 | I2C SCL | Software I2C clock for OLED, AHT10, and BH1750 |
| PA10 | I2C SDA | Software I2C data for OLED, AHT10, and BH1750 |
| PB1 | Battery sense ADC | BAT+ divider input, `ADC_IN9` |
| PC14 | LSE OSC32_IN | 32.768 kHz crystal |
| PC15 | LSE OSC32_OUT | 32.768 kHz crystal |
| PA13 | SWDIO | Debug/programming |
| PA14 | SWCLK | Debug/programming |

## 74HC595 Display

The current display module uses two chained 74HC595 shift registers:

| MCU pin | Display signal | 74HC595 pin |
| --- | --- | --- |
| PA2 | SER / DS | 14 |
| PA3 | RCLK / ST_CP | 12 |
| PA4 | SRCLK / SH_CP | 11 |
| PA5 | OE / TIM2_CH1 PWM | 13 |

OE is active-low. Firmware controls the display brightness automatically from
the BH1750 light sensor on the shared I2C bus.

### 74HC595 LED Map

The first register carries the six upper value LEDs plus the two high minute
bits. The second register carries the four low minute bits and mode/status
indicators.

| Firmware bit | Normal display meaning |
| --- | --- |
| `board1 bit0..bit5` | upper row value bits `32,16,8,4,2,1` |
| `board1 bit6..bit7` | lower row value bits `32,16` |
| `board2 bit0..bit3` | lower row value bits `8,4,2,1` |
| `board2 bit4` | time mode indicator |
| `board2 bit5` | date mode indicator |
| `board2 bit6` | environment mode indicator |
| `board2 bit7` | alarm indicator |

In time mode the upper row shows hours and the lower row shows minutes. In date
mode the upper row shows month and the lower row shows day. In environment mode
the upper row shows temperature as binary, while humidity is shown as a scale on
the lower row.

## Buttons ADC Ladder

The five buttons are read through one resistor ladder on `PA1 / ADC_IN1`.

Current thresholds:

| Button | Max ADC value |
| --- | ---: |
| MODE | 470 |
| SET | 950 |
| UP | 1630 |
| DOWN | 2330 |
| OFF | 3360 |

Values above `3360` are treated as no button.

## User Controls

Manual `MODE` presses cycle through:

```text
time -> date -> environment -> alarm -> battery -> time
```

Automatic display cycling only runs through:

```text
time -> date -> environment -> time
```

Current automatic durations are:

| Auto mode | Duration |
| --- | ---: |
| Time | 15 s |
| Date | 5 s |
| Environment | 5 s |

Button behavior by mode:

| Button | Time | Date | Environment | Alarm | Battery |
| --- | --- | --- | --- | --- | --- |
| MODE | next mode | next mode | next mode | next mode | next mode |
| SET | edit minutes/hours | edit day/month/year | no action | edit alarm hours/minutes | no action |
| UP | increment edited value | increment edited value | no action | when not editing: previous slot; when editing: increment value | no action |
| DOWN | decrement edited value | decrement edited value | no action | when not editing: next slot; when editing: decrement value | no action |
| OFF | exit editing | exit editing | no action | toggle/exit alarm editing | no action |

After user activity, automatic cycling is paused for 30 seconds. When this pause
expires, firmware returns to time mode.

### Time And Date Editing

In time setup, `SET` cycles `minutes -> hours -> minutes`. On the 74HC595
display only the time mode LED blinks; the edited row stays steadily lit.
Editing minutes lights only the lower row, editing hours lights only the upper
row.

In date setup, `SET` cycles `day -> month -> year -> day`. On the 74HC595
display only the date mode LED blinks. Editing day lights only the lower row,
editing month lights only the upper row, and editing year lights `20` on the
upper row plus the editable `00..99` year on the lower row.

## I2C OLED / AHT10 / BH1750 Bus

The experimental I2C branch uses a software I2C bus:

| MCU pin | I2C signal |
| --- | --- |
| PA9 | SCL / SCK |
| PA10 | SDA |

The 0.91" 128x32 OLED is probed at `0x3C`, then `0x3D`. The AHT10
temperature/humidity sensor uses address `0x38`. The BH1750 light sensor uses
address `0x23`. All devices share the same two lines. Both lines are configured
as open-drain GPIO with pull-ups enabled in firmware; external pull-ups are
still recommended for a stable bus.

The firmware initializes AHT10 with `0xE1 0x08 0x00`, triggers a measurement
with `0xAC 0x33 0x00`, waits about 80 ms, then reads 6 bytes and converts the
20-bit humidity and temperature values. Reads are throttled to no faster than
once every 2 seconds.

In environment display mode, the top/six-hour LEDs show temperature as a binary
number. The lower humidity LEDs are a scale, not a binary value:

| Humidity | Lit humidity LEDs |
| --- | ---: |
| `< 30%` | 0 |
| `40%` | 1 |
| `50%` | 2 |
| `60%` | 3 |
| `70%` | 4 |
| `80%` | 5 |
| `>= 90%` | 6 |

For example, top `011011` means `27 C`; three lit humidity LEDs means about
`60% RH`.

## BH1750 Light Sensor

The light sensor is now a BH1750 on the shared software I2C bus. Firmware puts
it into continuous high-resolution mode and reads lux about once per second,
then maps that value to 10 brightness levels with hysteresis. Brightness is
applied through `PA5 / TIM2_CH1` to the 74HC595 OE pin.

Current brightness calibration:

| Setting | Value |
| --- | ---: |
| Lux dark | 2 |
| Lux bright | 400 |
| Brightness min | 2% |
| Brightness max | 50% |
| Brightness levels | 10 |
| Lux hysteresis | 5 |

## OLED Display

The OLED mirrors the selected mode with text output:

| Mode | OLED content |
| --- | --- |
| Time | `HH:MM:SS`; a small `A` in the lower-right corner means at least one alarm is enabled |
| Date | `DD.MM.YY` |
| Environment | temperature and humidity from AHT10 |
| Alarm list | three rows, `*` marks the selected slot, `-` marks non-selected slots |
| Alarm edit | only the selected slot is shown large, with `..` over hours or minutes |
| Battery | `BAT xx.x%` plus a bottom charge bar |

Time/date setup on OLED uses two dots above the edited field. Alarm setup uses
two dots above the edited hour or minute field. When an alarm is ringing, OLED is
written once with `ALARM n` and normal redraw is skipped until the alarm stops.

## Active Buzzer

`PA6` drives the active buzzer circuit through a transistor. The firmware drives
`PA6` high while the buzzer should sound and low when it should be silent.

## Motion Sensor Idle Display

`PA7` is used as a digital motion sensor input. The sensor output should be
`0..3.3V`; high means motion is present. After one complete automatic display
cycle without motion, button activity, active setup, or an active alarm, firmware
clears the displays, turns the buzzer off, disables the 74HC595 OE PWM, and puts
external display/I2C control pins into analog mode. The firmware waits for the
actual automatic transitions `time -> date -> environment -> time`, so each
automatic screen is shown before idle display shutdown. With the current
`15s/5s/5s` auto-mode timing this is about 25 seconds. It does not enter standby
in this idle-display state.

The idle cycle is reset by:

- high level on `PA7`;
- any button press;
- active time/date/alarm editing;
- currently ringing alarm.

When main power is missing, the normal standby path runs first and the motion
sensor is ignored so it cannot block power-loss handling.

## Alarms

The firmware supports three alarm slots. Alarm settings are stored in RTC backup
registers, so configured slots survive main-power loss while the RTC domain is
kept alive by the supercapacitor.

Alarm mode behavior:

- `MODE` selects the alarm screen.
- entering alarm mode selects slot `A1`.
- `DOWN` selects the next slot: `A1 -> A2 -> A3 -> A1`.
- `UP` selects the previous slot: `A1 -> A3 -> A2 -> A1`.
- `SET` enters alarm hour/minute editing.
- `UP` / `DOWN` while editing increment/decrement the edited alarm value.
- `OFF` while editing exits to alarm-slot selection.
- `OFF` while not editing toggles the selected configured slot on/off.
- `OFF` on an empty slot blinks all hour/minute LEDs as an error.
- long `OFF` toggles all configured alarms: if any alarm is on it disables all;
  if all are off it enables all configured slots.

74HC595 alarm-mode display:

| LED group | Meaning |
| --- | --- |
| upper/lower value rows | selected alarm time |
| alarm LED, `board2 bit7` | alarm mode indicator; also lit in other modes if any alarm is enabled |
| status LED, `board2 bit4` | lit when selected slot is enabled |
| slot number LEDs, `board2 bit6/bit5` visually | selected slot as binary `01`, `10`, `11` |

While editing an alarm, the alarm LED stays steadily lit, the selected slot
number LEDs blink, and only the edited value row is lit: upper row for hours,
lower row for minutes.

OLED alarm-list display uses:

```text
*A1 17:20 ON
-A2 12:20 OFF
-A3 --:-- OFF
```

An empty slot is shown as `--:--`.

When an alarm is saved or re-enabled, firmware clears the "already triggered for
this minute" guard. This means a slot can ring again after being reconfigured,
even if that same slot already fired earlier today.

## RTC / LSE

Time is read from the internal RTC clocked by an external 32.768 kHz crystal on
`PC14/PC15`.

Date setup cycles through day, month, and year. The year is stored in the STM32
RTC as `00..99` with an implied `20xx` century. While editing the year, the
hour LEDs show the fixed `20` prefix and the minute LEDs show the editable
`00..99` year value; only the date mode indicator blinks. Leap-year day limits
are calculated for the `2000..2099` range.

RTC init keeps the LSE path conservative:

- if `LSERDY` is already set after standby wake, firmware does not re-run
  `HAL_RCC_OscConfig()` for LSE;
- if LSE/RTC init returns an error, firmware returns from RTC init instead of
  staying forever in `Error_Handler()`.

## Wake Power, Battery Sense, And Standby

`PA0 / WKUP1` is the hard power-loss and wake signal. If `PA0` is low, the
firmware powers external devices down, prepares `WKUP1`, and enters standby.

Battery level is measured separately on `PB1 / ADC_IN9`. The firmware converts
the ADC reading through `VREFINT`, then scales it through the measured battery
divider calibration. `PA0` and `PB1` intentionally have separate jobs:

- `PA0 / WKUP1` decides hard standby and wake after real power loss;
- `PB1 / ADC_IN9` decides low-battery display shutdown and battery percentage.

Current divider calibration:

```text
2140 mV on PB1 = 3650 mV on BAT+
```

Current low-battery thresholds:

```text
MAIN_POWER_BATTERY_MV_MIN = 2900 mV battery voltage
MAIN_POWER_BATTERY_MV_RECOVER = 3100 mV battery voltage
```

When the scaled battery voltage is below `2900 mV` for three consecutive checks
while `PA0` is still high, firmware enters low-battery mode instead of standby:
external devices are powered down and only the alarm LED blinks periodically.
Normal display operation resumes only after three consecutive checks at or above
`3100 mV`.

Before standby the firmware powers external devices down, then waits up to
`25 s` for `PA0 / WKUP1` to be low and debounces that low level for `300 ms`.

The battery display percentage uses a lookup curve from the same calibrated
battery voltage:

```text
2900 mV = 0%
3100 mV = 10%
3200 mV = 22%
3300 mV = 36%
3400 mV = 50%
3500 mV = 62%
3600 mV = 73%
3700 mV = 82%
3800 mV = 90%
4100 mV = 100%
```

Values between points are interpolated. The displayed percentage is filtered and
updated with hysteresis so it does not jump on every ADC sample.

The manual battery mode is reached through:

```text
time -> date -> environment -> alarm -> battery -> time
```

In battery mode the 74HC595 display uses only the six minute LEDs as a charge
bar. The OLED shows `BAT xx.x%` plus a bottom charge bar.

Expected power-loss flow:

```text
main power present  -> PA0 high, firmware runs
main power removed  -> PA0 low, VDD MCU held by supercapacitor
firmware enters standby
main power returns  -> PA0 rising edge wakes the MCU
```

`PA0` is not used as ADC or normal GPIO output. CubeMX assigns it as
`SYS_WKUP1`; firmware only enables the PWR wake source before entering standby.

## Flashing Notes

OpenOCD may fail to halt the target when the chip is in a low-power state. In
that case, use STM32CubeProgrammer CLI with connect-under-reset, for example:

```text
STM32_Programmer_CLI -c port=SWD mode=UR freq=50 -w build/Debug/l010f4p6.elf -v -rst
```

The VS Code tasks still include OpenOCD flash tasks for normal cases.
