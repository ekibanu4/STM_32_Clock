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
| PA7 | Motion sensor input | High means motion detected, low means idle |
| PA9 | I2C SCL | Software I2C clock for OLED and AHT10 |
| PA10 | I2C SDA | Software I2C data for OLED and AHT10 |
| PB1 | Main power sense ADC | Main VDD sense, `ADC_IN9` |
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

## I2C OLED / AHT10 Bus

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
then maps that value to six brightness steps with hysteresis. Brightness is
applied through `PA5 / TIM2_CH1` to the 74HC595 OE pin.

Current brightness calibration:

| Setting | Value |
| --- | ---: |
| Lux dark | 5 |
| Lux bright | 400 |
| Brightness min | 2% |
| Brightness max | 50% |
| Brightness levels | 10 |
| Lux hysteresis | 5 |

## Active Buzzer

`PA6` drives the active buzzer circuit through a transistor. The firmware drives
`PA6` high while the buzzer should sound and low when it should be silent.

## Motion Sensor Idle Display

`PA7` is used as a digital motion sensor input. The sensor output should be
`0..3.3V`; high means motion is present. After 15 seconds without motion,
button activity, active setup, or an active alarm, firmware clears the displays,
turns the buzzer off, disables the 74HC595 OE PWM, and puts external display/I2C
control pins into analog mode. It does not enter standby in this idle-display
state.

The idle timer is reset by:

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
- `UP` / `DOWN` select the alarm slot when not editing.
- `SET` enters alarm hour/minute editing.
- `OFF` while editing saves the selected slot and enables it.
- `OFF` while not editing toggles the selected configured slot on/off.
- `OFF` on an empty slot blinks all hour/minute LEDs as an error.
- long `OFF` toggles all configured alarms: if any alarm is on it disables all;
  if all are off it enables all configured slots.

When an alarm is saved or re-enabled, firmware clears the "already triggered for
this minute" guard. This means a slot can ring again after being reconfigured,
even if that same slot already fired earlier today.

## RTC / LSE

Time is read from the internal RTC clocked by an external 32.768 kHz crystal on
`PC14/PC15`.

Date setup cycles through day, month, and year. The year is stored in the STM32
RTC as `00..99` with an implied `20xx` century. While editing the year, the
hour LEDs show the fixed `20` prefix and the minute LEDs show the editable
`00..99` year value; the first alarm indicator blinks as the year-edit marker.
Leap-year day limits are calculated for the `2000..2099` range.

RTC init keeps the LSE path conservative:

- if `LSERDY` is already set after standby wake, firmware does not re-run
  `HAL_RCC_OscConfig()` for LSE;
- if LSE/RTC init returns an error, firmware returns from RTC init instead of
  staying forever in `Error_Handler()`.

## Wake Power, Battery Sense, And Standby

`PA0 / WKUP1` is the hard power-loss and wake signal. If `PA0` is low, the
firmware powers external devices down, prepares `WKUP1`, and enters standby.

Battery level is measured on `PB1 / ADC_IN9`. The firmware converts the
ADC reading through `VREFINT`, then scales it through the measured battery
divider calibration.

Current divider calibration:

```text
1880 mV on PB1 = 3650 mV on BAT+
```

Current threshold:

```text
MAIN_POWER_BATTERY_MV_MIN = 2900 mV battery voltage
MAIN_POWER_BATTERY_MV_MAX = 4200 mV battery voltage
```

When the scaled battery voltage is below the threshold for three consecutive
checks while `PA0` is still high, firmware enters low-battery mode instead of
standby: external devices are powered down and only the alarm LED blinks
periodically.

Before standby the firmware powers external devices down, then waits up to
`25 s` for `PA0 / WKUP1` to be low and debounces that low level for `300 ms`.

The battery display percentage uses the same calibrated battery voltage:

```text
2900 mV = 0%
4200 mV = 100%
```

The manual mode button cycle is:

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
STM32_Programmer_CLI -c port=SWD mode=UR freq=100 -w build/Debug/l010f4p6.elf -v -rst
```

The VS Code tasks still include OpenOCD flash tasks for normal cases.
