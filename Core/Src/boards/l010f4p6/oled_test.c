#include "oled_test.h"

#include "stm32l0xx_hal.h"

#define OLED_SCL_GPIO GPIOA
#define OLED_SCL_PIN GPIO_PIN_9
#define OLED_SDA_GPIO GPIOA
#define OLED_SDA_PIN GPIO_PIN_10
#define OLED_ADDR_PRIMARY 0x3CU
#define OLED_ADDR_SECONDARY 0x3DU
#define OLED_WIDTH 128U
#define OLED_PAGE_COUNT 4U
#define OLED_TIME_COLUMN 16U
#define OLED_TIME_PAGE 1U
#define OLED_MODE_COLUMN 16U
#define OLED_MODE_PAGE 1U

static uint8_t oledAddress = OLED_ADDR_PRIMARY;
static uint8_t oledReady = 0U;

static void I2cDelay(void) {
  for (volatile uint8_t index = 0U; index < 20U; ++index) {
  }
}

static void SclHigh(void) {
  HAL_GPIO_WritePin(OLED_SCL_GPIO, OLED_SCL_PIN, GPIO_PIN_SET);
  I2cDelay();
}

static void SclLow(void) {
  HAL_GPIO_WritePin(OLED_SCL_GPIO, OLED_SCL_PIN, GPIO_PIN_RESET);
  I2cDelay();
}

static void SdaHigh(void) {
  HAL_GPIO_WritePin(OLED_SDA_GPIO, OLED_SDA_PIN, GPIO_PIN_SET);
  I2cDelay();
}

static void SdaLow(void) {
  HAL_GPIO_WritePin(OLED_SDA_GPIO, OLED_SDA_PIN, GPIO_PIN_RESET);
  I2cDelay();
}

static GPIO_PinState SdaRead(void) {
  return HAL_GPIO_ReadPin(OLED_SDA_GPIO, OLED_SDA_PIN);
}

static void I2cStart(void) {
  SdaHigh();
  SclHigh();
  SdaLow();
  SclLow();
}

static void I2cStop(void) {
  SdaLow();
  SclHigh();
  SdaHigh();
}

static uint8_t I2cWriteByte(uint8_t value) {
  for (uint8_t bit = 0U; bit < 8U; ++bit) {
    if ((value & 0x80U) != 0U) {
      SdaHigh();
    } else {
      SdaLow();
    }
    SclHigh();
    SclLow();
    value <<= 1;
  }

  SdaHigh();
  SclHigh();
  uint8_t ack = (SdaRead() == GPIO_PIN_RESET) ? 1U : 0U;
  SclLow();
  return ack;
}

static uint8_t Oled_Write(uint8_t address, uint8_t control, const uint8_t *data,
                          uint8_t size) {
  I2cStart();
  if (I2cWriteByte((uint8_t)(address << 1)) == 0U) {
    I2cStop();
    return 0U;
  }
  if (I2cWriteByte(control) == 0U) {
    I2cStop();
    return 0U;
  }
  for (uint8_t index = 0U; index < size; ++index) {
    if (I2cWriteByte(data[index]) == 0U) {
      I2cStop();
      return 0U;
    }
  }
  I2cStop();
  return 1U;
}

static uint8_t Oled_WriteCommand(uint8_t command) {
  return Oled_Write(oledAddress, 0x00U, &command, 1U);
}

static uint8_t Oled_SetCursor(uint8_t page, uint8_t column) {
  if (Oled_WriteCommand((uint8_t)(0xB0U | (page & 0x07U))) == 0U) {
    return 0U;
  }
  if (Oled_WriteCommand((uint8_t)(0x00U | (column & 0x0FU))) == 0U) {
    return 0U;
  }
  return Oled_WriteCommand((uint8_t)(0x10U | ((column >> 4) & 0x0FU)));
}

