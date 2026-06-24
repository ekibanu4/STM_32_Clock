#include "ui_controller.h"

#include <stdint.h>

#include "alarm_manager.h"
#include "app_config.h"
#include "auto_mode_scheduler.h"
#include "board.h"

static DisplayMode_t displayMode = DISPLAY_TIME;
static EditTarget_t editTarget = EDIT_NONE;
static ClockDateTime_t currentDateTime = {0U, 0U, 12U, 1U, 1U};
static ClockButton_t lastButton = CLOCK_BUTTON_NONE;
static uint32_t offButtonHoldTicks = 0U;
static uint8_t offLongPressHandled = 0U;

static uint8_t MonthDays(uint8_t month) {
    switch (month) {
    case 4U:
    case 6U:
    case 9U:
    case 11U:
        return 30U;
    case 2U:
        return 28U;
    case 1U:
    case 3U:
    case 5U:
    case 7U:
    case 8U:
    case 10U:
    case 12U:
    default:
        return 31U;
    }
}

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

static void SaveTimeToRtc(void) {
    Board_WriteTime(currentDateTime.hours, currentDateTime.minutes,
                    currentDateTime.seconds);
}

static void SaveDateToRtc(void) {
    Board_WriteDate(currentDateTime.day, currentDateTime.month);
}

static void NormalizeDateTime(void) {
    if (currentDateTime.seconds > 59U) {
        currentDateTime.seconds = 0U;
    }
    if (currentDateTime.minutes > 59U) {
        currentDateTime.minutes = 0U;
    }
    if (currentDateTime.hours > 23U) {
        currentDateTime.hours = 12U;
    }
    if ((currentDateTime.month < 1U) || (currentDateTime.month > 12U)) {
        currentDateTime.month = 1U;
    }
    if ((currentDateTime.day < 1U) ||
        (currentDateTime.day > MonthDays(currentDateTime.month))) {
        currentDateTime.day = 1U;
    }
}

static void HandleModeButton(void) {
    AlarmManager_StopBuzzer();
    editTarget = EDIT_NONE;
    UiController_RefreshDateTime();

    if (displayMode == DISPLAY_TIME) {
        displayMode = DISPLAY_DATE;
    } else if (displayMode == DISPLAY_DATE) {
        displayMode = DISPLAY_ENVIRONMENT;
    } else if (displayMode == DISPLAY_ENVIRONMENT) {
        displayMode = DISPLAY_ALARM;
    } else {
        displayMode = DISPLAY_TIME;
    }
}

static void HandleSetButton(void) {
    AlarmManager_StopBuzzer();
    if (displayMode == DISPLAY_TIME) {
        if ((editTarget == EDIT_NONE) || (editTarget == EDIT_HOURS)) {
            editTarget = EDIT_MINUTES;
        } else {
            editTarget = EDIT_HOURS;
        }
    } else if (displayMode == DISPLAY_DATE) {
        if ((editTarget == EDIT_NONE) || (editTarget == EDIT_MONTH)) {
            editTarget = EDIT_DAY;
        } else {
            editTarget = EDIT_MONTH;
        }
    } else if (displayMode == DISPLAY_ALARM) {
        if ((editTarget == EDIT_NONE) || (editTarget == EDIT_ALARM_MINUTES)) {
            editTarget = EDIT_ALARM_HOURS;
        } else {
            editTarget = EDIT_ALARM_MINUTES;
        }
    }
}

static void HandleUpButton(void) {
    AlarmManager_StopBuzzer();
    switch (editTarget) {
    case EDIT_MINUTES:
        currentDateTime.minutes = IncrementWrap(currentDateTime.minutes, 0U, 59U);
        currentDateTime.seconds = 0U;
        SaveTimeToRtc();
        break;
    case EDIT_HOURS:
        currentDateTime.hours = IncrementWrap(currentDateTime.hours, 0U, 23U);
        SaveTimeToRtc();
        break;
    case EDIT_DAY:
        currentDateTime.day =
            IncrementWrap(currentDateTime.day, 1U, MonthDays(currentDateTime.month));
        SaveDateToRtc();
        break;
    case EDIT_MONTH:
        currentDateTime.month = IncrementWrap(currentDateTime.month, 1U, 12U);
        if (currentDateTime.day > MonthDays(currentDateTime.month)) {
            currentDateTime.day = MonthDays(currentDateTime.month);
        }
        SaveDateToRtc();
        break;
    case EDIT_ALARM_HOURS:
        AlarmManager_IncrementSelectedHour();
        break;
    case EDIT_ALARM_MINUTES:
        AlarmManager_IncrementSelectedMinute();
        break;
    case EDIT_NONE:
    default:
        if (displayMode == DISPLAY_ALARM) {
            AlarmManager_SelectNextSlot();
        }
        break;
    }
}

