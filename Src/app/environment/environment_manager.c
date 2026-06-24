#include "environment_manager.h"

#include <stdint.h>

#include "app_config.h"
#include "board.h"

static ClockEnvironment_t currentEnvironment = {0U, 0U, 0U};
static uint32_t environmentReadTicks = 0U;

static void ReadEnvironmentSensor(void) {
    ClockEnvironment_t readEnvironment = currentEnvironment;

    if (Board_ReadEnvironment(&readEnvironment) != 0U) {
        currentEnvironment = readEnvironment;
    }

    environmentReadTicks = ENVIRONMENT_READ_TICKS;
}

void EnvironmentManager_Init(void) {
    ReadEnvironmentSensor();
}

void EnvironmentManager_Update(void) {
    if (environmentReadTicks > 0U) {
        --environmentReadTicks;
        return;
    }

    ReadEnvironmentSensor();
}

const ClockEnvironment_t *EnvironmentManager_Current(void) {
    return &currentEnvironment;
}