static uint8_t Oled_GlyphColumn(char symbol, uint8_t column) {
  switch (symbol) {
  case '0': {
    static const uint8_t glyph[5] = {0x3EU, 0x51U, 0x49U, 0x45U, 0x3EU};
    return glyph[column];
  }
  case '1': {
    static const uint8_t glyph[5] = {0x00U, 0x42U, 0x7FU, 0x40U, 0x00U};
    return glyph[column];
  }
  case '2': {
    static const uint8_t glyph[5] = {0x42U, 0x61U, 0x51U, 0x49U, 0x46U};
    return glyph[column];
  }
  case '3': {
    static const uint8_t glyph[5] = {0x21U, 0x41U, 0x45U, 0x4BU, 0x31U};
    return glyph[column];
  }
  case '4': {
    static const uint8_t glyph[5] = {0x18U, 0x14U, 0x12U, 0x7FU, 0x10U};
    return glyph[column];
  }
  case '5': {
    static const uint8_t glyph[5] = {0x27U, 0x45U, 0x45U, 0x45U, 0x39U};
    return glyph[column];
  }
  case '6': {
    static const uint8_t glyph[5] = {0x3CU, 0x4AU, 0x49U, 0x49U, 0x30U};
    return glyph[column];
  }
  case '7': {
    static const uint8_t glyph[5] = {0x01U, 0x71U, 0x09U, 0x05U, 0x03U};
    return glyph[column];
  }
  case '8': {
    static const uint8_t glyph[5] = {0x36U, 0x49U, 0x49U, 0x49U, 0x36U};
    return glyph[column];
  }
  case '9': {
    static const uint8_t glyph[5] = {0x06U, 0x49U, 0x49U, 0x29U, 0x1EU};
    return glyph[column];
  }
  case ':': {
    static const uint8_t glyph[5] = {0x00U, 0x36U, 0x36U, 0x00U, 0x00U};
    return glyph[column];
  }
  case '.': {
    static const uint8_t glyph[5] = {0x00U, 0x60U, 0x60U, 0x00U, 0x00U};
    return glyph[column];
  }
  case '-': {
    static const uint8_t glyph[5] = {0x08U, 0x08U, 0x08U, 0x08U, 0x08U};
    return glyph[column];
  }
  case ' ': {
    return 0x00U;
  }
  case 'H': {
    static const uint8_t glyph[5] = {0x7FU, 0x08U, 0x08U, 0x08U, 0x7FU};
    return glyph[column];
  }
  case 'T': {
    static const uint8_t glyph[5] = {0x01U, 0x01U, 0x7FU, 0x01U, 0x01U};
    return glyph[column];
  }
  default:
    return 0x00U;
  }
}

static void Oled_Fill(uint8_t value) {
  uint8_t data[16];

  for (uint8_t index = 0U; index < sizeof(data); ++index) {
    data[index] = value;
  }

  for (uint8_t page = 0U; page < OLED_PAGE_COUNT; ++page) {
    if (Oled_SetCursor(page, 0U) == 0U) {
      return;
    }
    for (uint8_t column = 0U; column < OLED_WIDTH; column += sizeof(data)) {
      if (Oled_Write(oledAddress, 0x40U, data, sizeof(data)) == 0U) {
        return;
      }
    }
  }
}

static void Oled_ClearTextPages(void) {
  uint8_t data[16];

  for (uint8_t index = 0U; index < sizeof(data); ++index) {
    data[index] = 0x00U;
  }

  for (uint8_t page = OLED_MODE_PAGE; page <= (OLED_MODE_PAGE + 1U); ++page) {
    if (Oled_SetCursor(page, 0U) == 0U) {
      return;
    }
    for (uint8_t column = 0U; column < OLED_WIDTH; column += sizeof(data)) {
      if (Oled_Write(oledAddress, 0x40U, data, sizeof(data)) == 0U) {
        return;
      }
    }
  }
}

