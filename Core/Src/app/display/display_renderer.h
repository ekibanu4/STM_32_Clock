#ifndef DISPLAY_RENDERER_H_
#define DISPLAY_RENDERER_H_

#include <stdint.h>

#include "app_types.h"

ClockDisplay_t DisplayRenderer_Build(DisplayMode_t displayMode,
                                      EditTarget_t editTarget,
                                      const ClockDateTime_t *dateTime,
                                      const ClockEnvironment_t *environment,
                                      const AlarmSlot_t *alarmSlots,
                                      uint8_t alarmSlotCount,
                                      uint8_t selectedAlarmSlot,
                                      uint16_t batteryPercentTenths,
                                      uint8_t anyAlarmEnabled,
                                      uint8_t alarmErrorActive,
                                      uint8_t blinkOn);

#endif /* DISPLAY_RENDERER_H_ */
