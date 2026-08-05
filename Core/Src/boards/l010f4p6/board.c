#include "board.h"

#include "board_config.h"
#include "oled_128x32.h"
#include "stm32l0xx_ll_adc.h"

#define ALARM_STORAGE_SIZE 16U
#define ALARM_STORAGE_BACKUP_REGISTER_COUNT 4U
#define BRIGHTNESS_MAX 100U
#define DISPLAY_TEST_BRIGHTNESS 20U
#define ADC_FULL_SCALE 4095U
#define MAIN_POWER_CONFIRM_SAMPLES 3U
#define MAIN_POWER_WAKEUP_LOW_WAIT_MS 25000U
#define MAIN_POWER_WAKEUP_LOW_DEBOUNCE_MS 300U
#define BATTERY_PERCENT_FILTER_SHIFT 4U
#define BATTERY_PERCENT_DISPLAY_HYSTERESIS_TENTHS 20U
#define BATTERY_PERCENT_UPDATE_INTERVAL_MS 2000U
#define BATTERY_PERCENT_UNINITIALIZED 0xFFFFU
#define AHT10_ADDRESS 0x38U
#define AHT10_MIN_READ_INTERVAL_MS 15000UL
#define AHT10_INIT_DELAY_MS 40U
#define AHT10_MEASURE_DELAY_MS 80U
#define AHT10_STATUS_BUSY 0x80U
#define BH1750_ADDRESS 0x23U
#define BH1750_POWER_ON 0x01U
#define BH1750_RESET 0x07U
#define BH1750_CONT_HIGH_RES_MODE 0x10U
#define BH1750_READ_INTERVAL_MS 5000U
#define BOARD_I2C_DELAY_CYCLES 8U

extern ADC_HandleTypeDef hadc;
extern RTC_HandleTypeDef hrtc;

static uint8_t currentBrightness = DISPLAY_TEST_BRIGHTNESS;
static ClockEnvironment_t lastEnvironment = {0U, 0U, 0U};
static uint32_t lastEnvironmentReadMs = 0U;
static uint8_t aht10Initialized = 0U;
static uint8_t aht10MeasurePending = 0U;
static uint32_t aht10MeasureStartMs = 0U;
static uint32_t lastBrightnessReadMs = 0U;
static uint8_t bh1750Initialized = 0U;

static void Board_PowerDownExternalDevices(uint16_t gpioAPins, uint8_t stopAdc);

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

static uint16_t MainPower_ReadBatteryMv(void) {
  uint32_t senseMv = MainPower_ReadSenseMv();

  if (MAIN_POWER_SENSE_ADC_CAL_MV == 0U) {
    return 0U;
  }

  return (uint16_t)((senseMv * MAIN_POWER_SENSE_BATTERY_CAL_MV) /
                    MAIN_POWER_SENSE_ADC_CAL_MV);
}

static uint16_t BatteryPercentTenthsFromMv(uint16_t batteryMv) {
  static const uint16_t mv[] = {4100U, 3800U, 3700U, 3600U, 3500U, 3400U,
                                3300U, 3200U, 3100U, 2900U};
  static const uint16_t pct[] = {1000U, 900U, 820U, 730U, 620U, 500U,
                                 360U, 220U, 100U, 0U};

  if (batteryMv >= mv[0]) {
    return pct[0];
  }

  for (uint8_t index = 0U; index < ((sizeof(mv) / sizeof(mv[0])) - 1U);
       ++index) {
    if (batteryMv >= mv[index + 1U]) {
      uint16_t highMv = mv[index];
      uint16_t lowMv = mv[index + 1U];
      uint16_t highPct = pct[index];
      uint16_t lowPct = pct[index + 1U];
      return (uint16_t)(lowPct +
                        (((uint32_t)batteryMv - lowMv) *
                         (highPct - lowPct)) /
                            (highMv - lowMv));
    }
  }

  return pct[(sizeof(pct) / sizeof(pct[0])) - 1U];
}