static void Oled_WriteScaledText(const char *text, uint8_t page,
                                 uint8_t column) {
  uint8_t upper[12];
  uint8_t lower[12];

  if (Oled_SetCursor(page, column) == 0U) {
    return;
  }

  while (*text != '\0') {
    uint8_t outIndex = 0U;
    for (uint8_t glyphColumn = 0U; glyphColumn < 5U; ++glyphColumn) {
      uint8_t source = Oled_GlyphColumn(*text, glyphColumn);
      uint8_t top = 0U;
      uint8_t bottom = 0U;

      for (uint8_t bit = 0U; bit < 7U; ++bit) {
        if ((source & (1U << bit)) != 0U) {
          uint8_t scaledBit = (uint8_t)(bit * 2U);
          if (scaledBit < 8U) {
            top |= (uint8_t)(1U << scaledBit);
          } else {
            bottom |= (uint8_t)(1U << (scaledBit - 8U));
          }
          ++scaledBit;
          if (scaledBit < 8U) {
            top |= (uint8_t)(1U << scaledBit);
          } else {
            bottom |= (uint8_t)(1U << (scaledBit - 8U));
          }
        }
      }

      upper[outIndex] = top;
      lower[outIndex] = bottom;
      ++outIndex;
      upper[outIndex] = top;
      lower[outIndex] = bottom;
      ++outIndex;
    }
    upper[outIndex] = 0x00U;
    lower[outIndex] = 0x00U;
    ++outIndex;
    upper[outIndex] = 0x00U;
    lower[outIndex] = 0x00U;
    ++outIndex;

    if (Oled_Write(oledAddress, 0x40U, upper, outIndex) == 0U) {
      return;
    }
    if (Oled_SetCursor((uint8_t)(page + 1U), column) == 0U) {
      return;
    }
    if (Oled_Write(oledAddress, 0x40U, lower, outIndex) == 0U) {
      return;
    }
    column = (uint8_t)(column + outIndex);
    if (Oled_SetCursor(page, column) == 0U) {
      return;
    }
    ++text;
  }
}

static void Oled_ShowTime(const ClockDateTime_t *dateTime) {
  char text[9];

  text[0] = (char)('0' + ((dateTime->hours / 10U) % 10U));
  text[1] = (char)('0' + (dateTime->hours % 10U));
  text[2] = ':';
  text[3] = (char)('0' + ((dateTime->minutes / 10U) % 10U));
  text[4] = (char)('0' + (dateTime->minutes % 10U));
  text[5] = ':';
  text[6] = (char)('0' + ((dateTime->seconds / 10U) % 10U));
  text[7] = (char)('0' + (dateTime->seconds % 10U));
  text[8] = '\0';

  Oled_WriteScaledText(text, OLED_TIME_PAGE, OLED_TIME_COLUMN);
}

static void Oled_ShowDate(const ClockDateTime_t *dateTime) {
  char text[9];

  text[0] = (char)('0' + ((dateTime->day / 10U) % 10U));
  text[1] = (char)('0' + (dateTime->day % 10U));
  text[2] = '.';
  text[3] = (char)('0' + ((dateTime->month / 10U) % 10U));
  text[4] = (char)('0' + (dateTime->month % 10U));
  text[5] = '.';
  text[6] = (char)('0' + ((dateTime->year / 10U) % 10U));
  text[7] = (char)('0' + (dateTime->year % 10U));
  text[8] = '\0';

  Oled_WriteScaledText(text, OLED_MODE_PAGE, OLED_MODE_COLUMN);
}

static void Oled_ShowEnvironment(const ClockEnvironment_t *environment) {
  char text[8] = {'T', '-', '-', ' ', 'H', '-', '-', '\0'};

  if (environment->isValid != 0U) {
    text[1] = (char)('0' + ((environment->temperature / 10U) % 10U));
    text[2] = (char)('0' + (environment->temperature % 10U));
    text[5] = (char)('0' + ((environment->humidity / 10U) % 10U));
    text[6] = (char)('0' + (environment->humidity % 10U));
  }

  Oled_WriteScaledText(text, OLED_MODE_PAGE, OLED_MODE_COLUMN);
}

static uint8_t Oled_Probe(uint8_t address) {
  I2cStart();
  uint8_t ack = I2cWriteByte((uint8_t)(address << 1));
  I2cStop();
  return ack;
}

