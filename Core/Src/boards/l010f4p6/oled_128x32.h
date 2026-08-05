#ifndef L010F4P6_OLED_128X32_H_
#define L010F4P6_OLED_128X32_H_

#include <stdint.h>

#include "app_types.h"

void Oled128x32_Init(void);
void Oled128x32_Clear(void);
void Oled128x32_ShowAlarm(uint8_t alarmSlot);
void Oled128x32_Render(uint32_t nowMs, DisplayMode_t displayMode,
                        const ClockDateTime_t *dateTime,
                        const ClockEnvironment_t *environment,
                        const AlarmSlot_t *alarmSlots, uint8_t alarmSlotCount,
                        uint8_t selectedAlarmSlot, EditTarget_t editTarget,
                        uint16_t batteryPercentTenths,
                        uint8_t alarmAnyEnabled);

#endif /* L010F4P6_OLED_128X32_H_ */
