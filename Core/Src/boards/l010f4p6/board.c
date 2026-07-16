#include "board.h"

#include "board_config.h"
#include "stm32l0xx_ll_adc.h"

#define ALARM_STORAGE_SIZE 16U
#define ALARM_STORAGE_BACKUP_REGISTER_COUNT 4U
#define BRIGHTNESS_MAX 100U
#define DISPLAY_TEST_BRIGHTNESS 20U
#define ADC_FULL_SCALE 4095U
#define MAIN_POWER_CONFIRM_SAMPLES 3U
#define MAIN_POWER_WAKEUP_LOW_WAIT_MS 100U
#define DHT11_TIMEOUT 0xFFFFFFFFUL
#define DHT11_MIN_READ_INTERVAL_MS 2000UL
#define DHT11_START_LOW_MS 20U
#define DHT11_PULL_TIME_US 55U
#define DHT11_MAX_PULSE_LOOPS 10000U

extern ADC_HandleTypeDef hadc;
extern RTC_HandleTypeDef hrtc;

static uint8_t currentBrightness = DISPLAY_TEST_BRIGHTNESS;
static ClockEnvironment_t lastEnvironment = {0U, 0U, 0U};
static uint32_t lastEnvironmentReadMs = 0U;

static volatile uint32_t *AlarmStorage_BackupRegister(uint8_t registerIndex) {
  switch (registerIndex) {
  case 0U:
    return &RTC->BKP0R;
  case 1U:
    return &RTC->BKP1R;
  case 2U:
    return &RTC->BKP2R;
  case 3U:
    return &RTC->BKP3R;
  default:
    return &RTC->BKP4R;
  }
}

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

  if ((cyclesPerUs == 0U) ||
      ((SysTick->CTRL & SysTick_CTRL_ENABLE_Msk) == 0U)) {
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

  if (brightness > BRIGHTNESS_MAX) {
    brightness = BRIGHTNESS_MAX;
  }

  __HAL_RCC_TIM2_CLK_ENABLE();

  GPIO_InitStruct.Pin = SHIFT_REGISTER_OUTPUT_ENABLE_PIN;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  GPIO_InitStruct.Alternate = GPIO_AF5_TIM2;
  HAL_GPIO_Init(SHIFT_REGISTER_OUTPUT_ENABLE_GPIO, &GPIO_InitStruct);

  TIM2->CR1 = 0U;
  TIM2->PSC = 7U;
  TIM2->ARR = BRIGHTNESS_MAX - 1U;
  TIM2->CCR1 = brightness;
  TIM2->CCMR1 &= ~(0xFFUL);
  TIM2->CCMR1 |= (6UL << 4) | (1UL << 3);
  TIM2->CCER &= ~(0xFUL);
  TIM2->CCER |= TIM_CCER_CC1P | TIM_CCER_CC1E;
  TIM2->EGR = TIM_EGR_UG;
  TIM2->CR1 |= TIM_CR1_ARPE | TIM_CR1_CEN;
}

