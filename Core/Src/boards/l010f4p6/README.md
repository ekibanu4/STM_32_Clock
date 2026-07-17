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
| PA6 | Light ADC | LDR/light sensor input, `ADC_IN6` |
| PA7 | DHT11 DATA | Temperature/humidity single-wire data |
| PA9 | Buzzer control | Active buzzer transistor drive |
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
the light sensor on `PA6 / ADC_IN6`.

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

## DHT11 Environment Sensor

The DHT11 data line is connected to `PA7`. CubeMX does not need a fixed pin
mode for this line because firmware switches it between open-drain output for
the start pulse and input for the sensor response.

Use a pull-up on DATA to 3.3V. A ready-made DHT11 module may already include
this pull-up; a bare sensor usually needs an external resistor around
`4.7k-10k`.

The firmware waits a short startup delay before the first read and then polls
the DHT11 no faster than once every 2 seconds. The bit decoder follows the
older working driver style: it measures each low/high pulse pair and treats the
bit as `1` when the high pulse is longer than the low pulse. This is more
stable on the low-clocked L010 than sampling at a fixed microsecond offset.

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

## Light Sensor

The light sensor is connected to `PA6 / ADC_IN6`. The firmware reads it through
the shared ADC and maps the raw value to six brightness steps with hysteresis,
then applies that brightness through `PA5 / TIM2_CH1` to the 74HC595 OE pin.

Current brightness calibration:

| Setting | Value |
| --- | ---: |
| ADC dark | 1000 |
| ADC bright | 3200 |
| Brightness min | 2% |
| Brightness max | 25% |
| Brightness levels | 6 |
| ADC hysteresis | 80 |

## Active Buzzer

`PA9` drives the active buzzer through a transistor. The buzzer input is active
high from firmware: `PA9 = 1` turns the buzzer on, `PA9 = 0` turns it off.

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

## Main Power Sense And Standby

Main-power presence is measured on `PB1 / ADC_IN9`. The firmware converts the
ADC reading through `VREFINT`, so the threshold is in millivolts instead of raw
ADC counts.

Current threshold:

```text
MAIN_POWER_SENSE_MV_MIN = 2300 mV
```

The main loop enters standby when `PB1` is below the threshold for three
consecutive checks. Before standby the firmware waits up to `100 ms` for
`PA0 / WKUP1` to be low.

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
