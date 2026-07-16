# STM32L010F4P6 Test Board

This board package is the first STM32L010F4P6 port based on the CubeMX
reference project in `l010f4p6/`.

The current display module uses two chained 74HC595 shift registers:

| MCU pin | Display signal | 74HC595 pin |
| --- | --- | --- |
| PA2 | SER / DS | 14 |
| PA3 | RCLK / ST_CP | 12 |
| PA4 | SRCLK / SH_CP | 11 |
| PA5 | OE / TIM2_CH1 PWM | 13 |

The button resistor ladder is connected to `PA1 / ADC_IN1`.

`PA0` is reserved for the STM32L0 `WKUP1` standby wake input and is not used as
an ADC channel. Main power sense is connected to `PB1 / ADC_IN9`. The future
LDR brightness input is connected to `PA6 / ADC_IN6`.

Brightness is fixed in firmware while the LDR is not connected. Time is read
from the internal RTC clocked by the external 32.768 kHz LSE crystal.