static void Oled_GpioInit(void) {
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  __HAL_RCC_GPIOA_CLK_ENABLE();

  GPIO_InitStruct.Pin = OLED_SCL_PIN | OLED_SDA_PIN;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  HAL_GPIO_WritePin(GPIOA, OLED_SCL_PIN | OLED_SDA_PIN, GPIO_PIN_SET);
}

void OledTest_Init(void) {
  static const uint8_t initCommands[] = {
      0xAEU, 0x20U, 0x00U, 0x40U, 0xA1U, 0xC8U, 0x81U, 0x7FU,
      0xA6U, 0xA8U, 0x1FU, 0xD3U, 0x00U, 0xD5U, 0x80U, 0xD9U,
      0xF1U, 0xDAU, 0x02U, 0xDBU, 0x40U, 0x8DU, 0x14U, 0xAFU};

  Oled_GpioInit();
  SdaHigh();
  SclHigh();

  oledAddress = OLED_ADDR_PRIMARY;
  if (Oled_Probe(oledAddress) == 0U) {
    oledAddress = OLED_ADDR_SECONDARY;
    if (Oled_Probe(oledAddress) == 0U) {
      oledReady = 0U;
      return;
    }
  }

  for (uint8_t index = 0U; index < sizeof(initCommands); ++index) {
    if (Oled_WriteCommand(initCommands[index]) == 0U) {
      oledReady = 0U;
      return;
    }
  }

  oledReady = 1U;
  Oled_Fill(0x00U);
}

void OledTest_Render(uint32_t nowMs, DisplayMode_t displayMode,
                     const ClockDateTime_t *dateTime,
                     const ClockEnvironment_t *environment) {
  static uint32_t lastDrawMs = 0xFFFFFFFFUL;
  static DisplayMode_t lastDisplayMode = DISPLAY_ALARM;
  static uint8_t lastSecond = 0xFFU;
  static uint8_t lastDay = 0xFFU;
  static uint8_t lastMonth = 0xFFU;
  static uint8_t lastYear = 0xFFU;
  static uint8_t lastTemperature = 0xFFU;
  static uint8_t lastHumidity = 0xFFU;
  static uint8_t lastEnvironmentValid = 0xFFU;
  uint8_t shouldDraw = 0U;
  uint8_t modeChanged = 0U;

  if (oledReady == 0U) {
    return;
  }

  if (displayMode != lastDisplayMode) {
    shouldDraw = 1U;
    modeChanged = 1U;
  } else if (displayMode == DISPLAY_TIME) {
    shouldDraw = (dateTime->seconds != lastSecond) ? 1U : 0U;
  } else if (displayMode == DISPLAY_DATE) {
    shouldDraw = ((dateTime->day != lastDay) || (dateTime->month != lastMonth) ||
                  (dateTime->year != lastYear))
                     ? 1U
                     : 0U;
  } else if (displayMode == DISPLAY_ENVIRONMENT) {
    shouldDraw = ((environment->isValid != lastEnvironmentValid) ||
                  (environment->temperature != lastTemperature) ||
                  (environment->humidity != lastHumidity))
                     ? 1U
                     : 0U;
  }

  if (shouldDraw == 0U) {
    return;
  }

  if ((lastDrawMs != 0xFFFFFFFFUL) && ((nowMs - lastDrawMs) < 200U)) {
    return;
  }

  lastDrawMs = nowMs;
  lastDisplayMode = displayMode;
  lastSecond = dateTime->seconds;
  lastDay = dateTime->day;
  lastMonth = dateTime->month;
  lastYear = dateTime->year;
  lastTemperature = environment->temperature;
  lastHumidity = environment->humidity;
  lastEnvironmentValid = environment->isValid;

  if (modeChanged != 0U) {
    Oled_ClearTextPages();
  }

  if (displayMode == DISPLAY_DATE) {
    Oled_ShowDate(dateTime);
  } else if (displayMode == DISPLAY_ENVIRONMENT) {
    Oled_ShowEnvironment(environment);
  } else {
    Oled_ShowTime(dateTime);
  }
}
