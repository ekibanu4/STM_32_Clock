#include "alarm_manager.h"

#include "app_config.h"
#include "board.h"

#define ALARM_RAM_SIGNATURE 0xA7U
#define ALARM_RAM_VERSION 0x01U
#define ALARM_RAM_CHECKSUM_XOR 0x5AU

static AlarmSlot_t alarmSlots[ALARM_SLOT_COUNT] = {{0U, 0U, 0U, 0U},
                                                   {0U, 0U, 0U, 0U},
                                                   {0U, 0U, 0U, 0U}};
static uint8_t selectedAlarmSlot = 0U;
static uint32_t alarmBuzzerTicks = 0U;
static uint8_t lastAlarmTriggerHour = 255U;
static uint8_t lastAlarmTriggerMinute = 255U;

static uint8_t IncrementWrap(uint8_t value, uint8_t minimum, uint8_t maximum) {
    if (value >= maximum) {
        return minimum;
    }
    return (uint8_t)(value + 1U);
}

static uint8_t DecrementWrap(uint8_t value, uint8_t minimum, uint8_t maximum) {
    if (value <= minimum) {
        return maximum;
    }
    return (uint8_t)(value - 1U);
}

static uint8_t AlarmSlotFlags(const AlarmSlot_t *slot) {
    uint8_t flags = 0U;

    if (slot->configured != 0U) {
        flags |= 0x01U;
    }
    if (slot->enabled != 0U) {
        flags |= 0x02U;
    }

    return flags;
}

static uint8_t AlarmRamChecksum(const uint8_t *data, uint8_t size) {
    uint8_t checksum = 0U;

    for (uint8_t index = 0U; index < size; ++index) {
        checksum = (uint8_t)(checksum + data[index]);
    }

    return (uint8_t)(checksum ^ ALARM_RAM_CHECKSUM_XOR);
}

static uint8_t IsAlarmRamImageValid(const uint8_t *ramImage) {
    return ((ramImage[0U] == ALARM_RAM_SIGNATURE) &&
            (ramImage[1U] == ALARM_RAM_VERSION) &&
            (ramImage[11U] == AlarmRamChecksum(ramImage, 11U)));
}

static void SaveAlarmsToRtcRam(void) {
    uint8_t ramImage[12U] = {0U};

    ramImage[0U] = ALARM_RAM_SIGNATURE;
    ramImage[1U] = ALARM_RAM_VERSION;
    for (uint8_t slotIndex = 0U; slotIndex < ALARM_SLOT_COUNT; ++slotIndex) {
        uint8_t offset = (uint8_t)(2U + (slotIndex * 3U));
        ramImage[offset] = AlarmSlotFlags(&alarmSlots[slotIndex]);
        ramImage[offset + 1U] = alarmSlots[slotIndex].hour;
        ramImage[offset + 2U] = alarmSlots[slotIndex].minute;
    }
    ramImage[11U] = AlarmRamChecksum(ramImage, 11U);

    Board_WriteAlarmStorage(ramImage, sizeof(ramImage));
}

static void ClearAlarms(void) {
    for (uint8_t slotIndex = 0U; slotIndex < ALARM_SLOT_COUNT; ++slotIndex) {
        alarmSlots[slotIndex].configured = 0U;
        alarmSlots[slotIndex].enabled = 0U;
        alarmSlots[slotIndex].hour = 0U;
        alarmSlots[slotIndex].minute = 0U;
    }
}

static void LoadAlarmsFromRtcRam(void) {
    uint8_t ramImage[12U] = {0U};

    for (uint8_t attempt = 0U; attempt < 3U; ++attempt) {
        Board_ReadAlarmStorage(ramImage, sizeof(ramImage));
        if (IsAlarmRamImageValid(ramImage) != 0U) {
            break;
        }
    }

    if (IsAlarmRamImageValid(ramImage) == 0U) {
        ClearAlarms();
        return;
    }

    for (uint8_t slotIndex = 0U; slotIndex < ALARM_SLOT_COUNT; ++slotIndex) {
        uint8_t offset = (uint8_t)(2U + (slotIndex * 3U));
        uint8_t flags = ramImage[offset];
        uint8_t hour = ramImage[offset + 1U];
        uint8_t minute = ramImage[offset + 2U];

        alarmSlots[slotIndex].configured = (uint8_t)(flags & 0x01U);
        alarmSlots[slotIndex].enabled = (uint8_t)((flags >> 1U) & 0x01U);
        alarmSlots[slotIndex].hour = (hour <= 23U) ? hour : 0U;
        alarmSlots[slotIndex].minute = (minute <= 59U) ? minute : 0U;
        if (alarmSlots[slotIndex].configured == 0U) {
            alarmSlots[slotIndex].enabled = 0U;
        }
    }
}

