#ifndef BOARD_H_
#define BOARD_H_

#include <stdint.h>

typedef enum {
  CLOCK_BUTTON_NONE = 0,
  CLOCK_BUTTON_MODE,
  CLOCK_BUTTON_SET,
  CLOCK_BUTTON_UP,
  CLOCK_BUTTON_DOWN,
  CLOCK_BUTTON_OFF
} ClockButton_t;

typedef struct {
  uint8_t seconds;
  uint8_t minutes;
  uint8_t hours;
  uint8_t day;
  uint8_t month;
  uint8_t year;
} ClockDateTime_t;

typedef struct {
  uint8_t temperature;
  uint8_t humidity;
  uint8_t isValid;
} ClockEnvironment_t;

typedef struct {
  uint8_t board1;
  uint8_t board2;
} ClockDisplay_t;

enum {
  CLOCK_BOARD1_HOURS_32_BIT = 0U,
  CLOCK_BOARD1_HOURS_16_BIT = 1U,
  CLOCK_BOARD1_HOURS_8_BIT = 2U,
  CLOCK_BOARD1_HOURS_4_BIT = 3U,
  CLOCK_BOARD1_HOURS_2_BIT = 4U,
  CLOCK_BOARD1_HOURS_1_BIT = 5U,
  CLOCK_BOARD1_MINUTES_32_BIT = 6U,
  CLOCK_BOARD1_MINUTES_16_BIT = 7U,

  CLOCK_BOARD2_MINUTES_8_BIT = 0U,
  CLOCK_BOARD2_MINUTES_4_BIT = 1U,
  CLOCK_BOARD2_MINUTES_2_BIT = 2U,
  CLOCK_BOARD2_MINUTES_1_BIT = 3U,
  CLOCK_BOARD2_MODE_TIME_BIT = 4U,
  CLOCK_BOARD2_MODE_DATE_BIT = 5U,
  CLOCK_BOARD2_MODE_ENVIRONMENT_BIT = 6U,
  CLOCK_BOARD2_ALARM_BIT = 7U
};

void Board_Init(void);
uint8_t Board_IsMainPowerPresent(void);
void Board_EnterStandby(void);
ClockButton_t Board_ReadButton(void);
uint8_t Board_ReadBrightness(void);
void Board_SetBrightness(uint8_t brightness);
void Board_WriteDisplay(ClockDisplay_t display);
void Board_ReadDateTime(ClockDateTime_t *dateTime);
void Board_WriteTime(uint8_t hours, uint8_t minutes, uint8_t seconds);
void Board_WriteDate(uint8_t day, uint8_t month, uint8_t year);
uint8_t Board_ReadEnvironment(ClockEnvironment_t *environment);
void Board_ReadAlarmStorage(uint8_t *data, uint8_t size);
void Board_WriteAlarmStorage(const uint8_t *data, uint8_t size);
void Board_SetBuzzer(uint8_t isEnabled);
void Board_PowerDownExternalDevicesForTest(void);
void Board_DelayLoop(void);

#endif /* BOARD_H_ */
