#include "display_renderer.h"

#include "app_config.h"

#define MATRIX_HUMIDITY_DISPLAY_MIN 20U
#define MATRIX_HUMIDITY_DISPLAY_MAX 100U
#define MATRIX_MARKER_BIT 0x80U

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

static uint8_t ValueToMatrixRow(uint8_t value) {
    uint8_t row = 0U;

    for (uint8_t bit = 0U; bit < 7U; ++bit) {
        if ((value & (uint8_t)(1U << bit)) != 0U) {
            row |= (uint8_t)(1U << bit);
        }
    }

    return row;
}

static uint8_t ScaleToMatrixRow(uint8_t value, uint8_t maxValue) {
    uint8_t litCount;
    uint8_t row = 0U;

    if (maxValue == 0U) {
        return 0U;
    }

    if (value >= maxValue) {
        litCount = 8U;
    } else {
        litCount = (uint8_t)(((uint16_t)value * 8U) / maxValue);
    }

    for (uint8_t column = 0U; column < litCount; ++column) {
        row |= (uint8_t)(1U << column);
    }

    return row;
}

static uint8_t HumidityToMatrixRow(uint8_t humidity) {
    if (humidity <= MATRIX_HUMIDITY_DISPLAY_MIN) {
        return 0U;
    }
    if (humidity >= MATRIX_HUMIDITY_DISPLAY_MAX) {
        return 0xFFU;
    }
    return ScaleToMatrixRow((uint8_t)(humidity - MATRIX_HUMIDITY_DISPLAY_MIN),
                            (uint8_t)(MATRIX_HUMIDITY_DISPLAY_MAX -
                                      MATRIX_HUMIDITY_DISPLAY_MIN));
}

static void AddMatrixStatus(DisplayMode_t displayMode, uint8_t modeFocusActive,
                            ClockDisplay_t *display) {
    if (modeFocusActive == 0U) {
        return;
    }

    if (displayMode == DISPLAY_TIME) {
        display->rows[0] |= MATRIX_MARKER_BIT;
    } else if (displayMode == DISPLAY_DATE) {
        display->rows[3] |= MATRIX_MARKER_BIT;
    } else if (displayMode == DISPLAY_ENVIRONMENT) {
        display->rows[6] |= MATRIX_MARKER_BIT;
    }
}

static void BuildMatrixOverview(DisplayMode_t displayMode,
                                EditTarget_t editTarget,
                                const ClockDateTime_t *dateTime,
                                const ClockEnvironment_t *environment,
                                uint8_t anyAlarmEnabled, uint8_t lightLevel,
                                uint8_t modeFocusActive,
                                uint8_t blinkOn, ClockDisplay_t *display) {
    uint8_t temperature = environment->temperature;
    uint8_t humidity = environment->humidity;

    if (temperature > ENVIRONMENT_TEMPERATURE_DISPLAY_MAX) {
        temperature = ENVIRONMENT_TEMPERATURE_DISPLAY_MAX;
    }
    (void)lightLevel;

    display->rows[0] = ValueToMatrixRow(dateTime->hours);
    display->rows[1] = ValueToMatrixRow(dateTime->minutes);
    display->rows[2] = ValueToMatrixRow(dateTime->seconds);
    display->rows[3] = ValueToMatrixRow(dateTime->day);
    display->rows[4] = ValueToMatrixRow(dateTime->month);
    display->rows[5] = ValueToMatrixRow(dateTime->year);
    display->rows[6] = ValueToMatrixRow(temperature);
    display->rows[7] = HumidityToMatrixRow(humidity);

    if (editTarget != EDIT_NONE) {
        uint8_t markerRow = 0xFFU;

        switch (editTarget) {
        case EDIT_HOURS:
            markerRow = 0U;
            break;
        case EDIT_MINUTES:
            markerRow = 1U;
            break;
        case EDIT_MONTH:
            markerRow = 4U;
            break;
        case EDIT_YEAR:
            markerRow = 5U;
            break;
        case EDIT_DAY:
            markerRow = 3U;
            break;
        default:
            break;
        }

        if (markerRow < 8U) {
            if (blinkOn != 0U) {
                display->rows[markerRow] |= MATRIX_MARKER_BIT;
            } else {
                display->rows[markerRow] &= (uint8_t)~MATRIX_MARKER_BIT;
            }
        }
    }

    (void)anyAlarmEnabled;
    AddMatrixStatus(displayMode, modeFocusActive, display);
}

