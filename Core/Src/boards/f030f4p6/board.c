#include "board.h"

#include "board_config.h"

extern ADC_HandleTypeDef hadc;

volatile uint16_t buttonsAdcRaw = 0U;
volatile ClockButton_t buttonsDecoded = CLOCK_BUTTON_NONE;
volatile uint16_t lightSensorAdcRaw = 0U;

#define DS1302_SECONDS_READ 0x81U
#define DS1302_SECONDS_WRITE 0x80U
#define DS1302_MINUTES_READ 0x83U
#define DS1302_MINUTES_WRITE 0x82U
#define DS1302_HOURS_READ 0x85U
#define DS1302_HOURS_WRITE 0x84U
#define DS1302_DATE_READ 0x87U
#define DS1302_DATE_WRITE 0x86U
#define DS1302_MONTH_READ 0x89U
#define DS1302_MONTH_WRITE 0x88U
#define DS1302_CONTROL_WRITE 0x8EU
#define DS1302_RAM_WRITE_BASE 0xC0U
#define DS1302_RAM_READ_BASE 0xC1U
#define DS1302_RAM_SIZE 31U

#define DHT11_TIMEOUT 0xFFFFFFFFUL
#define DHT11_MIN_READ_INTERVAL_MS 2000UL
#define DHT11_START_LOW_MS 25U
#define DHT11_START_RELEASE_US 30U
#define DHT11_RESPONSE_TIMEOUT_US 120U
#define DHT11_BIT_TIMEOUT_US 100U
#define DHT11_ONE_THRESHOLD_US 45U

#define PWM_MAX_DUTY 100U

static ClockEnvironment_t lastEnvironment = {0U, 0U, 0U};
static uint32_t lastEnvironmentReadMs = 0U;

static void Board_DelayCycles(volatile uint32_t cycles) {
  while (cycles > 0U) {
    --cycles;
  }
}

static void Board_DelayUs(uint32_t microseconds) {
  uint32_t cyclesPerUs = SystemCoreClock / 1000000UL;
  uint32_t waitTicks = microseconds * cyclesPerUs;
  uint32_t reload = SysTick->LOAD;
  uint32_t start = SysTick->VAL;
  uint32_t elapsed = 0U;

  if ((cyclesPerUs == 0U) || ((SysTick->CTRL & SysTick_CTRL_ENABLE_Msk) == 0U)) {
    while (microseconds > 0U) {
      Board_DelayCycles(8U);
      --microseconds;
    }
    return;
  }

  while (elapsed < waitTicks) {
    uint32_t current = SysTick->VAL;

    if (start >= current) {
      elapsed = start - current;
    } else {
      elapsed = start + (reload - current) + 1U;
    }
  }
}

static void ShiftRegister_Pulse(GPIO_TypeDef *GPIOx, uint16_t pin) {
  HAL_GPIO_WritePin(GPIOx, pin, GPIO_PIN_SET);
  HAL_GPIO_WritePin(GPIOx, pin, GPIO_PIN_RESET);
}

static void ShiftRegister_OutputEnablePwmInit(uint8_t brightness) {
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  if (brightness > PWM_MAX_DUTY) {
    brightness = PWM_MAX_DUTY;
  }

  __HAL_RCC_TIM3_CLK_ENABLE();

  GPIO_InitStruct.Pin = SHIFT_REGISTER_OUTPUT_ENABLE_PIN;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  GPIO_InitStruct.Alternate = GPIO_AF1_TIM3;
  HAL_GPIO_Init(SHIFT_REGISTER_OUTPUT_ENABLE_GPIO, &GPIO_InitStruct);

  TIM3->CR1 = 0U;
  TIM3->PSC = 7U;
  TIM3->ARR = PWM_MAX_DUTY - 1U;
  TIM3->CCR1 = brightness;
  TIM3->CCMR1 &= ~(0xFFUL);
  TIM3->CCMR1 |= (6UL << 4) | (1UL << 3);
  TIM3->CCER &= ~(0xFUL);
  TIM3->CCER |= (1UL << 1) | (1UL << 0);
  TIM3->EGR = TIM_EGR_UG;
  TIM3->CR1 |= TIM_CR1_ARPE | TIM_CR1_CEN;
}

