#ifndef DHT11_H_
#define DHT11_H_

#include <stdint.h>

typedef struct {
  uint8_t temperature;
  uint8_t humidity;
  uint8_t isValid;
} DHT11_Reading_t;

typedef enum {
  DHT11_STATUS_IDLE = 0,
  DHT11_STATUS_ERROR = 1,
  DHT11_STATUS_OK = 2
} DHT11_DebugStatus_t;

extern volatile uint8_t dhtDebugStatus;

void DHT11_Init(void);
uint8_t DHT11_Read(DHT11_Reading_t *reading);

#endif /* DHT11_H_ */
