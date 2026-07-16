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
| PA6 | Light ADC | Future LDR input, `ADC_IN6` |
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

OE is active-low. Firmware currently starts with fixed brightness `20%` while
the LDR input is not used.

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

## RTC / LSE

Time is read from the internal RTC clocked by an external 32.768 kHz crystal on
`PC14/PC15`.

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