static uint8_t BcdToDecimal(uint8_t bcdValue) {
  return (uint8_t)(((bcdValue >> 4U) * 10U) + (bcdValue & 0x0FU));
}

static uint8_t DecimalToBcd(uint8_t decimalValue) {
  return (uint8_t)(((decimalValue / 10U) << 4U) | (decimalValue % 10U));
}

static uint16_t ADC_ReadChannel(uint32_t channel) {
  ADC_ChannelConfTypeDef sConfig = {0};
  uint16_t value = 0x0FFFU;

  HAL_ADC_Stop(&hadc);
  ADC1->CHSELR = 0U;

  sConfig.Channel = channel;
  sConfig.Rank = ADC_RANK_CHANNEL_NUMBER;
  sConfig.SamplingTime = ADC_SAMPLETIME_239CYCLES_5;
  if (HAL_ADC_ConfigChannel(&hadc, &sConfig) != HAL_OK) {
    return value;
  }

  for (uint8_t sample = 0U; sample < 2U; ++sample) {
    if (HAL_ADC_Start(&hadc) != HAL_OK) {
      break;
    }
    if (HAL_ADC_PollForConversion(&hadc, 5U) == HAL_OK) {
      value = (uint16_t)(HAL_ADC_GetValue(&hadc) & 0x0FFFU);
    }
    HAL_ADC_Stop(&hadc);
  }
  return value;
}

static ClockButton_t DecodeButton(uint16_t adcValue) {
  if (adcValue <= BUTTON_MODE_ADC_MAX) {
    return CLOCK_BUTTON_MODE;
  }
  if (adcValue <= BUTTON_SET_ADC_MAX) {
    return CLOCK_BUTTON_SET;
  }
  if (adcValue <= BUTTON_UP_ADC_MAX) {
    return CLOCK_BUTTON_UP;
  }
  if (adcValue <= BUTTON_DOWN_ADC_MAX) {
    return CLOCK_BUTTON_DOWN;
  }
  if (adcValue <= BUTTON_OFF_ADC_MAX) {
    return CLOCK_BUTTON_OFF;
  }
  return CLOCK_BUTTON_NONE;
}

static void DHT11_PinInput(void) {
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  GPIO_InitStruct.Pin = DHT11_DATA_PIN;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(DHT11_DATA_GPIO, &GPIO_InitStruct);
}

static void DHT11_PinOutput(void) {
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  GPIO_InitStruct.Pin = DHT11_DATA_PIN;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(DHT11_DATA_GPIO, &GPIO_InitStruct);
}

static GPIO_PinState DHT11_ReadPinFast(void) {
  return ((DHT11_DATA_GPIO->IDR & DHT11_DATA_PIN) != 0U) ? GPIO_PIN_SET
                                                          : GPIO_PIN_RESET;
}

static uint32_t SysTickElapsedTicks(uint32_t start, uint32_t current) {
  uint32_t reload = SysTick->LOAD;

  if (start >= current) {
    return start - current;
  }

  return start + (reload - current) + 1U;
}

static uint32_t DHT11_MeasurePulseUs(GPIO_PinState level, uint32_t timeoutUs) {
  uint32_t cyclesPerUs = SystemCoreClock / 1000000UL;
  uint32_t timeoutTicks = timeoutUs * cyclesPerUs;
  uint32_t start = SysTick->VAL;
  uint32_t elapsedTicks = 0U;

  if ((cyclesPerUs == 0U) || ((SysTick->CTRL & SysTick_CTRL_ENABLE_Msk) == 0U)) {
    return DHT11_TIMEOUT;
  }

  while (DHT11_ReadPinFast() == level) {
    elapsedTicks = SysTickElapsedTicks(start, SysTick->VAL);
    if (elapsedTicks >= timeoutTicks) {
      return DHT11_TIMEOUT;
    }
  }

  return elapsedTicks / cyclesPerUs;
}

