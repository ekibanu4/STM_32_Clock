#include "display_renderer.h"

#include "app_config.h"

static uint8_t HoursToBoard1Mask(uint8_t hours) {
    uint8_t mask = 0U;

    if ((hours & 32U) != 0U) {
        mask |= (uint8_t)(1U << CLOCK_BOARD1_HOURS_32_BIT);
    }
    if ((hours & 16U) != 0U) {
        mask |= (uint8_t)(1U << CLOCK_BOARD1_HOURS_16_BIT);
    }
    if ((hours & 8U) != 0U) {
        mask |= (uint8_t)(1U << CLOCK_BOARD1_HOURS_8_BIT);
    }
    if ((hours & 4U) != 0U) {
        mask |= (uint8_t)(1U << CLOCK_BOARD1_HOURS_4_BIT);
    }
    if ((hours & 2U) != 0U) {
        mask |= (uint8_t)(1U << CLOCK_BOARD1_HOURS_2_BIT);
    }
    if ((hours & 1U) != 0U) {
        mask |= (uint8_t)(1U << CLOCK_BOARD1_HOURS_1_BIT);
    }

    return mask;
}

static void AddMinutesToDisplay(uint8_t minutes, ClockDisplay_t *display) {
    if ((minutes & 32U) != 0U) {
        display->board1 |= (uint8_t)(1U << CLOCK_BOARD1_MINUTES_32_BIT);
    }
    if ((minutes & 16U) != 0U) {
        display->board1 |= (uint8_t)(1U << CLOCK_BOARD1_MINUTES_16_BIT);
    }
    if ((minutes & 8U) != 0U) {
        display->board2 |= (uint8_t)(1U << CLOCK_BOARD2_MINUTES_8_BIT);
    }
    if ((minutes & 4U) != 0U) {
        display->board2 |= (uint8_t)(1U << CLOCK_BOARD2_MINUTES_4_BIT);
    }
    if ((minutes & 2U) != 0U) {
        display->board2 |= (uint8_t)(1U << CLOCK_BOARD2_MINUTES_2_BIT);
    }
    if ((minutes & 1U) != 0U) {
        display->board2 |= (uint8_t)(1U << CLOCK_BOARD2_MINUTES_1_BIT);
    }
}

static void AddHumidityScaleLed(uint8_t ledIndex, ClockDisplay_t *display) {
    switch (ledIndex) {
    case 0U:
        display->board2 |= (uint8_t)(1U << CLOCK_BOARD2_MINUTES_1_BIT);
        break;
    case 1U:
        display->board2 |= (uint8_t)(1U << CLOCK_BOARD2_MINUTES_2_BIT);
        break;
    case 2U:
        display->board2 |= (uint8_t)(1U << CLOCK_BOARD2_MINUTES_4_BIT);
        break;
    case 3U:
        display->board2 |= (uint8_t)(1U << CLOCK_BOARD2_MINUTES_8_BIT);
        break;
    case 4U:
        display->board1 |= (uint8_t)(1U << CLOCK_BOARD1_MINUTES_16_BIT);
        break;
    case 5U:
        display->board1 |= (uint8_t)(1U << CLOCK_BOARD1_MINUTES_32_BIT);
        break;
    default:
        break;
    }
}

static void AddMinuteScaleLed(uint8_t ledIndex, ClockDisplay_t *display) {
    AddHumidityScaleLed(ledIndex, display);
}

static void AddHumidityToEnvironmentDisplay(uint8_t humidity,
                                            ClockDisplay_t *display) {
    uint8_t fullLedCount = 0U;

    if (humidity >= ENVIRONMENT_HUMIDITY_DISPLAY_MAX) {
        fullLedCount = ENVIRONMENT_HUMIDITY_LED_COUNT;
    } else if (humidity > ENVIRONMENT_HUMIDITY_DISPLAY_MIN) {
        uint8_t humidityOffset =
            (uint8_t)(humidity - ENVIRONMENT_HUMIDITY_DISPLAY_MIN);
        fullLedCount = (uint8_t)(humidityOffset / ENVIRONMENT_HUMIDITY_STEP);
    }

    for (uint8_t ledIndex = 0U; ledIndex < fullLedCount; ++ledIndex) {
        AddHumidityScaleLed(ledIndex, display);
    }
}