static void BoardI2c_Delay(void) {
  for (volatile uint8_t index = 0U; index < BOARD_I2C_DELAY_CYCLES; ++index) {
  }
}

static void BoardI2c_SclHigh(void) {
  HAL_GPIO_WritePin(BOARD_I2C_SCL_GPIO, BOARD_I2C_SCL_PIN, GPIO_PIN_SET);
  BoardI2c_Delay();
}

static void BoardI2c_SclLow(void) {
  HAL_GPIO_WritePin(BOARD_I2C_SCL_GPIO, BOARD_I2C_SCL_PIN, GPIO_PIN_RESET);
  BoardI2c_Delay();
}

static void BoardI2c_SdaHigh(void) {
  HAL_GPIO_WritePin(BOARD_I2C_SDA_GPIO, BOARD_I2C_SDA_PIN, GPIO_PIN_SET);
  BoardI2c_Delay();
}

static void BoardI2c_SdaLow(void) {
  HAL_GPIO_WritePin(BOARD_I2C_SDA_GPIO, BOARD_I2C_SDA_PIN, GPIO_PIN_RESET);
  BoardI2c_Delay();
}

static GPIO_PinState BoardI2c_SdaRead(void) {
  return HAL_GPIO_ReadPin(BOARD_I2C_SDA_GPIO, BOARD_I2C_SDA_PIN);
}

static void BoardI2c_Start(void) {
  BoardI2c_SdaHigh();
  BoardI2c_SclHigh();
  BoardI2c_SdaLow();
  BoardI2c_SclLow();
}

static void BoardI2c_Stop(void) {
  BoardI2c_SdaLow();
  BoardI2c_SclHigh();
  BoardI2c_SdaHigh();
}

static uint8_t BoardI2c_WriteByte(uint8_t value) {
  for (uint8_t bit = 0U; bit < 8U; ++bit) {
    if ((value & 0x80U) != 0U) {
      BoardI2c_SdaHigh();
    } else {
      BoardI2c_SdaLow();
    }
    BoardI2c_SclHigh();
    BoardI2c_SclLow();
    value <<= 1;
  }

  BoardI2c_SdaHigh();
  BoardI2c_SclHigh();
  uint8_t ack = (BoardI2c_SdaRead() == GPIO_PIN_RESET) ? 1U : 0U;
  BoardI2c_SclLow();
  return ack;
}

static uint8_t BoardI2c_ReadByte(uint8_t ack) {
  uint8_t value = 0U;

  BoardI2c_SdaHigh();
  for (uint8_t bit = 0U; bit < 8U; ++bit) {
    value <<= 1U;
    BoardI2c_SclHigh();
    if (BoardI2c_SdaRead() == GPIO_PIN_SET) {
      value |= 1U;
    }
    BoardI2c_SclLow();
  }

  if (ack != 0U) {
    BoardI2c_SdaLow();
  } else {
    BoardI2c_SdaHigh();
  }
  BoardI2c_SclHigh();
  BoardI2c_SclLow();
  BoardI2c_SdaHigh();
  return value;
}

static uint8_t BoardI2c_Write(uint8_t address, const uint8_t *data,
                              uint8_t size) {
  BoardI2c_Start();
  if (BoardI2c_WriteByte((uint8_t)(address << 1U)) == 0U) {
    BoardI2c_Stop();
    return 0U;
  }
  for (uint8_t index = 0U; index < size; ++index) {
    if (BoardI2c_WriteByte(data[index]) == 0U) {
      BoardI2c_Stop();
      return 0U;
    }
  }
  BoardI2c_Stop();
  return 1U;
}

static uint8_t BoardI2c_Read(uint8_t address, uint8_t *data, uint8_t size) {
  BoardI2c_Start();
  if (BoardI2c_WriteByte((uint8_t)((address << 1U) | 1U)) == 0U) {
    BoardI2c_Stop();
    return 0U;
  }
  for (uint8_t index = 0U; index < size; ++index) {
    data[index] = BoardI2c_ReadByte((index + 1U) < size);
  }
  BoardI2c_Stop();
  return 1U;
}

