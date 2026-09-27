#ifndef ALARM_MANAGER_H_
#define ALARM_MANAGER_H_

#include <stdint.h>

#include "app_types.h"

void AlarmManager_Init(void);
void AlarmManager_StopBuzzer(void);
void AlarmManager_DisableAll(void);
void AlarmManager_ToggleAll(void);
void AlarmManager_UpdateTrigger(const ClockDateTime_t *dateTime);
void AlarmManager_UpdateBuzzer(void);

uint8_t AlarmManager_IsBuzzerActive(void);
uint8_t AlarmManager_ActiveSlot(void);
uint8_t AlarmManager_AnyEnabled(void);
const AlarmSlot_t *AlarmManager_Slots(void);
uint8_t AlarmManager_SlotCount(void);
uint8_t AlarmManager_SelectedSlot(void);

void AlarmManager_SelectNextSlot(void);
void AlarmManager_SelectPreviousSlot(void);
void AlarmManager_IncrementSelectedHour(void);
void AlarmManager_DecrementSelectedHour(void);
void AlarmManager_IncrementSelectedMinute(void);
void AlarmManager_DecrementSelectedMinute(void);
void AlarmManager_EnableSelectedSlot(void);
uint8_t AlarmManager_ToggleSelectedSlot(void);
uint8_t AlarmManager_DisableSelectedSlotIfEnabled(void);

#endif /* ALARM_MANAGER_H_ */