static void AddMatrixAlarmSlot(const AlarmSlot_t *slot, uint8_t slotIndex,
                               uint8_t selectedAlarmSlot,
                               EditTarget_t editTarget, uint8_t blinkOn,
                               ClockDisplay_t *display) {
    uint8_t hourRowIndex = (uint8_t)(slotIndex * 2U);
    uint8_t minuteRowIndex = (uint8_t)(hourRowIndex + 1U);
    uint8_t hourRow = ValueToMatrixRow(slot->hour);
    uint8_t minuteRow = ValueToMatrixRow(slot->minute);
    uint8_t isSelected = (slotIndex == selectedAlarmSlot) ? 1U : 0U;

    if (slot->enabled != 0U) {
        hourRow |= MATRIX_MARKER_BIT;
    }

    if (isSelected != 0U) {
        minuteRow |= MATRIX_MARKER_BIT;
    }

    if ((isSelected != 0U) && (editTarget == EDIT_ALARM_HOURS)) {
        hourRow &= (uint8_t)~MATRIX_MARKER_BIT;
        hourRow |= (blinkOn != 0U) ? MATRIX_MARKER_BIT : 0U;
    }
    if ((isSelected != 0U) && (editTarget == EDIT_ALARM_MINUTES)) {
        minuteRow &= (uint8_t)~MATRIX_MARKER_BIT;
        minuteRow |= (blinkOn != 0U) ? MATRIX_MARKER_BIT : 0U;
    }

    display->rows[hourRowIndex] = hourRow;
    display->rows[minuteRowIndex] = minuteRow;
}

