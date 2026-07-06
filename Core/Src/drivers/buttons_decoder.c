#include "buttons_decoder.h"

#include "board_config.h"

void ButtonsDecoder_Init(void) {
  RCC_EnableGPIO(BUTTONS_GPIO);
  RCC->APB2ENR |= (1U << 8);
  (void)RCC->APB2ENR;

  GPIO_SetPinMode(BUTTONS_GPIO, BUTTONS_PIN, ANALOG);
  GPIO_SetPinPull(BUTTONS_GPIO, BUTTONS_PIN, NO_PUDR);

  GPIO_SetPinMode(LIGHT_SENSOR_GPIO, LIGHT_SENSOR_PIN, ANALOG);
  GPIO_SetPinPull(LIGHT_SENSOR_GPIO, LIGHT_SENSOR_PIN, NO_PUDR);

  ADC_COMMON->CCR = 0U;
  ADC1->CR1 = 0U;
  ADC1->CR2 = 0U;

  ADC1->SMPR2 &= ~(0x7UL << (BUTTONS_ADC_CHANNEL * 3U));
  ADC1->SMPR2 |= (0x7UL << (BUTTONS_ADC_CHANNEL * 3U));
  ADC1->SMPR2 &= ~(0x7UL << (LIGHT_SENSOR_ADC_CHANNEL * 3U));
  ADC1->SMPR2 |= (0x7UL << (LIGHT_SENSOR_ADC_CHANNEL * 3U));
  ADC1->SQR1 = 0U;

  ADC1->CR2 |= ADC_CR2_EOCS;
  ADC1->CR2 |= ADC_CR2_ADON;
  (void)ADC1->CR2;
  DelayCycles(10000U);

  (void)ADC1_ReadChannel(BUTTONS_ADC_CHANNEL);
}

uint16_t ButtonsDecoder_ReadRaw(void) {
  return ADC1_ReadChannel(BUTTONS_ADC_CHANNEL);
}

ButtonCode_t ButtonsDecoder_Decode(uint16_t adcValue) {
  if (adcValue <= BUTTON_MODE_ADC_MAX) {
    return BUTTON_MODE;
  }
  if (adcValue <= BUTTON_SET_ADC_MAX) {
    return BUTTON_SET;
  }
  if (adcValue <= BUTTON_UP_ADC_MAX) {
    return BUTTON_UP;
  }
  if (adcValue <= BUTTON_DOWN_ADC_MAX) {
    return BUTTON_DOWN;
  }
  if (adcValue <= BUTTON_OFF_ADC_MAX) {
    return BUTTON_OFF;
  }
  return BUTTON_NONE;
}

ButtonCode_t ButtonsDecoder_Read(void) {
  return ButtonsDecoder_Decode(ButtonsDecoder_ReadRaw());
}