static uint8_t ModeToBoard2Mask(DisplayMode_t displayMode) {
    if (displayMode == DISPLAY_TIME) {
        return (uint8_t)(1U << CLOCK_BOARD2_MODE_TIME_BIT);
    }
    if (displayMode == DISPLAY_DATE) {
        return (uint8_t)(1U << CLOCK_BOARD2_MODE_DATE_BIT);
    }
    if (displayMode == DISPLAY_ENVIRONMENT) {
        return (uint8_t)(1U << CLOCK_BOARD2_MODE_ENVIRONMENT_BIT);
    }
    return 0U;
}

static void AddAlarmStatusToDisplay(DisplayMode_t displayMode,
                                    uint8_t anyAlarmEnabled,
                                    ClockDisplay_t *display) {
    if ((displayMode == DISPLAY_ALARM) || (anyAlarmEnabled != 0U)) {
        display->board2 |= (uint8_t)(1U << CLOCK_BOARD2_ALARM_BIT);
    }
}

static void AddSelectedAlarmIndicator(const AlarmSlot_t *slot,
                                      uint8_t selectedAlarmSlot,
                                      uint8_t showNumber,
                                      ClockDisplay_t *display) {
    uint8_t alarmNumber = (uint8_t)(selectedAlarmSlot + 1U);

    if (slot->enabled != 0U) {
        display->board2 |= (uint8_t)(1U << CLOCK_BOARD2_MODE_TIME_BIT);
    }
    if (showNumber != 0U) {
        if ((alarmNumber & 1U) != 0U) {
            display->board2 |=
                (uint8_t)(1U << CLOCK_BOARD2_MODE_ENVIRONMENT_BIT);
        }
        if ((alarmNumber & 2U) != 0U) {
            display->board2 |= (uint8_t)(1U << CLOCK_BOARD2_MODE_DATE_BIT);
        }
    }
}

static ClockDisplay_t BuildTimeDisplay(const ClockDateTime_t *dateTime,
                                       DisplayMode_t displayMode) {
    ClockDisplay_t display = {0U, ModeToBoard2Mask(displayMode)};

    display.board1 = HoursToBoard1Mask(dateTime->hours);
    AddMinutesToDisplay(dateTime->minutes, &display);
    return display;
}

static ClockDisplay_t BuildDateDisplay(const ClockDateTime_t *dateTime,
                                       DisplayMode_t displayMode) {
    ClockDisplay_t display = {0U, ModeToBoard2Mask(displayMode)};

    display.board1 = HoursToBoard1Mask(dateTime->month);
    AddMinutesToDisplay(dateTime->day, &display);
    return display;
}

static ClockDisplay_t
BuildEnvironmentDisplay(const ClockEnvironment_t *environment,
                        DisplayMode_t displayMode) {
    uint8_t displayTemperature = environment->temperature;
    uint8_t displayHumidity = environment->humidity;
    ClockDisplay_t display = {0U, ModeToBoard2Mask(displayMode)};

    if (displayTemperature > ENVIRONMENT_TEMPERATURE_DISPLAY_MAX) {
        displayTemperature = ENVIRONMENT_TEMPERATURE_DISPLAY_MAX;
    }
    if (displayHumidity > ENVIRONMENT_HUMIDITY_DISPLAY_MAX) {
        displayHumidity = ENVIRONMENT_HUMIDITY_DISPLAY_MAX;
    }

    display.board1 = HoursToBoard1Mask(displayTemperature);
    AddHumidityToEnvironmentDisplay(displayHumidity, &display);
    return display;
}