static void DS1302_DataOutput(void) {
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  GPIO_InitStruct.Pin = DS1302_DATA_PIN;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(DS1302_DATA_GPIO, &GPIO_InitStruct);
}

static void DS1302_DataInput(void) {
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  GPIO_InitStruct.Pin = DS1302_DATA_PIN;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(DS1302_DATA_GPIO, &GPIO_InitStruct);
}

static void DS1302_WriteByte(uint8_t value) {
  DS1302_DataOutput();

  for (uint8_t bit = 0U; bit < 8U; ++bit) {
    HAL_GPIO_WritePin(DS1302_DATA_GPIO, DS1302_DATA_PIN,
                      ((value >> bit) & 0x1U) ? GPIO_PIN_SET
                                               : GPIO_PIN_RESET);
    HAL_GPIO_WritePin(DS1302_CLK_GPIO, DS1302_CLK_PIN, GPIO_PIN_SET);
    Board_DelayCycles(20U);
    HAL_GPIO_WritePin(DS1302_CLK_GPIO, DS1302_CLK_PIN, GPIO_PIN_RESET);
    Board_DelayCycles(20U);
  }
}

static uint8_t DS1302_ReadByte(void) {
  uint8_t value = 0U;

  DS1302_DataInput();

  for (uint8_t bit = 0U; bit < 8U; ++bit) {
    if (HAL_GPIO_ReadPin(DS1302_DATA_GPIO, DS1302_DATA_PIN) == GPIO_PIN_SET) {
      value |= (uint8_t)(1U << bit);
    }
    HAL_GPIO_WritePin(DS1302_CLK_GPIO, DS1302_CLK_PIN, GPIO_PIN_SET);
    Board_DelayCycles(20U);
    HAL_GPIO_WritePin(DS1302_CLK_GPIO, DS1302_CLK_PIN, GPIO_PIN_RESET);
    Board_DelayCycles(20U);
  }

  return value;
}

static uint8_t DS1302_ReadRegister(uint8_t command) {
  uint8_t value;

  HAL_GPIO_WritePin(DS1302_CLK_GPIO, DS1302_CLK_PIN, GPIO_PIN_RESET);
  HAL_GPIO_WritePin(DS1302_RST_GPIO, DS1302_RST_PIN, GPIO_PIN_SET);
  Board_DelayCycles(20U);

  DS1302_WriteByte(command);
  value = DS1302_ReadByte();

  HAL_GPIO_WritePin(DS1302_RST_GPIO, DS1302_RST_PIN, GPIO_PIN_RESET);
  HAL_GPIO_WritePin(DS1302_CLK_GPIO, DS1302_CLK_PIN, GPIO_PIN_RESET);
  DS1302_DataOutput();

  return value;
}

static void DS1302_WriteRegister(uint8_t command, uint8_t value) {
  HAL_GPIO_WritePin(DS1302_CLK_GPIO, DS1302_CLK_PIN, GPIO_PIN_RESET);
  HAL_GPIO_WritePin(DS1302_RST_GPIO, DS1302_RST_PIN, GPIO_PIN_SET);
  Board_DelayCycles(20U);

  DS1302_WriteByte(command);
  DS1302_WriteByte(value);

  HAL_GPIO_WritePin(DS1302_RST_GPIO, DS1302_RST_PIN, GPIO_PIN_RESET);
  HAL_GPIO_WritePin(DS1302_CLK_GPIO, DS1302_CLK_PIN, GPIO_PIN_RESET);
  DS1302_DataOutput();
}

static void DS1302_SetWriteProtect(uint8_t isEnabled) {
  DS1302_WriteRegister(DS1302_CONTROL_WRITE, isEnabled ? 0x80U : 0x00U);
}

static void DS1302_StartClock(void) {
  uint8_t seconds = DS1302_ReadRegister(DS1302_SECONDS_READ);

  DS1302_SetWriteProtect(0U);
  DS1302_WriteRegister(DS1302_SECONDS_WRITE, (uint8_t)(seconds & 0x7FU));
}