static uint16_t ADC_ReadChannel(uint32_t channel) {
  ADC_ChannelConfTypeDef sConfig = {0};
  uint16_t value = 0x0FFFU;

  HAL_ADC_Stop(&hadc);
  ADC1->CHSELR = 0U;

  sConfig.Channel = channel;
  sConfig.Rank = ADC_RANK_CHANNEL_NUMBER;
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

static uint16_t MainPower_ReadSenseMv(void) {
  uint16_t senseRaw;
  uint16_t vrefRaw;
  uint32_t vddaMv;

  (void)HAL_ADCEx_EnableVREFINT();
  senseRaw = ADC_ReadChannel(MAIN_POWER_SENSE_ADC_CHANNEL);
  vrefRaw = ADC_ReadChannel(ADC_CHANNEL_VREFINT);
  HAL_ADCEx_DisableVREFINT();

  if (vrefRaw == 0U) {
    return 0U;
  }

  vddaMv = __LL_ADC_CALC_VREFANALOG_VOLTAGE(vrefRaw, ADC_RESOLUTION_12B);
  return (uint16_t)(((uint32_t)senseRaw * vddaMv) / ADC_FULL_SCALE);
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

static uint32_t DHT11_CountPulse(GPIO_PinState level) {
  uint32_t count = 0U;

  while (DHT11_ReadPinFast() == level) {
    ++count;
    if (count >= DHT11_MAX_PULSE_LOOPS) {
      return DHT11_TIMEOUT;
    }
  }

  return count;
}

static uint8_t DHT11_Fail(ClockEnvironment_t *environment) {
  environment->temperature = 0U;
  environment->humidity = 0U;
  environment->isValid = 0U;
  lastEnvironment = *environment;
  return 1U;
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

void Board_Init(void) {
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  (void)HAL_ADCEx_Calibration_Start(&hadc, ADC_SINGLE_ENDED);

  __HAL_RCC_GPIOA_CLK_ENABLE();

  GPIO_InitStruct.Pin = SHIFT_REGISTER_DATA_PIN | SHIFT_REGISTER_LATCH_PIN |
                        SHIFT_REGISTER_CLOCK_PIN;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  HAL_GPIO_WritePin(BUZZER_GPIO, BUZZER_PIN, GPIO_PIN_RESET);
  GPIO_InitStruct.Pin = BUZZER_PIN;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(BUZZER_GPIO, &GPIO_InitStruct);

  HAL_GPIO_WritePin(SHIFT_REGISTER_DATA_GPIO, SHIFT_REGISTER_DATA_PIN,
                    GPIO_PIN_RESET);
  HAL_GPIO_WritePin(SHIFT_REGISTER_CLOCK_GPIO, SHIFT_REGISTER_CLOCK_PIN,
                    GPIO_PIN_RESET);
  HAL_GPIO_WritePin(SHIFT_REGISTER_LATCH_GPIO, SHIFT_REGISTER_LATCH_PIN,
                    GPIO_PIN_RESET);

  HAL_GPIO_WritePin(DHT11_DATA_GPIO, DHT11_DATA_PIN, GPIO_PIN_SET);
  DHT11_PinInput();

  ShiftRegister_OutputEnablePwmInit(DISPLAY_TEST_BRIGHTNESS);
  Board_WriteDisplay((ClockDisplay_t){0U, 0U});
}

uint8_t Board_IsMainPowerPresent(void) {
  for (uint8_t sample = 0U; sample < MAIN_POWER_CONFIRM_SAMPLES; ++sample) {
    if (MainPower_ReadSenseMv() >= MAIN_POWER_SENSE_MV_MIN) {
      return 1U;
    }
  }
  return 0U;
}

void Board_EnterStandby(void) {
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  uint32_t waitStartMs = HAL_GetTick();

  while (HAL_GPIO_ReadPin(MAIN_POWER_WAKEUP_GPIO,
                          MAIN_POWER_WAKEUP_GPIO_PIN) == GPIO_PIN_SET) {
    if ((HAL_GetTick() - waitStartMs) >= MAIN_POWER_WAKEUP_LOW_WAIT_MS) {
      return;
    }
  }

  Board_WriteDisplay((ClockDisplay_t){0U, 0U});
  TIM2->CCR1 = 0U;
  TIM2->CCER = 0U;
  TIM2->CR1 = 0U;
  HAL_GPIO_WritePin(SHIFT_REGISTER_OUTPUT_ENABLE_GPIO,
                    SHIFT_REGISTER_OUTPUT_ENABLE_PIN, GPIO_PIN_SET);

  HAL_ADC_Stop(&hadc);
  HAL_ADC_DeInit(&hadc);
  HAL_ADCEx_DisableVREFINT();
  HAL_PWREx_DisableFastWakeUp();
  HAL_PWREx_EnableUltraLowPower();
  DBGMCU->CR &= ~DBGMCU_CR_DBG;

  GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
  GPIO_InitStruct.Pull = GPIO_NOPULL;

  GPIO_InitStruct.Pin = GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3 | GPIO_PIN_4 |
                        GPIO_PIN_5 | GPIO_PIN_6 | GPIO_PIN_7 | GPIO_PIN_8 |
                        GPIO_PIN_9 | GPIO_PIN_10 | GPIO_PIN_11 | GPIO_PIN_12 |
                        GPIO_PIN_15;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  GPIO_InitStruct.Pin = GPIO_PIN_1;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  HAL_PWR_DisableWakeUpPin(MAIN_POWER_WAKEUP_PIN);
  __HAL_PWR_CLEAR_FLAG(PWR_FLAG_WU);
  while (__HAL_PWR_GET_FLAG(PWR_FLAG_WU) != RESET) {
    __HAL_PWR_CLEAR_FLAG(PWR_FLAG_WU);
  }
  HAL_PWR_EnableWakeUpPin(MAIN_POWER_WAKEUP_PIN);
  __DSB();
  __ISB();
  HAL_PWR_EnterSTANDBYMode();

  while (1) {
  }
}

ClockButton_t Board_ReadButton(void) {
  return DecodeButton(ADC_ReadChannel(BUTTONS_ADC_CHANNEL));
}

uint8_t Board_ReadBrightness(void) {
  return currentBrightness;
}

void Board_SetBrightness(uint8_t brightness) {
  if (brightness > BRIGHTNESS_MAX) {
    brightness = BRIGHTNESS_MAX;
  }
  currentBrightness = brightness;
  TIM2->CCR1 = brightness;
}

void Board_WriteDisplay(ClockDisplay_t display) {
  HAL_GPIO_WritePin(SHIFT_REGISTER_LATCH_GPIO, SHIFT_REGISTER_LATCH_PIN,
                    GPIO_PIN_RESET);

  for (int8_t bit = 7; bit >= 0; --bit) {
    HAL_GPIO_WritePin(SHIFT_REGISTER_DATA_GPIO, SHIFT_REGISTER_DATA_PIN,
                      ((display.board2 >> bit) & 0x1U) ? GPIO_PIN_SET
                                                        : GPIO_PIN_RESET);
    ShiftRegister_Pulse(SHIFT_REGISTER_CLOCK_GPIO, SHIFT_REGISTER_CLOCK_PIN);
  }

  for (int8_t bit = 7; bit >= 0; --bit) {
    HAL_GPIO_WritePin(SHIFT_REGISTER_DATA_GPIO, SHIFT_REGISTER_DATA_PIN,
                      ((display.board1 >> bit) & 0x1U) ? GPIO_PIN_SET
                                                        : GPIO_PIN_RESET);
    ShiftRegister_Pulse(SHIFT_REGISTER_CLOCK_GPIO, SHIFT_REGISTER_CLOCK_PIN);
  }

  ShiftRegister_Pulse(SHIFT_REGISTER_LATCH_GPIO, SHIFT_REGISTER_LATCH_PIN);
}

void Board_ReadDateTime(ClockDateTime_t *dateTime) {
  RTC_TimeTypeDef time = {0};
  RTC_DateTypeDef date = {0};

  if (HAL_RTC_GetTime(&hrtc, &time, RTC_FORMAT_BIN) != HAL_OK) {
    return;
  }
  if (HAL_RTC_GetDate(&hrtc, &date, RTC_FORMAT_BIN) != HAL_OK) {
    return;
  }

  dateTime->seconds = time.Seconds;
  dateTime->minutes = time.Minutes;
  dateTime->hours = time.Hours;
  dateTime->day = date.Date;
  dateTime->month = date.Month;
}

void Board_WriteTime(uint8_t hours, uint8_t minutes, uint8_t seconds) {
  RTC_TimeTypeDef time = {0};

  time.Hours = (uint8_t)(hours % 24U);
  time.Minutes = (uint8_t)(minutes % 60U);
  time.Seconds = (uint8_t)(seconds % 60U);
  time.DayLightSaving = RTC_DAYLIGHTSAVING_NONE;
  time.StoreOperation = RTC_STOREOPERATION_RESET;
  (void)HAL_RTC_SetTime(&hrtc, &time, RTC_FORMAT_BIN);
}

void Board_WriteDate(uint8_t day, uint8_t month) {
  RTC_DateTypeDef date = {0};

  date.WeekDay = RTC_WEEKDAY_MONDAY;
  date.Month = month;
  date.Date = day;
  date.Year = 24U;
  (void)HAL_RTC_SetDate(&hrtc, &date, RTC_FORMAT_BIN);
}

uint8_t Board_ReadEnvironment(ClockEnvironment_t *environment) {
  uint8_t data[5] = {0U, 0U, 0U, 0U, 0U};
  uint32_t nowMs = HAL_GetTick();
  uint32_t lowCount;
  uint32_t highCount;

  if (((nowMs - lastEnvironmentReadMs) < DHT11_MIN_READ_INTERVAL_MS) &&
      (lastEnvironment.isValid != 0U)) {
    *environment = lastEnvironment;
    return 1U;
  }
  lastEnvironmentReadMs = nowMs;

  DHT11_PinInput();
  HAL_Delay(1U);

  HAL_GPIO_WritePin(DHT11_DATA_GPIO, DHT11_DATA_PIN, GPIO_PIN_RESET);
  DHT11_PinOutput();
  HAL_Delay(DHT11_START_LOW_MS);
  DHT11_PinInput();
  Board_DelayUs(DHT11_PULL_TIME_US);

  __disable_irq();

  if (DHT11_ReadPinFast() == GPIO_PIN_SET) {
    __enable_irq();
    return DHT11_Fail(environment);
  }

  if (DHT11_CountPulse(GPIO_PIN_RESET) == DHT11_TIMEOUT) {
    __enable_irq();
    return DHT11_Fail(environment);
  }

  if (DHT11_CountPulse(GPIO_PIN_SET) == DHT11_TIMEOUT) {
    __enable_irq();
    return DHT11_Fail(environment);
  }

  for (uint8_t bitIndex = 0U; bitIndex < 40U; ++bitIndex) {
    lowCount = DHT11_CountPulse(GPIO_PIN_RESET);
    if (lowCount == DHT11_TIMEOUT) {
      __enable_irq();
      return DHT11_Fail(environment);
    }

    highCount = DHT11_CountPulse(GPIO_PIN_SET);
    if (highCount == DHT11_TIMEOUT) {
      __enable_irq();
      return DHT11_Fail(environment);
    }

    data[bitIndex / 8U] <<= 1U;
    if (highCount > lowCount) {
      data[bitIndex / 8U] |= 1U;
    }
  }

  __enable_irq();

  if (data[4] != (uint8_t)(data[0] + data[1] + data[2] + data[3])) {
    return DHT11_Fail(environment);
  }

  environment->humidity = data[0];
  environment->temperature = data[2];
  environment->isValid = 1U;
  lastEnvironment = *environment;
  return 1U;
}

void Board_ReadAlarmStorage(uint8_t *data, uint8_t size) {
  uint8_t dataIndex = 0U;

  for (uint8_t index = 0U; index < size; ++index) {
    data[index] = 0U;
  }

  if (size > ALARM_STORAGE_SIZE) {
    size = ALARM_STORAGE_SIZE;
  }

  HAL_PWR_EnableBkUpAccess();

  for (uint8_t registerIndex = 0U;
       (registerIndex < ALARM_STORAGE_BACKUP_REGISTER_COUNT) &&
       (dataIndex < size);
       ++registerIndex) {
    uint32_t registerValue = *AlarmStorage_BackupRegister(registerIndex);

    for (uint8_t byteIndex = 0U; (byteIndex < 4U) && (dataIndex < size);
         ++byteIndex) {
      data[dataIndex] = (uint8_t)((registerValue >> (8U * byteIndex)) & 0xFFU);
      ++dataIndex;
    }
  }
}

void Board_WriteAlarmStorage(const uint8_t *data, uint8_t size) {
  uint8_t dataIndex = 0U;

  if (size > ALARM_STORAGE_SIZE) {
    size = ALARM_STORAGE_SIZE;
  }

  HAL_PWR_EnableBkUpAccess();
  __HAL_RTC_WRITEPROTECTION_DISABLE(&hrtc);

  for (uint8_t registerIndex = 0U;
       registerIndex < ALARM_STORAGE_BACKUP_REGISTER_COUNT; ++registerIndex) {
    uint32_t registerValue = 0U;

    for (uint8_t byteIndex = 0U; byteIndex < 4U; ++byteIndex) {
      if (dataIndex < size) {
        registerValue |= ((uint32_t)data[dataIndex]) << (8U * byteIndex);
      }
      ++dataIndex;
    }

    *AlarmStorage_BackupRegister(registerIndex) = registerValue;
  }

  __HAL_RTC_WRITEPROTECTION_ENABLE(&hrtc);
}

void Board_SetBuzzer(uint8_t isEnabled) {
  HAL_GPIO_WritePin(BUZZER_GPIO, BUZZER_PIN,
                    (isEnabled != 0U) ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

void Board_DelayLoop(void) {
  HAL_Delay(1U);
}