static ClockDisplay_t BuildAlarmDisplay(EditTarget_t editTarget,
                                        const AlarmSlot_t *alarmSlots,
                                        uint8_t selectedAlarmSlot,
                                        uint8_t alarmErrorActive,
                                        uint8_t blinkOn) {
    const AlarmSlot_t *slot = &alarmSlots[selectedAlarmSlot];
    ClockDisplay_t display = {0U, 0U};

    AddAlarmStatusToDisplay(DISPLAY_ALARM, 1U, &display);
    AddSelectedAlarmIndicator(slot, selectedAlarmSlot,
                              ((editTarget == EDIT_NONE) || (blinkOn != 0U))
                                  ? 1U
                                  : 0U,
                              &display);

    if ((alarmErrorActive != 0U) && (blinkOn != 0U)) {
        display.board1 = 0xFFU;
        display.board2 |= (uint8_t)((1U << CLOCK_BOARD2_MINUTES_8_BIT) |
                                    (1U << CLOCK_BOARD2_MINUTES_4_BIT) |
                                    (1U << CLOCK_BOARD2_MINUTES_2_BIT) |
                                    (1U << CLOCK_BOARD2_MINUTES_1_BIT));
        return display;
    }

    if (editTarget == EDIT_ALARM_HOURS) {
        display.board1 = HoursToBoard1Mask(slot->hour);
        return display;
    }

    if (editTarget == EDIT_ALARM_MINUTES) {
        AddMinutesToDisplay(slot->minute, &display);
        return display;
    }

    display.board1 = HoursToBoard1Mask(slot->hour);
    AddMinutesToDisplay(slot->minute, &display);
    return display;
}

static ClockDisplay_t BuildBatteryDisplay(uint16_t batteryPercentTenths) {
    ClockDisplay_t display = {0U, 0U};
    uint8_t fullLedCount =
        (uint8_t)((batteryPercentTenths + 166U) / 167U);

    if (fullLedCount > 6U) {
        fullLedCount = 6U;
    }

    for (uint8_t ledIndex = 0U; ledIndex < fullLedCount; ++ledIndex) {
        AddMinuteScaleLed(ledIndex, &display);
    }
    return display;
}

static ClockDisplay_t BuildEditDisplay(DisplayMode_t displayMode,
                                       EditTarget_t editTarget,
                                       const ClockDateTime_t *dateTime,
                                       uint8_t anyAlarmEnabled,
                                       uint8_t blinkOn) {
    ClockDisplay_t display = {0U, ModeToBoard2Mask(displayMode)};
    uint8_t modeBit = 0xFFU;

    AddAlarmStatusToDisplay(displayMode, anyAlarmEnabled, &display);

    if (displayMode == DISPLAY_TIME) {
        modeBit = CLOCK_BOARD2_MODE_TIME_BIT;
    } else if (displayMode == DISPLAY_DATE) {
        modeBit = CLOCK_BOARD2_MODE_DATE_BIT;
    }

    if ((modeBit != 0xFFU) && (blinkOn == 0U)) {
        display.board2 &= (uint8_t) ~(1U << modeBit);
    }

    switch (editTarget) {
    case EDIT_MINUTES:
        AddMinutesToDisplay(dateTime->minutes, &display);
        break;
    case EDIT_HOURS:
        display.board1 = HoursToBoard1Mask(dateTime->hours);
        break;
    case EDIT_DAY:
        AddMinutesToDisplay(dateTime->day, &display);
        break;
    case EDIT_MONTH:
        display.board1 = HoursToBoard1Mask(dateTime->month);
        break;
    case EDIT_YEAR:
        display.board1 = HoursToBoard1Mask(20U);
        AddMinutesToDisplay(dateTime->year, &display);
        break;
    case EDIT_NONE:
    default:
        break;
    }

    return display;
}

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
                                      uint8_t blinkOn) {
    ClockDisplay_t display = {0U, 0U};

    if (displayMode == DISPLAY_ALARM) {
        return BuildAlarmDisplay(editTarget, alarmSlots, selectedAlarmSlot,
                                 alarmErrorActive, blinkOn);
    }

    if (displayMode == DISPLAY_BATTERY) {
        return BuildBatteryDisplay(batteryPercentTenths);
    }

    if (editTarget != EDIT_NONE) {
        return BuildEditDisplay(displayMode, editTarget, dateTime,
                                anyAlarmEnabled, blinkOn);
    }

    if (displayMode == DISPLAY_TIME) {
        display = BuildTimeDisplay(dateTime, displayMode);
    } else if (displayMode == DISPLAY_DATE) {
        display = BuildDateDisplay(dateTime, displayMode);
    } else {
        display = BuildEnvironmentDisplay(environment, displayMode);
    }

    AddAlarmStatusToDisplay(displayMode, anyAlarmEnabled, &display);
    return display;
}
