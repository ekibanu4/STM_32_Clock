#include "buzzer.h"

#include "board_config.h"

void Buzzer_Init(void) {
  RCC_EnableGPIO(BUZZER_GPIO);

  GPIO_SetPinMode(BUZZER_GPIO, BUZZER_PIN, OUTPUT);
  GPIO_SetPinPull(BUZZER_GPIO, BUZZER_PIN, NO_PUDR);
  GPIO_SetPinOutputType(BUZZER_GPIO, BUZZER_PIN, PUSH_PULL);
  GPIO_SetPinSpeed(BUZZER_GPIO, BUZZER_PIN, LOW_SPEED);
  GPIO_ResetPin(BUZZER_GPIO, BUZZER_PIN);
}

void Buzzer_SetEnabled(uint8_t isEnabled) {
  GPIO_WritePin(BUZZER_GPIO, BUZZER_PIN, isEnabled ? ON : OFF);
}