static uint8_t AHT10_WriteCommand(const uint8_t *data, uint8_t size) {
  return BoardI2c_Write(AHT10_ADDRESS, data, size);
}

static uint8_t AHT10_ReadData(uint8_t *data, uint8_t size) {
  return BoardI2c_Read(AHT10_ADDRESS, data, size);
}

static void BoardI2c_GpioInit(void) {
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  __HAL_RCC_GPIOA_CLK_ENABLE();

  GPIO_InitStruct.Pin = BOARD_I2C_SCL_PIN | BOARD_I2C_SDA_PIN;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  HAL_GPIO_WritePin(GPIOA, BOARD_I2C_SCL_PIN | BOARD_I2C_SDA_PIN,
                    GPIO_PIN_SET);
}

static uint8_t AHT10_InitSensor(void) {
  static const uint8_t initCommand[3] = {0xE1U, 0x08U, 0x00U};

  BoardI2c_GpioInit();
  HAL_Delay(AHT10_INIT_DELAY_MS);

  if (AHT10_WriteCommand(initCommand, sizeof(initCommand)) == 0U) {
    return 0U;
  }

  HAL_Delay(AHT10_INIT_DELAY_MS);
  aht10Initialized = 1U;
  return 1U;
}

static uint8_t AHT10_Fail(ClockEnvironment_t *environment) {
  aht10MeasurePending = 0U;
  environment->temperature = 0U;
  environment->humidity = 0U;
  environment->isValid = 0U;
  lastEnvironment = *environment;
  return 1U;
}

static uint8_t BH1750_WriteCommand(uint8_t command) {
  return BoardI2c_Write(BH1750_ADDRESS, &command, 1U);
}

static uint8_t BH1750_InitSensor(void) {
  BoardI2c_GpioInit();

  if (BH1750_WriteCommand(BH1750_POWER_ON) == 0U) {
    return 0U;
  }
  (void)BH1750_WriteCommand(BH1750_RESET);
  if (BH1750_WriteCommand(BH1750_CONT_HIGH_RES_MODE) == 0U) {
    return 0U;
  }

  lastBrightnessReadMs = HAL_GetTick();
  bh1750Initialized = 1U;
  return 1U;
}

static uint8_t BH1750_ReadLux(uint16_t *lux) {
  uint8_t data[2] = {0U, 0U};
  uint32_t raw;

  if ((bh1750Initialized == 0U) && (BH1750_InitSensor() == 0U)) {
    return 0U;
  }

  if (BoardI2c_Read(BH1750_ADDRESS, data, sizeof(data)) == 0U) {
    bh1750Initialized = 0U;
    return 0U;
  }

  raw = (((uint32_t)data[0]) << 8U) | data[1];
  *lux = (uint16_t)((raw * 5U) / 6U);
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

  HAL_GPIO_WritePin(SHIFT_REGISTER_DATA_GPIO, SHIFT_REGISTER_DATA_PIN,
                    GPIO_PIN_RESET);
  HAL_GPIO_WritePin(SHIFT_REGISTER_CLOCK_GPIO, SHIFT_REGISTER_CLOCK_PIN,
                    GPIO_PIN_RESET);
  HAL_GPIO_WritePin(SHIFT_REGISTER_LATCH_GPIO, SHIFT_REGISTER_LATCH_PIN,
                    GPIO_PIN_RESET);

  GPIO_InitStruct.Pin = BUZZER_PIN;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(BUZZER_GPIO, &GPIO_InitStruct);
  HAL_GPIO_WritePin(BUZZER_GPIO, BUZZER_PIN, GPIO_PIN_RESET);

  GPIO_InitStruct.Pin = GPIO_PIN_7;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  BoardI2c_GpioInit();

  ShiftRegister_OutputEnablePwmInit(DISPLAY_TEST_BRIGHTNESS);
  Board_WriteDisplay((ClockDisplay_t){0U, 0U});
}