void AlarmManager_Init(void) {
    LoadAlarmsFromRtcRam();
}

void AlarmManager_StopBuzzer(void) {
    alarmBuzzerTicks = 0U;
    Board_SetBuzzer(0U);
}

void AlarmManager_DisableAll(void) {
    AlarmManager_StopBuzzer();

    for (uint8_t slotIndex = 0U; slotIndex < ALARM_SLOT_COUNT; ++slotIndex) {
        alarmSlots[slotIndex].enabled = 0U;
    }

    SaveAlarmsToRtcRam();
}

void AlarmManager_UpdateTrigger(const ClockDateTime_t *dateTime) {
    if ((dateTime->hours == lastAlarmTriggerHour) &&
        (dateTime->minutes == lastAlarmTriggerMinute)) {
        return;
    }

    for (uint8_t slotIndex = 0U; slotIndex < ALARM_SLOT_COUNT; ++slotIndex) {
        AlarmSlot_t *slot = &alarmSlots[slotIndex];

        if ((slot->enabled != 0U) && (slot->configured != 0U) &&
            (slot->hour == dateTime->hours) &&
            (slot->minute == dateTime->minutes)) {
            alarmBuzzerTicks = ALARM_BUZZER_DURATION_TICKS;
            lastAlarmTriggerHour = dateTime->hours;
            lastAlarmTriggerMinute = dateTime->minutes;
            return;
        }
    }
}

void AlarmManager_UpdateBuzzer(void) {
    uint8_t buzzerOn = 0U;

    if (alarmBuzzerTicks > 0U) {
        uint32_t elapsedTicks = ALARM_BUZZER_DURATION_TICKS - alarmBuzzerTicks;
        uint32_t groupPhase = elapsedTicks % ALARM_BUZZER_GROUP_TICKS;
        uint32_t beepsWindowTicks =
            ALARM_BUZZER_BEEP_COUNT * ALARM_BUZZER_BEEP_STEP_TICKS;

        if (groupPhase < beepsWindowTicks) {
            uint32_t beepPhase = groupPhase % ALARM_BUZZER_BEEP_STEP_TICKS;
            buzzerOn = (beepPhase < ALARM_BUZZER_BEEP_ON_TICKS) ? 1U : 0U;
        }
        --alarmBuzzerTicks;
    }

    Board_SetBuzzer(buzzerOn);
}

uint8_t AlarmManager_AnyEnabled(void) {
    for (uint8_t slotIndex = 0U; slotIndex < ALARM_SLOT_COUNT; ++slotIndex) {
        if (alarmSlots[slotIndex].enabled != 0U) {
            return 1U;
        }
    }

    return 0U;
}

const AlarmSlot_t *AlarmManager_Slots(void) {
    return alarmSlots;
}

uint8_t AlarmManager_SlotCount(void) {
    return ALARM_SLOT_COUNT;
}

uint8_t AlarmManager_SelectedSlot(void) {
    return selectedAlarmSlot;
}

void AlarmManager_SelectNextSlot(void) {
    selectedAlarmSlot =
        IncrementWrap(selectedAlarmSlot, 0U, (uint8_t)(ALARM_SLOT_COUNT - 1U));
}

void AlarmManager_SelectPreviousSlot(void) {
    selectedAlarmSlot =
        DecrementWrap(selectedAlarmSlot, 0U, (uint8_t)(ALARM_SLOT_COUNT - 1U));
}

void AlarmManager_IncrementSelectedHour(void) {
    alarmSlots[selectedAlarmSlot].hour =
        IncrementWrap(alarmSlots[selectedAlarmSlot].hour, 0U, 23U);
}

void AlarmManager_DecrementSelectedHour(void) {
    alarmSlots[selectedAlarmSlot].hour =
        DecrementWrap(alarmSlots[selectedAlarmSlot].hour, 0U, 23U);
}

void AlarmManager_IncrementSelectedMinute(void) {
    alarmSlots[selectedAlarmSlot].minute =
        IncrementWrap(alarmSlots[selectedAlarmSlot].minute, 0U, 59U);
}

void AlarmManager_DecrementSelectedMinute(void) {
    alarmSlots[selectedAlarmSlot].minute =
        DecrementWrap(alarmSlots[selectedAlarmSlot].minute, 0U, 59U);
}

void AlarmManager_EnableSelectedSlot(void) {
    alarmSlots[selectedAlarmSlot].configured = 1U;
    alarmSlots[selectedAlarmSlot].enabled = 1U;
    SaveAlarmsToRtcRam();
}

uint8_t AlarmManager_DisableSelectedSlotIfEnabled(void) {
    if (alarmSlots[selectedAlarmSlot].enabled == 0U) {
        return 0U;
    }

    alarmSlots[selectedAlarmSlot].enabled = 0U;
    SaveAlarmsToRtcRam();
    return 1U;
}
