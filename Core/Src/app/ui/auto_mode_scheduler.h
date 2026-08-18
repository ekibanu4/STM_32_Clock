#ifndef AUTO_MODE_SCHEDULER_H_
#define AUTO_MODE_SCHEDULER_H_

#include <stdint.h>

#include "app_types.h"

void AutoModeScheduler_Reset(void);
void AutoModeScheduler_PauseForUserActivity(void);
void AutoModeScheduler_Update(DisplayMode_t *displayMode,
                              EditTarget_t *editTarget);

#endif /* AUTO_MODE_SCHEDULER_H_ */
