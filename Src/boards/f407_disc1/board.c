#include "board.h"

#include "board_config.h"
#include "buzzer.h"
#include "buttons_decoder.h"
#include "dht11.h"
#include "ds1302.h"
#include "light_sensor.h"
#include "shift_register.h"

static ClockButton_t Board_ConvertButton(ButtonCode_t button) {
  switch (button) {
  case BUTTON_MODE:
    return CLOCK_BUTTON_MODE;
  case BUTTON_SET:
    return CLOCK_BUTTON_SET;
  case BUTTON_UP:
    return CLOCK_BUTTON_UP;
  case BUTTON_DOWN:
    return CLOCK_BUTTON_DOWN;
  case BUTTON_OFF:
    return CLOCK_BUTTON_OFF;
  case BUTTON_NONE:
  default:
    return CLOCK_BUTTON_NONE;
  }
}

static ClockDateTime_t Board_FromHardwareDateTime(const DateTime_t *dateTime) {
  ClockDateTime_t appDateTime = {0U, 0U, 0U, 0U, 0U};

  appDateTime.seconds = dateTime->seconds;
  appDateTime.minutes = dateTime->minutes;
  appDateTime.hours = dateTime->hours;
  appDateTime.day = dateTime->day;
  appDateTime.month = dateTime->month;

  return appDateTime;
}

void Board_Init(void) {
  ShiftRegister_Init();
  ButtonsDecoder_Init();
  ShiftRegister_OutputEnablePwm_Init(LIGHT_SENSOR_BRIGHTNESS_MAX);
  DS1302_Init();
  Buzzer_Init();
  DHT11_Init();
  DS1302_StartClock();
}

ClockButton_t Board_ReadButton(void) {
  return Board_ConvertButton(ButtonsDecoder_Read());
}

uint8_t Board_ReadBrightness(void) {
  return LightSensor_ReadBrightness();
}

void Board_SetBrightness(uint8_t brightness) {
  ShiftRegister_SetBrightness(brightness);
}

void Board_WriteDisplay(ClockDisplay_t display) {
  ShiftRegister_WriteBoards(display.board1, display.board2);
}

void Board_ReadDateTime(ClockDateTime_t *dateTime) {
  DateTime_t hardwareDateTime = {0U, 0U, 0U, 0U, 0U};

  DS1302_ReadDateTime(&hardwareDateTime);
  *dateTime = Board_FromHardwareDateTime(&hardwareDateTime);
}

void Board_WriteTime(uint8_t hours, uint8_t minutes, uint8_t seconds) {
  DS1302_WriteTime(hours, minutes, seconds);
}

void Board_WriteDate(uint8_t day, uint8_t month) {
  DS1302_WriteDate(day, month);
}

uint8_t Board_ReadEnvironment(ClockEnvironment_t *environment) {
  DHT11_Reading_t dhtReading = {0U, 0U, 0U};
  uint8_t result = DHT11_Read(&dhtReading);

  if (result != 0U) {
    environment->temperature = dhtReading.temperature;
    environment->humidity = dhtReading.humidity;
    environment->isValid = dhtReading.isValid;
  }

  return result;
}

void Board_ReadAlarmStorage(uint8_t *data, uint8_t size) {
  for (uint8_t address = 0U; address < size; ++address) {
    data[address] = DS1302_ReadRamByte(address);
  }
}

void Board_WriteAlarmStorage(const uint8_t *data, uint8_t size) {
  for (uint8_t address = 0U; address < size; ++address) {
    DS1302_WriteRamByte(address, data[address]);
  }
}

void Board_SetBuzzer(uint8_t isEnabled) {
  Buzzer_SetEnabled(isEnabled);
}

void Board_DelayLoop(void) {
  DelayCycles(BOARD_LOOP_DELAY_CYCLES);
}