static uint8_t DS1302_RamCommand(uint8_t address, uint8_t isRead) {
  address %= DS1302_RAM_SIZE;
  return (uint8_t)(DS1302_RAM_WRITE_BASE + (address * 2U) +
                   (isRead ? 1U : 0U));
}

static uint8_t DS1302_ReadRamByte(uint8_t address) {
  return DS1302_ReadRegister(DS1302_RamCommand(address, 1U));
}

static void DS1302_WriteRamByte(uint8_t address, uint8_t value) {
  DS1302_SetWriteProtect(0U);
  DS1302_WriteRegister(DS1302_RamCommand(address, 0U), value);
}

void Board_Init(void) {
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  (void)HAL_ADCEx_Calibration_Start(&hadc);

  GPIO_InitStruct.Pin = SHIFT_REGISTER_DATA_PIN | SHIFT_REGISTER_LATCH_PIN |
                        SHIFT_REGISTER_CLOCK_PIN;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  HAL_GPIO_WritePin(SHIFT_REGISTER_DATA_GPIO, SHIFT_REGISTER_DATA_PIN,
                    GPIO_PIN_RESET);
  HAL_GPIO_WritePin(SHIFT_REGISTER_CLOCK_GPIO, SHIFT_REGISTER_CLOCK_PIN,
                    GPIO_PIN_RESET);
  HAL_GPIO_WritePin(SHIFT_REGISTER_LATCH_GPIO, SHIFT_REGISTER_LATCH_PIN,
                    GPIO_PIN_RESET);
  ShiftRegister_OutputEnablePwmInit(LIGHT_SENSOR_BRIGHTNESS_MAX);

  HAL_GPIO_WritePin(DS1302_CLK_GPIO, DS1302_CLK_PIN, GPIO_PIN_RESET);
  HAL_GPIO_WritePin(DS1302_RST_GPIO, DS1302_RST_PIN, GPIO_PIN_RESET);
  HAL_GPIO_WritePin(DS1302_DATA_GPIO, DS1302_DATA_PIN, GPIO_PIN_RESET);
  DS1302_DataOutput();
  HAL_GPIO_WritePin(DHT11_DATA_GPIO, DHT11_DATA_PIN, GPIO_PIN_SET);
  DHT11_PinInput();

  HAL_GPIO_WritePin(BUZZER_GPIO, BUZZER_PIN, GPIO_PIN_RESET);
  Board_WriteDisplay((ClockDisplay_t){0U, 0U});
  DS1302_StartClock();
}

ClockButton_t Board_ReadButton(void) {
  buttonsAdcRaw = ADC_ReadChannel(BUTTONS_ADC_CHANNEL);
  buttonsDecoded = DecodeButton(buttonsAdcRaw);
  return buttonsDecoded;
}

uint8_t Board_ReadBrightness(void) {
  static uint8_t currentLevel = 0U;
  uint16_t adcValue = ADC_ReadChannel(LIGHT_SENSOR_ADC_CHANNEL);
  uint32_t adcRange = LIGHT_SENSOR_ADC_BRIGHT - LIGHT_SENSOR_ADC_DARK;
  uint8_t maxLevel = LIGHT_SENSOR_BRIGHTNESS_LEVELS - 1U;
  uint8_t nextLevel = currentLevel;

  lightSensorAdcRaw = adcValue;

  if (adcValue <= LIGHT_SENSOR_ADC_DARK) {
    nextLevel = 0U;
  } else if (adcValue >= LIGHT_SENSOR_ADC_BRIGHT) {
    nextLevel = maxLevel;
  } else {
    uint32_t adcOffset = adcValue - LIGHT_SENSOR_ADC_DARK;
    nextLevel =
        (uint8_t)((adcOffset * maxLevel + (adcRange / 2U)) / adcRange);
  }

  if (nextLevel > currentLevel) {
    uint32_t upThreshold =
        LIGHT_SENSOR_ADC_DARK +
        ((uint32_t)(currentLevel + 1U) * adcRange) / maxLevel;
    if (adcValue >= (upThreshold + LIGHT_SENSOR_ADC_HYSTERESIS)) {
      currentLevel = nextLevel;
    }
  } else if (nextLevel < currentLevel) {
    uint32_t downThreshold =
        LIGHT_SENSOR_ADC_DARK + ((uint32_t)currentLevel * adcRange) / maxLevel;
    if ((adcValue + LIGHT_SENSOR_ADC_HYSTERESIS) <= downThreshold) {
      currentLevel = nextLevel;
    }
  }

  uint32_t brightnessRange =
      LIGHT_SENSOR_BRIGHTNESS_MAX - LIGHT_SENSOR_BRIGHTNESS_MIN;

  return (uint8_t)(LIGHT_SENSOR_BRIGHTNESS_MIN +
                   (((uint32_t)currentLevel * brightnessRange) / maxLevel));
}

