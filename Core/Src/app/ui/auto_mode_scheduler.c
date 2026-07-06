#include "auto_mode_scheduler.h"

#include "app_config.h"

static uint32_t autoModeTicks = 0U;
static uint32_t userPauseTicks = 0U;

static uint32_t AutoModeDurationTicks(DisplayMode_t mode) {
    if (mode == DISPLAY_TIME) {
        return AUTO_TIME_TICKS;
    }
    if (mode == DISPLAY_DATE) {
        return AUTO_DATE_TICKS;
    }
    return AUTO_ENVIRONMENT_TICKS;
}

static void AdvanceAutoMode(DisplayMode_t *displayMode) {
    if (*displayMode == DISPLAY_TIME) {
        *displayMode = DISPLAY_DATE;
    } else if (*displayMode == DISPLAY_DATE) {
        *displayMode = DISPLAY_ENVIRONMENT;
    } else {
        *displayMode = DISPLAY_TIME;
    }
    autoModeTicks = 0U;
}

void AutoModeScheduler_Reset(void) {
    autoModeTicks = 0U;
    userPauseTicks = 0U;
}

void AutoModeScheduler_PauseForUserActivity(void) {
    userPauseTicks = AUTO_USER_PAUSE_TICKS;
}

uint8_t AutoModeScheduler_Update(DisplayMode_t *displayMode,
                                 EditTarget_t *editTarget) {
    if (userPauseTicks > 0U) {
        --userPauseTicks;
        if (userPauseTicks == 0U) {
            *editTarget = EDIT_NONE;
            *displayMode = DISPLAY_TIME;
            autoModeTicks = 0U;
            return 1U;
        }
        return 0U;
    }

    if (*editTarget != EDIT_NONE) {
        return 0U;
    }

    if (*displayMode == DISPLAY_ALARM) {
        return 0U;
    }

    ++autoModeTicks;
    if (autoModeTicks >= AutoModeDurationTicks(*displayMode)) {
        AdvanceAutoMode(displayMode);
    }

    return 0U;
}
