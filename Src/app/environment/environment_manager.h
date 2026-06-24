#ifndef ENVIRONMENT_MANAGER_H_
#define ENVIRONMENT_MANAGER_H_

#include "app_types.h"

void EnvironmentManager_Init(void);
void EnvironmentManager_Update(void);
const ClockEnvironment_t *EnvironmentManager_Current(void);

#endif /* ENVIRONMENT_MANAGER_H_ */