void Board_SetBrightness(uint8_t brightness) {
  if (brightness > PWM_MAX_DUTY) {
    brightness = PWM_MAX_DUTY;
  }
  TIM3->CCR1 = brightness;
}

void Board_WriteDisplay(ClockDisplay_t display) {
  HAL_GPIO_WritePin(SHIFT_REGISTER_LATCH_GPIO, SHIFT_REGISTER_LATCH_PIN,
                    GPIO_PIN_RESET);

  /* First shifted byte ends up in the second 74HC595 through QH'. */
  for (int8_t bit = 7; bit >= 0; --bit) {
    HAL_GPIO_WritePin(SHIFT_REGISTER_DATA_GPIO, SHIFT_REGISTER_DATA_PIN,
                      ((display.board2 >> bit) & 0x1U) ? GPIO_PIN_SET
                                                        : GPIO_PIN_RESET);
    ShiftRegister_Pulse(SHIFT_REGISTER_CLOCK_GPIO, SHIFT_REGISTER_CLOCK_PIN);
  }

  /* Last shifted byte remains in the first 74HC595 connected to MCU DATA. */
  for (int8_t bit = 7; bit >= 0; --bit) {
    HAL_GPIO_WritePin(SHIFT_REGISTER_DATA_GPIO, SHIFT_REGISTER_DATA_PIN,
                      ((display.board1 >> bit) & 0x1U) ? GPIO_PIN_SET
                                                        : GPIO_PIN_RESET);
    ShiftRegister_Pulse(SHIFT_REGISTER_CLOCK_GPIO, SHIFT_REGISTER_CLOCK_PIN);
  }

  ShiftRegister_Pulse(SHIFT_REGISTER_LATCH_GPIO, SHIFT_REGISTER_LATCH_PIN);
}

void Board_ReadDateTime(ClockDateTime_t *dateTime) {
  dateTime->seconds =
      BcdToDecimal((uint8_t)(DS1302_ReadRegister(DS1302_SECONDS_READ) & 0x7FU));
  dateTime->minutes =
      BcdToDecimal((uint8_t)(DS1302_ReadRegister(DS1302_MINUTES_READ) & 0x7FU));
  dateTime->hours =
      BcdToDecimal((uint8_t)(DS1302_ReadRegister(DS1302_HOURS_READ) & 0x3FU));
  dateTime->day =
      BcdToDecimal((uint8_t)(DS1302_ReadRegister(DS1302_DATE_READ) & 0x3FU));
  dateTime->month =
      BcdToDecimal((uint8_t)(DS1302_ReadRegister(DS1302_MONTH_READ) & 0x1FU));
  dateTime->year = 0U;
}

void Board_WriteTime(uint8_t hours, uint8_t minutes, uint8_t seconds) {
  DS1302_SetWriteProtect(0U);
  DS1302_WriteRegister(DS1302_SECONDS_WRITE, DecimalToBcd(seconds));
  DS1302_WriteRegister(DS1302_MINUTES_WRITE, DecimalToBcd(minutes));
  DS1302_WriteRegister(DS1302_HOURS_WRITE,
                       DecimalToBcd((uint8_t)(hours & 0x3FU)));
}

void Board_WriteDate(uint8_t day, uint8_t month, uint8_t year) {
  (void)year;
  DS1302_SetWriteProtect(0U);
  DS1302_WriteRegister(DS1302_DATE_WRITE, DecimalToBcd(day));
  DS1302_WriteRegister(DS1302_MONTH_WRITE, DecimalToBcd(month));
}

