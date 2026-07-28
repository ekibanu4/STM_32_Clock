#ifndef L010F4P6_OLED_TEST_H_
#define L010F4P6_OLED_TEST_H_

#include <stdint.h>

#include "app_types.h"

void OledTest_Init(void);
void OledTest_Render(uint32_t nowMs, DisplayMode_t displayMode,
                     const ClockDateTime_t *dateTime,
                     const ClockEnvironment_t *environment,
                     const AlarmSlot_t *alarmSlots, uint8_t alarmSlotCount,
                     uint8_t selectedAlarmSlot, EditTarget_t editTarget,
                     uint8_t blinkOn);

#endif /* L010F4P6_OLED_TEST_H_ */