uint8_t Board_IsMainPowerPresent(void) {
  return Board_IsBatteryAboveLowThreshold();
}

uint8_t Board_IsWakePowerPresent(void) {
  return (HAL_GPIO_ReadPin(MAIN_POWER_WAKEUP_GPIO,
                           MAIN_POWER_WAKEUP_GPIO_PIN) == GPIO_PIN_SET)
             ? 1U
             : 0U;
}

uint8_t Board_IsBatteryAboveLowThreshold(void) {
  static uint8_t batteryLow = 0U;
  uint8_t aboveSamples = 0U;
  uint8_t belowSamples = 0U;

  for (uint8_t sample = 0U; sample < MAIN_POWER_CONFIRM_SAMPLES; ++sample) {
    uint16_t batteryMv = MainPower_ReadBatteryMv();
    if (batteryMv >= MAIN_POWER_BATTERY_MV_RECOVER) {
      ++aboveSamples;
    }
    if (batteryMv < MAIN_POWER_BATTERY_MV_MIN) {
      ++belowSamples;
    }
  }

  if (batteryLow != 0U) {
    if (aboveSamples >= MAIN_POWER_CONFIRM_SAMPLES) {
      batteryLow = 0U;
    }
  } else if (belowSamples >= MAIN_POWER_CONFIRM_SAMPLES) {
    batteryLow = 1U;
  }

  return (batteryLow == 0U) ? 1U : 0U;
}

uint16_t Board_ReadBatteryPercentTenths(void) {
  uint16_t batteryMv = MainPower_ReadBatteryMv();
  uint16_t percentTenths = BatteryPercentTenthsFromMv(batteryMv);
  uint32_t now = HAL_GetTick();
  static uint16_t filteredPercentTenths = BATTERY_PERCENT_UNINITIALIZED;
  static uint16_t displayedPercentTenths = BATTERY_PERCENT_UNINITIALIZED;
  static uint32_t lastDisplayUpdateMs = 0U;

  if (filteredPercentTenths == BATTERY_PERCENT_UNINITIALIZED) {
    filteredPercentTenths = percentTenths;
    displayedPercentTenths = percentTenths;
    lastDisplayUpdateMs = now;
    return displayedPercentTenths;
  }

  if ((percentTenths == 0U) || (percentTenths == 1000U)) {
    filteredPercentTenths = percentTenths;
    displayedPercentTenths = percentTenths;
    lastDisplayUpdateMs = now;
    return displayedPercentTenths;
  }

  filteredPercentTenths =
      (uint16_t)((((uint32_t)filteredPercentTenths *
                   ((1UL << BATTERY_PERCENT_FILTER_SHIFT) - 1UL)) +
                  percentTenths + (1UL << (BATTERY_PERCENT_FILTER_SHIFT - 1U))) >>
                 BATTERY_PERCENT_FILTER_SHIFT);

  if (((now - lastDisplayUpdateMs) >= BATTERY_PERCENT_UPDATE_INTERVAL_MS) &&
      ((filteredPercentTenths >= displayedPercentTenths &&
        (filteredPercentTenths - displayedPercentTenths) >=
            BATTERY_PERCENT_DISPLAY_HYSTERESIS_TENTHS) ||
       (filteredPercentTenths < displayedPercentTenths &&
        (displayedPercentTenths - filteredPercentTenths) >=
            BATTERY_PERCENT_DISPLAY_HYSTERESIS_TENTHS))) {
    displayedPercentTenths = filteredPercentTenths;
    lastDisplayUpdateMs = now;
  }

  return displayedPercentTenths;
}

