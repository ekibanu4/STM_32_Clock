#include "light_sensor.h"

#include "board_config.h"
#include "buttons_decoder.h"

void LightSensor_Init(void) {
  ButtonsDecoder_Init();
}

uint16_t LightSensor_ReadRaw(void) {
  return ADC1_ReadChannel(LIGHT_SENSOR_ADC_CHANNEL);
}

uint8_t LightSensor_ReadBrightness(void) {
  static uint8_t currentLevel = 0U;
  uint16_t adcValue = LightSensor_ReadRaw();
  uint32_t adcRange = LIGHT_SENSOR_ADC_BRIGHT - LIGHT_SENSOR_ADC_DARK;
  uint8_t maxLevel = LIGHT_SENSOR_BRIGHTNESS_LEVELS - 1U;
  uint8_t nextLevel = currentLevel;

  if (adcValue <= LIGHT_SENSOR_ADC_DARK) {
    nextLevel = 0U;
  } else if (adcValue >= LIGHT_SENSOR_ADC_BRIGHT) {
    nextLevel = maxLevel;
  } else {
    uint32_t adcOffset = adcValue - LIGHT_SENSOR_ADC_DARK;
    nextLevel =
        (uint8_t)((adcOffset * maxLevel + (adcRange / 2U)) / adcRange);
  }

  if (nextLevel > currentLevel) {
    uint32_t upThreshold =
        LIGHT_SENSOR_ADC_DARK + ((uint32_t)(currentLevel + 1U) * adcRange) / maxLevel;
    if (adcValue >= (upThreshold + LIGHT_SENSOR_ADC_HYSTERESIS)) {
      currentLevel = nextLevel;
    }
  } else if (nextLevel < currentLevel) {
    uint32_t downThreshold =
        LIGHT_SENSOR_ADC_DARK + ((uint32_t)currentLevel * adcRange) / maxLevel;
    if ((adcValue + LIGHT_SENSOR_ADC_HYSTERESIS) <= downThreshold) {
      currentLevel = nextLevel;
    }
  }

  uint32_t brightnessRange =
      LIGHT_SENSOR_BRIGHTNESS_MAX - LIGHT_SENSOR_BRIGHTNESS_MIN;

  return (uint8_t)(LIGHT_SENSOR_BRIGHTNESS_MIN +
                   (((uint32_t)currentLevel * brightnessRange) / maxLevel));
}