uint8_t Board_ReadEnvironment(ClockEnvironment_t *environment) {
  uint8_t data[5] = {0U, 0U, 0U, 0U, 0U};
  uint32_t nowMs = HAL_GetTick();

  if (((nowMs - lastEnvironmentReadMs) < DHT11_MIN_READ_INTERVAL_MS) &&
      (lastEnvironment.isValid != 0U)) {
    *environment = lastEnvironment;
    environment->isValid = 0U;
    return 1U;
  }
  lastEnvironmentReadMs = nowMs;

  DHT11_PinOutput();
  HAL_GPIO_WritePin(DHT11_DATA_GPIO, DHT11_DATA_PIN, GPIO_PIN_SET);
  Board_DelayUs(DHT11_START_RELEASE_US);
  HAL_GPIO_WritePin(DHT11_DATA_GPIO, DHT11_DATA_PIN, GPIO_PIN_RESET);
  HAL_Delay(DHT11_START_LOW_MS);
  HAL_GPIO_WritePin(DHT11_DATA_GPIO, DHT11_DATA_PIN, GPIO_PIN_SET);
  Board_DelayUs(DHT11_START_RELEASE_US);

  DHT11_PinInput();
  __disable_irq();

  if (DHT11_ReadPinFast() == GPIO_PIN_SET) {
    if (DHT11_MeasurePulseUs(GPIO_PIN_SET, DHT11_RESPONSE_TIMEOUT_US) ==
        DHT11_TIMEOUT) {
      __enable_irq();
      environment->isValid = 0U;
      return 0U;
    }
  }

  if (DHT11_MeasurePulseUs(GPIO_PIN_RESET, DHT11_RESPONSE_TIMEOUT_US) ==
      DHT11_TIMEOUT) {
    __enable_irq();
    environment->isValid = 0U;
    return 0U;
  }

  if (DHT11_MeasurePulseUs(GPIO_PIN_SET, DHT11_RESPONSE_TIMEOUT_US) ==
      DHT11_TIMEOUT) {
    __enable_irq();
    environment->isValid = 0U;
    return 0U;
  }

  for (uint8_t bitIndex = 0U; bitIndex < 40U; ++bitIndex) {
    uint32_t highUs;

    if (DHT11_MeasurePulseUs(GPIO_PIN_RESET, DHT11_BIT_TIMEOUT_US) ==
        DHT11_TIMEOUT) {
      __enable_irq();
      environment->isValid = 0U;
      return 0U;
    }

    highUs = DHT11_MeasurePulseUs(GPIO_PIN_SET, DHT11_BIT_TIMEOUT_US);
    if (highUs == DHT11_TIMEOUT) {
      __enable_irq();
      environment->isValid = 0U;
      return 0U;
    }

    data[bitIndex / 8U] <<= 1U;
    if (highUs > DHT11_ONE_THRESHOLD_US) {
      data[bitIndex / 8U] |= 1U;
    }
  }

  __enable_irq();

  if (data[4] != (uint8_t)(data[0] + data[1] + data[2] + data[3])) {
    environment->isValid = 0U;
    return 0U;
  }

  environment->humidity = data[0];
  environment->temperature = data[2];
  environment->isValid = 1U;
  lastEnvironment = *environment;
  return 1U;
}

void Board_ReadAlarmStorage(uint8_t *data, uint8_t size) {
  for (uint8_t index = 0U; index < size; ++index) {
    data[index] = DS1302_ReadRamByte(index);
  }
}

void Board_WriteAlarmStorage(const uint8_t *data, uint8_t size) {
  for (uint8_t index = 0U; index < size; ++index) {
    DS1302_WriteRamByte(index, data[index]);
  }
}

void Board_SetBuzzer(uint8_t isEnabled) {
  HAL_GPIO_WritePin(BUZZER_GPIO, BUZZER_PIN,
                    (isEnabled != 0U) ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

void Board_DelayLoop(void) {
  Board_DelayCycles(20000U);
}
