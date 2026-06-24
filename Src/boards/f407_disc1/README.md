# STM32F407G-DISC1 Board Package

This board package contains the current prototype wiring for the binary clock on
STM32F407G-DISC1.

## MCU Pinout

| MCU pin | Function | Notes |
| --- | --- | --- |
| PA0 | Light sensor ADC | LDR divider input |
| PA1 | Buttons ADC | Five-button resistor ladder |
| PA2 | 74HC595 SER / DS | Shift-register serial data, pin 14 |
| PA3 | 74HC595 RCLK / ST_CP | Shift-register latch, pin 12 |
| PA4 | 74HC595 SRCLK / SH_CP | Shift-register clock, pin 11 |
| PA5 | DS1302 CLK | RTC clock |
| PA6 | 74HC595 OE / TIM3_CH1 | Output-enable PWM dimming, pin 13 |
| PA7 | DS1302 DATA / I/O | RTC bidirectional data |
| PA8 | DS1302 RST / CE | RTC chip enable |
| PA10 | DHT11 DATA / S | HW-481 DHT11 module data |
| PA15 | Buzzer drive | NPN base through 1k..4.7k |

PA13 and PA14 are kept free for SWD debugging.

## Buttons ADC Ladder

```text
3.3V
 |
[10k]
 |
 +-------> PA1 / ADC_BUTTONS
 |
 +--[ MODE]--[1k]--GND
 |
 +--[ SET ]--[2k]--GND
 |
 +--[ UP  ]--[4.7k]--GND
 |
 +--[ DOWN]--[10k]--GND
 |
 +--[ OFF ]--[20k]--GND
```

ADC thresholds live in `board_config.h`.

## Light Sensor

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

The light sensor controls display brightness from 5% to 70%. The 74HC595 outputs
are dimmed together through the OE pin.

## 74HC595 Chain

First 74HC595N wiring:

| 74HC595 pin | Signal | MCU / rail |
| --- | --- | --- |
| 14 DS / SER | Serial data | PA2 |
| 11 SH_CP / SRCLK | Shift clock | PA4 |
| 12 ST_CP / RCLK | Latch clock | PA3 |
| 13 OE | Output enable PWM | PA6 |
| 10 MR / SRCLR | Master reset | 3.3V |
| 16 VCC | Power | 3.3V |
| 8 GND | Ground | GND |

Second 74HC595N is chained from the first register QH' to the second register DS.

### Display Mapping

Board 1 outputs:

| Output | Display |
| --- | --- |
| QA | hours 32 |
| QB | hours 16 |
| QC | hours 8 |
| QD | hours 4 |
| QE | hours 2 |
| QF | hours 1 |
| QG | minutes 32 |
| QH | minutes 16 |

Board 2 outputs:

| Output | Display |
| --- | --- |
| QA | minutes 8 |
| QB | minutes 4 |
| QC | minutes 2 |
| QD | minutes 1 |
| QE | mode time |
| QF | mode date |
| QG | mode environment |
| QH | alarm set |

## DS1302Z RTC

| DS1302 pin | MCU / rail |
| --- | --- |
| CLK | PA5 |
| DATA / I/O | PA7 |
| RST / CE | PA8 |
| VCC | 3.3V |
| GND | GND |

The module has quartz and backup battery assembled. DS1302 RAM stores alarm
slots.

## HW-481 / DHT11

| Module pin | MCU / rail |
| --- | --- |
| DATA / S | PA10 |
| VCC / + | 5V on the current prototype |
| GND / - | GND |

The DHT11 module was unreliable at the lower prototype voltage, so the current
test wiring uses 5V module power while the data line goes to PA10.

## Active Buzzer

Current prototype buzzer: TMB12A05 active buzzer.

```text
PA15 -> NPN base through 1k..4.7k
3.3V -> 270R -> buzzer +
buzzer - -> NPN collector
NPN emitter -> GND
```

The 270R resistor was chosen experimentally to reduce volume.