static void BuildMatrixAlarmDisplay(const AlarmSlot_t *alarmSlots,
                                    uint8_t alarmSlotCount,
                                    uint8_t selectedAlarmSlot,
                                    EditTarget_t editTarget,
                                    uint8_t alarmErrorActive,
                                    uint8_t blinkOn,
                                    ClockDisplay_t *display) {
    if ((alarmErrorActive != 0U) && (blinkOn != 0U)) {
        for (uint8_t row = 0U; row < 8U; ++row) {
            display->rows[row] = 0xFFU;
        }
        return;
    }

    for (uint8_t slotIndex = 0U;
         (slotIndex < alarmSlotCount) && (slotIndex < 3U); ++slotIndex) {
        AddMatrixAlarmSlot(&alarmSlots[slotIndex], slotIndex,
                           selectedAlarmSlot, editTarget, blinkOn, display);
    }

    (void)selectedAlarmSlot;
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

static uint8_t AlarmSlotLedBit(uint8_t slotIndex) {
    switch (slotIndex) {
    case 0U:
        return CLOCK_BOARD2_MODE_TIME_BIT;
    case 1U:
        return CLOCK_BOARD2_MODE_DATE_BIT;
    case 2U:
    default:
        return CLOCK_BOARD2_MODE_ENVIRONMENT_BIT;
    }
}

static void AddAlarmStatusToDisplay(DisplayMode_t displayMode,
                                    uint8_t anyAlarmEnabled,
                                    ClockDisplay_t *display) {
    if ((displayMode == DISPLAY_ALARM) || (anyAlarmEnabled != 0U)) {
        display->board2 |= (uint8_t)(1U << CLOCK_BOARD2_ALARM_BIT);
    }
}

static void AddAlarmSlotsToDisplay(const AlarmSlot_t *alarmSlots,
                                   uint8_t alarmSlotCount,
                                   uint8_t selectedAlarmSlot, uint8_t blinkOn,
                                   ClockDisplay_t *display) {
    for (uint8_t slotIndex = 0U; slotIndex < alarmSlotCount; ++slotIndex) {
        uint8_t bitMask = (uint8_t)(1U << AlarmSlotLedBit(slotIndex));
        uint8_t isSelected = (slotIndex == selectedAlarmSlot);
        uint8_t isEnabled = alarmSlots[slotIndex].enabled;
        uint8_t isConfigured = alarmSlots[slotIndex].configured;

        if (isEnabled != 0U) {
            if ((isSelected == 0U) || (blinkOn != 0U)) {
                display->board2 |= bitMask;
            }
        } else if ((isConfigured != 0U) && (isSelected != 0U) &&
                   (blinkOn != 0U)) {
            display->board2 |= bitMask;
        } else if ((isConfigured == 0U) && (isSelected != 0U) &&
                   (blinkOn != 0U)) {
            display->board2 |= bitMask;
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
                                        uint8_t alarmSlotCount,
                                        uint8_t selectedAlarmSlot,
                                        uint8_t alarmErrorActive,
                                        uint8_t blinkOn) {
    const AlarmSlot_t *slot = &alarmSlots[selectedAlarmSlot];
    ClockDisplay_t display = {0U, 0U};

    AddAlarmStatusToDisplay(DISPLAY_ALARM, 1U, &display);
    AddAlarmSlotsToDisplay(alarmSlots, alarmSlotCount, selectedAlarmSlot,
                           blinkOn, &display);

    if ((alarmErrorActive != 0U) && (blinkOn != 0U)) {
        display.board1 = 0xFFU;
        display.board2 |= (uint8_t)((1U << CLOCK_BOARD2_MINUTES_8_BIT) |
                                    (1U << CLOCK_BOARD2_MINUTES_4_BIT) |
                                    (1U << CLOCK_BOARD2_MINUTES_2_BIT) |
                                    (1U << CLOCK_BOARD2_MINUTES_1_BIT));
        return display;
    }

    if (editTarget == EDIT_ALARM_HOURS) {
        if (blinkOn != 0U) {
            display.board1 = HoursToBoard1Mask(slot->hour);
        }
        AddMinutesToDisplay(slot->minute, &display);
        return display;
    }

    if (editTarget == EDIT_ALARM_MINUTES) {
        display.board1 = HoursToBoard1Mask(slot->hour);
        if (blinkOn != 0U) {
            AddMinutesToDisplay(slot->minute, &display);
        }
        return display;
    }

    display.board1 = HoursToBoard1Mask(slot->hour);
    AddMinutesToDisplay(slot->minute, &display);
    return display;
}

static ClockDisplay_t BuildEditDisplay(DisplayMode_t displayMode,
                                       EditTarget_t editTarget,
                                       const ClockDateTime_t *dateTime,
                                       uint8_t anyAlarmEnabled,
                                       uint8_t blinkOn) {
    ClockDisplay_t display = {0U, ModeToBoard2Mask(displayMode)};

    AddAlarmStatusToDisplay(displayMode, anyAlarmEnabled, &display);

    if (blinkOn == 0U) {
        return display;
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
        display.board1 = HoursToBoard1Mask(dateTime->year);
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
                                      uint8_t anyAlarmEnabled,
                                      uint8_t alarmErrorActive,
                                      uint8_t lightLevel,
                                      uint8_t modeFocusActive,
                                      uint8_t blinkOn) {
    ClockDisplay_t display = {0U, 0U};

    if (displayMode == DISPLAY_ALARM) {
        display = BuildAlarmDisplay(editTarget, alarmSlots, alarmSlotCount,
                                    selectedAlarmSlot, alarmErrorActive,
                                    blinkOn);
    } else if (editTarget != EDIT_NONE) {
        display = BuildEditDisplay(displayMode, editTarget, dateTime,
                                   anyAlarmEnabled, blinkOn);
    } else if (displayMode == DISPLAY_TIME) {
        display = BuildTimeDisplay(dateTime, displayMode);
    } else if (displayMode == DISPLAY_DATE) {
        display = BuildDateDisplay(dateTime, displayMode);
    } else {
        display = BuildEnvironmentDisplay(environment, displayMode);
    }

    AddAlarmStatusToDisplay(displayMode, anyAlarmEnabled, &display);
    if (displayMode == DISPLAY_ALARM) {
        BuildMatrixAlarmDisplay(alarmSlots, alarmSlotCount, selectedAlarmSlot,
                                editTarget, alarmErrorActive, blinkOn,
                                &display);
    } else {
        BuildMatrixOverview(displayMode, editTarget, dateTime, environment,
                            anyAlarmEnabled, lightLevel, modeFocusActive,
                            blinkOn, &display);
    }
    return display;
}
