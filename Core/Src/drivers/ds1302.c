#include "ds1302.h"

#include "board_config.h"

#define DS1302_SECONDS_WRITE 0x80U
#define DS1302_SECONDS_READ 0x81U
#define DS1302_MINUTES_WRITE 0x82U
#define DS1302_MINUTES_READ 0x83U
#define DS1302_HOURS_WRITE 0x84U
#define DS1302_HOURS_READ 0x85U
#define DS1302_DATE_WRITE 0x86U
#define DS1302_DATE_READ 0x87U
#define DS1302_MONTH_WRITE 0x88U
#define DS1302_MONTH_READ 0x89U
#define DS1302_CONTROL_WRITE 0x8EU
#define DS1302_RAM_WRITE_BASE 0xC0U
#define DS1302_RAM_READ_BASE 0xC1U

static uint8_t BcdToDecimal(uint8_t bcdValue) {
  return (uint8_t)(((bcdValue >> 4U) * 10U) + (bcdValue & 0x0FU));
}

static uint8_t DecimalToBcd(uint8_t decimalValue) {
  return (uint8_t)(((decimalValue / 10U) << 4U) | (decimalValue % 10U));
}

static void DS1302_DataOutput(void) {
  GPIO_SetPinMode(DS1302_GPIO, DS1302_DATA_PIN, OUTPUT);
}

static void DS1302_DataInput(void) {
  GPIO_SetPinMode(DS1302_GPIO, DS1302_DATA_PIN, INPUT);
}

void DS1302_Init(void) {
  RCC_EnableGPIO(DS1302_GPIO);

  GPIO_SetPinMode(DS1302_GPIO, DS1302_CLK_PIN, OUTPUT);
  GPIO_SetPinMode(DS1302_GPIO, DS1302_RST_PIN, OUTPUT);
  DS1302_DataOutput();

  GPIO_SetPinPull(DS1302_GPIO, DS1302_CLK_PIN, NO_PUDR);
  GPIO_SetPinPull(DS1302_GPIO, DS1302_RST_PIN, NO_PUDR);
  GPIO_SetPinPull(DS1302_GPIO, DS1302_DATA_PIN, NO_PUDR);

  GPIO_SetPinOutputType(DS1302_GPIO, DS1302_CLK_PIN, PUSH_PULL);
  GPIO_SetPinOutputType(DS1302_GPIO, DS1302_RST_PIN, PUSH_PULL);
  GPIO_SetPinOutputType(DS1302_GPIO, DS1302_DATA_PIN, PUSH_PULL);

  GPIO_SetPinSpeed(DS1302_GPIO, DS1302_CLK_PIN, LOW_SPEED);
  GPIO_SetPinSpeed(DS1302_GPIO, DS1302_RST_PIN, LOW_SPEED);
  GPIO_SetPinSpeed(DS1302_GPIO, DS1302_DATA_PIN, LOW_SPEED);

  GPIO_ResetPin(DS1302_GPIO, DS1302_CLK_PIN);
  GPIO_ResetPin(DS1302_GPIO, DS1302_RST_PIN);
  GPIO_ResetPin(DS1302_GPIO, DS1302_DATA_PIN);
}

static void DS1302_WriteByte(uint8_t value) {
  DS1302_DataOutput();

  for (uint8_t bit = 0U; bit < 8U; ++bit) {
    GPIO_WritePin(DS1302_GPIO, DS1302_DATA_PIN,
                  ((value >> bit) & 0x1U) ? ON : OFF);
    GPIO_SetPin(DS1302_GPIO, DS1302_CLK_PIN);
    DelayCycles(20U);
    GPIO_ResetPin(DS1302_GPIO, DS1302_CLK_PIN);
    DelayCycles(20U);
  }
}

static uint8_t DS1302_ReadByte(void) {
  uint8_t value = 0U;
  DS1302_DataInput();

  for (uint8_t bit = 0U; bit < 8U; ++bit) {
    if (GPIO_ReadPin(DS1302_GPIO, DS1302_DATA_PIN) != 0U) {
      value |= (uint8_t)(1U << bit);
    }
    GPIO_SetPin(DS1302_GPIO, DS1302_CLK_PIN);
    DelayCycles(20U);
    GPIO_ResetPin(DS1302_GPIO, DS1302_CLK_PIN);
    DelayCycles(20U);
  }

  return value;
}

static void DS1302_WriteRegister(uint8_t command, uint8_t value) {
  GPIO_SetPin(DS1302_GPIO, DS1302_RST_PIN);
  DelayCycles(20U);
  DS1302_WriteByte(command);
  DS1302_WriteByte(value);
  GPIO_ResetPin(DS1302_GPIO, DS1302_RST_PIN);
  GPIO_ResetPin(DS1302_GPIO, DS1302_CLK_PIN);
}

static uint8_t DS1302_ReadRegister(uint8_t command) {
  uint8_t value = 0U;

  GPIO_SetPin(DS1302_GPIO, DS1302_RST_PIN);
  DelayCycles(20U);
  DS1302_WriteByte(command);
  value = DS1302_ReadByte();
  GPIO_ResetPin(DS1302_GPIO, DS1302_RST_PIN);
  GPIO_ResetPin(DS1302_GPIO, DS1302_CLK_PIN);
  DS1302_DataOutput();

  return value;
}

static void DS1302_SetWriteProtect(uint8_t isEnabled) {
  DS1302_WriteRegister(DS1302_CONTROL_WRITE, isEnabled ? 0x80U : 0x00U);
}

void DS1302_StartClock(void) {
  uint8_t seconds = DS1302_ReadRegister(DS1302_SECONDS_READ);
  DS1302_SetWriteProtect(0U);
  DS1302_WriteRegister(DS1302_SECONDS_WRITE, (uint8_t)(seconds & 0x7FU));
}

void DS1302_ReadDateTime(DateTime_t *dateTime) {
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
}

void DS1302_WriteTime(uint8_t hours, uint8_t minutes, uint8_t seconds) {
  DS1302_SetWriteProtect(0U);
  DS1302_WriteRegister(DS1302_SECONDS_WRITE, DecimalToBcd(seconds));
  DS1302_WriteRegister(DS1302_MINUTES_WRITE, DecimalToBcd(minutes));
  DS1302_WriteRegister(DS1302_HOURS_WRITE, DecimalToBcd((uint8_t)(hours & 0x3FU)));
}

void DS1302_WriteDate(uint8_t day, uint8_t month) {
  DS1302_SetWriteProtect(0U);
  DS1302_WriteRegister(DS1302_DATE_WRITE, DecimalToBcd(day));
  DS1302_WriteRegister(DS1302_MONTH_WRITE, DecimalToBcd(month));
}

static uint8_t DS1302_RamCommand(uint8_t address, uint8_t isRead) {
  address %= 31U;
  return (uint8_t)(DS1302_RAM_WRITE_BASE + (address * 2U) + (isRead ? 1U : 0U));
}

void DS1302_WriteRamByte(uint8_t address, uint8_t value) {
  DS1302_SetWriteProtect(0U);
  DS1302_WriteRegister(DS1302_RamCommand(address, 0U), value);
}

uint8_t DS1302_ReadRamByte(uint8_t address) {
  return DS1302_ReadRegister(DS1302_RamCommand(address, 1U));
}
