# STM32F407G-DISC1 Pinout - Binary Clock Phase 1

This file documents the current working wiring for the STM32F407G-DISC1
prototype.

## MCU Pins In Use

| MCU pin | Function | Notes |
| --- | --- | --- |
| PA0 | LDR ADC input | Light sensor / auto brightness |
| PA1 | Buttons ADC input | 5-button resistor ladder |
| PA2 | 74HC595 DS / SER | Shift register serial data |
| PA3 | 74HC595 STCP / RCLK | Shift register latch |
| PA4 | 74HC595 SHCP / SRCLK | Shift register clock |
| PA5 | DS1302 CLK | RTC clock |
| PA6 | 74HC595 OE | TIM3_CH1 PWM, active-low output enable |
| PA7 | DS1302 DATA / I/O | Bidirectional RTC data |
| PA8 | DS1302 RST / CE | RTC chip enable |
| PA10 | DHT11 DATA / S | HW-481 DHT11 data line |
| PA15 | Buzzer control | Drives NPN transistor base through resistor |

Total currently used MCU GPIO pins: 11.

Do not use:

| MCU pin | Reason |
| --- | --- |
| PA13 | SWDIO debug/programming |
| PA14 | SWCLK debug/programming |

## 74HC595N Chain

Two 74HC595N chips are chained.

STM32 to first 74HC595N:

| STM32 | 74HC595N pin | Function |
| --- | --- | --- |
| PA2 | pin 14 | SER / DS |
| PA4 | pin 11 | SHCP / SRCLK |
| PA3 | pin 12 | STCP / RCLK |
| PA6 | pin 13 | OE |
| 3.3V | pin 10 | MR / SRCLR |
| 3.3V | pin 16 | VCC |
| GND | pin 8 | GND |

Chain:

```text
first 74HC595 QH' -> second 74HC595 DS
```

OE is controlled by PWM from `PA6 / TIM3_CH1`.
OE is active-low, so the firmware uses inverted logic for global LED brightness.

## LED Output Mapping

### Board 1 / First 74HC595N

| 74HC595 output | LED meaning |
| --- | --- |
| QA | hours 32 |
| QB | hours 16 |
| QC | hours 8 |
| QD | hours 4 |
| QE | hours 2 |
| QF | hours 1 |
| QG | minutes 32 |
| QH | minutes 16 |

### Board 2 / Second 74HC595N

| 74HC595 output | LED meaning |
| --- | --- |
| QA | minutes 8 |
| QB | minutes 4 |
| QC | minutes 2 |
| QD | minutes 1 |
| QE | mode time / alarm slot 1 |
| QF | mode date / alarm slot 2 |
| QG | mode environment / alarm slot 3 |
| QH | alarm status |

## Buttons ADC Ladder

Button ladder is connected to `PA1`.

Current wiring:

```text
3.3V
 |
[10k]
 |
 +-------> PA1 / ADC_BUTTONS
 |
 +--[ MODE]--[1k]----GND
 |
 +--[ SET ]--[2k]----GND
 |
 +--[ UP  ]--[4.7k]--GND
 |
 +--[ DOWN]--[10k]---GND
 |
 +--[ OFF ]--[20k]---GND
```

Current ADC thresholds:

| Button | Max ADC value |
| --- | ---: |
| MODE | 520 |
| SET | 990 |
| UP | 1670 |
| DOWN | 2380 |
| OFF | 3410 |

Values above `3410` are treated as no button.

## Light Sensor

LDR voltage divider is connected to `PA0`.

Current wiring:

```text
3.3V
 |
[LDR]
 |
 +-------> PA0 / ADC_LIGHT
 |
[10k]
 |
GND
```

Current calibration:

| Constant | Value |
| --- | ---: |
| ADC dark | 1000 |
| ADC bright | 3200 |
| Brightness min | 5% |
| Brightness max | 70% |
| Brightness levels | 6 |
| ADC hysteresis | 80 |

## DS1302Z RTC

| STM32 | DS1302Z | Notes |
| --- | --- | --- |
| PA5 | CLK | Clock |
| PA7 | DATA / I/O | Bidirectional data |
| PA8 | RST / CE | Chip enable |
| 3.3V | VCC | Main power |
| GND | GND | Common ground |

The DS1302 backup battery keeps RTC time and RAM alive.

DS1302 RAM usage:

```text
byte 0  -> signature
byte 1  -> version
byte 2  -> alarm slot 1 flags
byte 3  -> alarm slot 1 hour
byte 4  -> alarm slot 1 minute
byte 5  -> alarm slot 2 flags
byte 6  -> alarm slot 2 hour
byte 7  -> alarm slot 2 minute
byte 8  -> alarm slot 3 flags
byte 9  -> alarm slot 3 hour
byte 10 -> alarm slot 3 minute
byte 11 -> checksum
```

Only 12 of 31 RAM bytes are currently used.

Alarm flags:

```text
bit 0 -> configured
bit 1 -> enabled
```

## DHT11 / HW-481

Current connection:

| STM32 | HW-481 / DHT11 | Notes |
| --- | --- | --- |
| PA10 | DATA / S | Single-wire DHT protocol |
| VCC | VCC / + | Use voltage that works for the module |
| GND | GND / - | Common ground |

Notes from prototype debugging:

- The DHT11 module wiring must be checked carefully. Wrong pin order caused
  several false software failures during development.
- Some DHT11 modules are unreliable below their specified supply voltage.
- If the module data output is pulled to 5V, do not connect it directly to a
  non-5V-tolerant MCU pin on the final design.

## Buzzer

Current buzzer:

```text
TMB12A05 active buzzer
```

Current wiring:

```text
PA15 -> 1k..4.7k -> NPN base
NPN emitter -> GND
NPN collector -> buzzer -
3.3V -> 270 ohm -> buzzer +
```

The 270 ohm series resistor reduces the buzzer volume to an acceptable level.

Recommended optional protection for magnetic buzzer:

```text
diode parallel to buzzer
diode cathode -> buzzer +
diode anode   -> buzzer -
```

The buzzer is active, so the firmware only switches it on/off.
No audio-frequency PWM is required.

## Power Notes

- MCU logic is 3.3V.
- 74HC595N is powered from 3.3V in the current prototype.
- DS1302Z is powered from 3.3V in the current prototype.
- DHT11 module voltage depends on the specific HW-481 board revision.
- Buzzer is tested as acceptable at 3.3V with 270 ohm series resistor.
- All external modules must share common ground with STM32.

## Reserved / Avoided Pins

Avoid using these in the prototype and future board unless there is a strong
reason:

| Pin | Reason |
| --- | --- |
| PA13 | SWDIO |
| PA14 | SWCLK |

`PA11` was avoided in this prototype because it was not available on the
current board setup. `PB0` was tested and later freed.