static void HandleDownButton(void) {
    AlarmManager_StopBuzzer();
    switch (editTarget) {
    case EDIT_MINUTES:
        currentDateTime.minutes = DecrementWrap(currentDateTime.minutes, 0U, 59U);
        currentDateTime.seconds = 0U;
        SaveTimeToRtc();
        break;
    case EDIT_HOURS:
        currentDateTime.hours = DecrementWrap(currentDateTime.hours, 0U, 23U);
        SaveTimeToRtc();
        break;
    case EDIT_DAY:
        currentDateTime.day =
            DecrementWrap(currentDateTime.day, 1U, MonthDays(currentDateTime.month));
        SaveDateToRtc();
        break;
    case EDIT_MONTH:
        currentDateTime.month = DecrementWrap(currentDateTime.month, 1U, 12U);
        if (currentDateTime.day > MonthDays(currentDateTime.month)) {
            currentDateTime.day = MonthDays(currentDateTime.month);
        }
        SaveDateToRtc();
        break;
    case EDIT_ALARM_HOURS:
        AlarmManager_DecrementSelectedHour();
        break;
    case EDIT_ALARM_MINUTES:
        AlarmManager_DecrementSelectedMinute();
        break;
    case EDIT_NONE:
    default:
        if (displayMode == DISPLAY_ALARM) {
            AlarmManager_SelectPreviousSlot();
        }
        break;
    }
}

static void HandleOffButton(void) {
    AlarmManager_StopBuzzer();
    if (displayMode == DISPLAY_ALARM) {
        if (editTarget != EDIT_NONE) {
            AlarmManager_EnableSelectedSlot();
            editTarget = EDIT_NONE;
            return;
        }

        if (AlarmManager_DisableSelectedSlotIfEnabled() != 0U) {
            return;
        }
    }

    editTarget = EDIT_NONE;
    UiController_RefreshDateTime();
}

static void HandleButton(ClockButton_t button) {
    AutoModeScheduler_PauseForUserActivity();

    switch (button) {
    case CLOCK_BUTTON_MODE:
        HandleModeButton();
        break;
    case CLOCK_BUTTON_SET:
        HandleSetButton();
        break;
    case CLOCK_BUTTON_UP:
        HandleUpButton();
        break;
    case CLOCK_BUTTON_DOWN:
        HandleDownButton();
        break;
    case CLOCK_BUTTON_OFF:
        HandleOffButton();
        break;
    case CLOCK_BUTTON_NONE:
    default:
        break;
    }
}

void UiController_Init(void) {
    AutoModeScheduler_Reset();
    UiController_RefreshDateTime();
}

void UiController_UpdateButton(ClockButton_t pressedButton) {
    if (pressedButton == CLOCK_BUTTON_OFF) {
        if (offButtonHoldTicks < OFF_LONG_PRESS_TICKS) {
            ++offButtonHoldTicks;
        }
        if ((offButtonHoldTicks >= OFF_LONG_PRESS_TICKS) &&
            (offLongPressHandled == 0U)) {
            AutoModeScheduler_PauseForUserActivity();
            AlarmManager_DisableAll();
            editTarget = EDIT_NONE;
            offLongPressHandled = 1U;
        }
    } else {
        if ((lastButton == CLOCK_BUTTON_OFF) && (offLongPressHandled == 0U)) {
            HandleButton(CLOCK_BUTTON_OFF);
        }
        offButtonHoldTicks = 0U;
        offLongPressHandled = 0U;
    }

    if ((pressedButton != CLOCK_BUTTON_NONE) &&
        (pressedButton != CLOCK_BUTTON_OFF) && (pressedButton != lastButton)) {
        HandleButton(pressedButton);
    }
    lastButton = pressedButton;
}

void UiController_UpdateAutoModeCycle(void) {
    if (AutoModeScheduler_Update(&displayMode, &editTarget) != 0U) {
        UiController_RefreshDateTime();
    }
}

void UiController_RefreshDateTime(void) {
    Board_ReadDateTime(&currentDateTime);
    NormalizeDateTime();
}

DisplayMode_t UiController_DisplayMode(void) {
    return displayMode;
}

EditTarget_t UiController_EditTarget(void) {
    return editTarget;
}

const ClockDateTime_t *UiController_DateTime(void) {
    return &currentDateTime;
}
