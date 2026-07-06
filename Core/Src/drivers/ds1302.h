#ifndef DS1302_H_
#define DS1302_H_

#include <stdint.h>

typedef struct {
  uint8_t seconds;
  uint8_t minutes;
  uint8_t hours;
  uint8_t day;
  uint8_t month;
} DateTime_t;

void DS1302_Init(void);
void DS1302_StartClock(void);
void DS1302_ReadDateTime(DateTime_t *dateTime);
void DS1302_WriteTime(uint8_t hours, uint8_t minutes, uint8_t seconds);
void DS1302_WriteDate(uint8_t day, uint8_t month);
void DS1302_WriteRamByte(uint8_t address, uint8_t value);
uint8_t DS1302_ReadRamByte(uint8_t address);

#endif /* DS1302_H_ */
