#ifndef LIGHT_SENSOR_H_
#define LIGHT_SENSOR_H_

#include <stdint.h>

void LightSensor_Init(void);
uint16_t LightSensor_ReadRaw(void);
uint8_t LightSensor_ReadBrightness(void);

#endif /* LIGHT_SENSOR_H_ */
