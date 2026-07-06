#ifndef UI_CONTROLLER_H_
#define UI_CONTROLLER_H_

#include "app_types.h"

void UiController_Init(void);
void UiController_UpdateButton(ClockButton_t pressedButton);
void UiController_UpdateAutoModeCycle(void);
void UiController_RefreshDateTime(void);

DisplayMode_t UiController_DisplayMode(void);
EditTarget_t UiController_EditTarget(void);
const ClockDateTime_t *UiController_DateTime(void);

#endif /* UI_CONTROLLER_H_ */
