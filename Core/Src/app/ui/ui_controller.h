#ifndef UI_CONTROLLER_H_
#define UI_CONTROLLER_H_

#include "app_types.h"

void UiController_Init(void);
void UiController_UpdateButton(ClockButton_t pressedButton, uint32_t nowMs);
void UiController_UpdateAutoModeCycle(void);
void UiController_RefreshDateTime(void);
void UiController_ShowTimeMode(void);

DisplayMode_t UiController_DisplayMode(void);
EditTarget_t UiController_EditTarget(void);
const ClockDateTime_t *UiController_DateTime(void);
uint8_t UiController_AlarmErrorActive(void);

#endif /* UI_CONTROLLER_H_ */