void Board_EnterStandby(void) {
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  uint32_t waitStartMs = HAL_GetTick();

  Board_PowerDownExternalDevices(GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3 |
                                     GPIO_PIN_4 | GPIO_PIN_5 | GPIO_PIN_6 |
                                     GPIO_PIN_7 | GPIO_PIN_9 | GPIO_PIN_10 |
                                     GPIO_PIN_11 | GPIO_PIN_12 | GPIO_PIN_15,
                                 1U);

  while (HAL_GPIO_ReadPin(MAIN_POWER_WAKEUP_GPIO,
                          MAIN_POWER_WAKEUP_GPIO_PIN) == GPIO_PIN_SET) {
    if ((HAL_GetTick() - waitStartMs) >= MAIN_POWER_WAKEUP_LOW_WAIT_MS) {
      return;
    }
  }
  HAL_Delay(MAIN_POWER_WAKEUP_LOW_DEBOUNCE_MS);
  if (HAL_GPIO_ReadPin(MAIN_POWER_WAKEUP_GPIO,
                       MAIN_POWER_WAKEUP_GPIO_PIN) == GPIO_PIN_SET) {
    return;
  }

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

static void Board_PowerDownExternalDevices(uint16_t gpioAPins, uint8_t stopAdc) {
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  Board_WriteDisplay((ClockDisplay_t){0U, 0U});
  Oled128x32_Clear();
  Board_SetBuzzer(0U);

  TIM2->CCR1 = 0U;
  TIM2->CCER = 0U;
  TIM2->CR1 = 0U;
  HAL_GPIO_WritePin(SHIFT_REGISTER_OUTPUT_ENABLE_GPIO,
                    SHIFT_REGISTER_OUTPUT_ENABLE_PIN, GPIO_PIN_SET);

  if (stopAdc != 0U) {
    HAL_ADC_Stop(&hadc);
    HAL_ADCEx_DisableVREFINT();
  }

  GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Pin = gpioAPins;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  if (stopAdc != 0U) {
    GPIO_InitStruct.Pin = GPIO_PIN_1;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
  }
}

void Board_PowerDownIdleDevices(void) {
  Board_WriteDisplay((ClockDisplay_t){0U, 0U});
  Oled128x32_Clear();
  Board_SetBuzzer(0U);
}

ClockButton_t Board_ReadButton(void) {
  return DecodeButton(ADC_ReadChannel(BUTTONS_ADC_CHANNEL));
}

uint8_t Board_ReadBrightness(void) {
  static uint8_t currentLevel = 0U;
  uint32_t nowMs = HAL_GetTick();
  uint16_t lux = 0U;
  uint32_t luxRange = LIGHT_SENSOR_LUX_BRIGHT - LIGHT_SENSOR_LUX_DARK;
  uint8_t maxLevel = LIGHT_SENSOR_BRIGHTNESS_LEVELS - 1U;
  uint8_t nextLevel = currentLevel;
  uint32_t brightnessRange;

  if ((luxRange == 0U) || (maxLevel == 0U)) {
    return currentBrightness;
  }

  if (bh1750Initialized == 0U) {
    (void)BH1750_InitSensor();
    return currentBrightness;
  }

  if ((nowMs - lastBrightnessReadMs) < BH1750_READ_INTERVAL_MS) {
    return currentBrightness;
  }
  lastBrightnessReadMs = nowMs;

  if (BH1750_ReadLux(&lux) == 0U) {
    return currentBrightness;
  }

  if (lux <= LIGHT_SENSOR_LUX_DARK) {
    nextLevel = 0U;
  } else if (lux >= LIGHT_SENSOR_LUX_BRIGHT) {
    nextLevel = maxLevel;
  } else {
    uint32_t luxOffset = lux - LIGHT_SENSOR_LUX_DARK;
    nextLevel =
        (uint8_t)((luxOffset * maxLevel + (luxRange / 2U)) / luxRange);
  }

  if (nextLevel > currentLevel) {
    uint32_t upThreshold =
        LIGHT_SENSOR_LUX_DARK +
        ((uint32_t)(currentLevel + 1U) * luxRange) / maxLevel;
    if (lux >= (upThreshold + LIGHT_SENSOR_LUX_HYSTERESIS)) {
      currentLevel = nextLevel;
    }
  } else if (nextLevel < currentLevel) {
    uint32_t downThreshold =
        LIGHT_SENSOR_LUX_DARK + ((uint32_t)currentLevel * luxRange) / maxLevel;
    if ((lux + LIGHT_SENSOR_LUX_HYSTERESIS) <= downThreshold) {
      currentLevel = nextLevel;
    }
  }

  brightnessRange = LIGHT_SENSOR_BRIGHTNESS_MAX - LIGHT_SENSOR_BRIGHTNESS_MIN;
  return (uint8_t)(LIGHT_SENSOR_BRIGHTNESS_MIN +
                   (((uint32_t)currentLevel * brightnessRange) / maxLevel));
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
  dateTime->year = date.Year;
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

void Board_WriteDate(uint8_t day, uint8_t month, uint8_t year) {
  RTC_DateTypeDef date = {0};

  date.WeekDay = RTC_WEEKDAY_MONDAY;
  date.Month = month;
  date.Date = day;
  date.Year = (uint8_t)(year % 100U);
  (void)HAL_RTC_SetDate(&hrtc, &date, RTC_FORMAT_BIN);
}

uint8_t Board_ReadEnvironment(ClockEnvironment_t *environment) {
  static const uint8_t measureCommand[3] = {0xACU, 0x33U, 0x00U};
  uint8_t data[6] = {0U, 0U, 0U, 0U, 0U, 0U};
  uint32_t nowMs = HAL_GetTick();
  uint32_t humidityRaw;
  uint32_t temperatureRaw;
  uint32_t humidity;
  int32_t temperatureCx10;

  if ((aht10MeasurePending == 0U) &&
      ((nowMs - lastEnvironmentReadMs) < AHT10_MIN_READ_INTERVAL_MS) &&
      (lastEnvironment.isValid != 0U)) {
    *environment = lastEnvironment;
    return 1U;
  }

  if ((aht10Initialized == 0U) && (AHT10_InitSensor() == 0U)) {
    return AHT10_Fail(environment);
  }

  if (aht10MeasurePending == 0U) {
    if (AHT10_WriteCommand(measureCommand, sizeof(measureCommand)) == 0U) {
      aht10Initialized = 0U;
      return AHT10_Fail(environment);
    }
    aht10MeasurePending = 1U;
    aht10MeasureStartMs = nowMs;
    return 0U;
  }

  if ((nowMs - aht10MeasureStartMs) < AHT10_MEASURE_DELAY_MS) {
    return 0U;
  }
  aht10MeasurePending = 0U;
  lastEnvironmentReadMs = nowMs;

  if (AHT10_ReadData(data, sizeof(data)) == 0U) {
    aht10Initialized = 0U;
    return AHT10_Fail(environment);
  }

  if ((data[0] & AHT10_STATUS_BUSY) != 0U) {
    return AHT10_Fail(environment);
  }

  humidityRaw =
      (((uint32_t)data[1]) << 12U) | (((uint32_t)data[2]) << 4U) |
      (((uint32_t)data[3]) >> 4U);
  temperatureRaw = ((((uint32_t)data[3]) & 0x0FU) << 16U) |
                   (((uint32_t)data[4]) << 8U) | data[5];

  humidity = ((humidityRaw * 100U) + 524288U) >> 20U;
  if (humidity > 100U) {
    humidity = 100U;
  }

  temperatureCx10 =
      (int32_t)(((temperatureRaw * 2000U) + 524288U) >> 20U) - 500;
  if (temperatureCx10 < 0) {
    temperatureCx10 = 0;
  }
  temperatureCx10 = (temperatureCx10 + 5) / 10;
  if (temperatureCx10 > 99) {
    temperatureCx10 = 99;
  }

  environment->humidity = (uint8_t)humidity;
  environment->temperature = (uint8_t)temperatureCx10;
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
