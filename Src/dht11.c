#include "dht11.h"
#include "utils.h"

#define DHT11_CPU_CLOCK_HZ 16000000UL
#define DHT11_CYCLES_PER_US (DHT11_CPU_CLOCK_HZ / 1000000UL)
#define DHT11_MAX_CYCLES 300000UL
#define DHT11_TIMEOUT 0xFFFFFFFFUL
#define DHT11_PULL_TIME_US 55UL
#define DHT11_MIN_INTERVAL_US 2000000UL

#define CORE_DEBUG_DEMCR (*(volatile uint32_t *)0xE000EDFCUL)
#define DWT_CTRL (*(volatile uint32_t *)0xE0001000UL)
#define DWT_CYCCNT (*(volatile uint32_t *)0xE0001004UL)
#define CORE_DEBUG_DEMCR_TRCENA (1UL << 24)
#define DWT_CTRL_CYCCNTENA (1UL << 0)

volatile uint8_t dhtDebugStatus = DHT11_STATUS_IDLE;

static uint32_t lastReadCycles = 0U;
static uint8_t lastResult = 0U;

static void DWT_Init(void) {
  CORE_DEBUG_DEMCR |= CORE_DEBUG_DEMCR_TRCENA;
  DWT_CYCCNT = 0U;
  DWT_CTRL |= DWT_CTRL_CYCCNTENA;
}

static void DelayUs(uint32_t microseconds) {
  uint32_t startCycles = DWT_CYCCNT;
  uint32_t waitCycles = microseconds * DHT11_CYCLES_PER_US;

  while ((DWT_CYCCNT - startCycles) < waitCycles) {
  }
}

static void DHT11_PinInput(void) {
  GPIO_SetPinPull(DHT11_GPIO, DHT11_DATA_PIN, NO_PUDR);
  GPIO_SetPinMode(DHT11_GPIO, DHT11_DATA_PIN, INPUT);
}

static void DHT11_PinOutput(void) {
  GPIO_SetPinMode(DHT11_GPIO, DHT11_DATA_PIN, OUTPUT);
}

static uint32_t ExpectPulse(uint8_t level) {
  uint32_t count = 0U;

  while (GPIO_ReadPin(DHT11_GPIO, DHT11_DATA_PIN) == level) {
    if (count++ >= DHT11_MAX_CYCLES) {
      return DHT11_TIMEOUT;
    }
  }

  return count;
}

void DHT11_Init(void) {
  RCC_EnableGPIO(DHT11_GPIO);
  DWT_Init();

  GPIO_SetPinPull(DHT11_GPIO, DHT11_DATA_PIN, NO_PUDR);
  GPIO_SetPinOutputType(DHT11_GPIO, DHT11_DATA_PIN, OPEN_DRAIN);
  GPIO_SetPinSpeed(DHT11_GPIO, DHT11_DATA_PIN, MEDIUM_SPEED);

  DHT11_PinInput();
  lastReadCycles = DWT_CYCCNT - (DHT11_MIN_INTERVAL_US * DHT11_CYCLES_PER_US);
}

uint8_t DHT11_Read(DHT11_Reading_t *reading) {
  uint8_t data[5] = {0U, 0U, 0U, 0U, 0U};
  uint32_t cycles[80];
  uint32_t currentCycles = DWT_CYCCNT;

  if (((currentCycles - lastReadCycles) <
       (DHT11_MIN_INTERVAL_US * DHT11_CYCLES_PER_US)) &&
      (lastResult != 0U)) {
    return lastResult;
  }
  lastReadCycles = currentCycles;

  dhtDebugStatus = DHT11_STATUS_IDLE;

  DHT11_PinInput();
  DelayUs(1000U);

  DHT11_PinOutput();
  GPIO_ResetPin(DHT11_GPIO, DHT11_DATA_PIN);
  DelayUs(20000U);

  DHT11_PinInput();
  DelayUs(DHT11_PULL_TIME_US);

  DisableInterrupts();

  if (ExpectPulse(0U) == DHT11_TIMEOUT) {
    EnableInterrupts();
    reading->isValid = 0U;
    lastResult = 0U;
    dhtDebugStatus = DHT11_STATUS_ERROR;
    return 0U;
  }

  if (ExpectPulse(1U) == DHT11_TIMEOUT) {
    EnableInterrupts();
    reading->isValid = 0U;
    lastResult = 0U;
    dhtDebugStatus = DHT11_STATUS_ERROR;
    return 0U;
  }

  for (uint8_t pulseIndex = 0U; pulseIndex < 80U; pulseIndex += 2U) {
    cycles[pulseIndex] = ExpectPulse(0U);
    if (cycles[pulseIndex] == DHT11_TIMEOUT) {
      EnableInterrupts();
      reading->isValid = 0U;
      lastResult = 0U;
      dhtDebugStatus = DHT11_STATUS_ERROR;
      return 0U;
    }

    cycles[pulseIndex + 1U] = ExpectPulse(1U);
    if (cycles[pulseIndex + 1U] == DHT11_TIMEOUT) {
      EnableInterrupts();
      reading->isValid = 0U;
      lastResult = 0U;
      dhtDebugStatus = DHT11_STATUS_ERROR;
      return 0U;
    }
  }

  EnableInterrupts();

  for (uint8_t bitIndex = 0U; bitIndex < 40U; ++bitIndex) {
    uint32_t lowCycles = cycles[2U * bitIndex];
    uint32_t highCycles = cycles[(2U * bitIndex) + 1U];

    data[bitIndex / 8U] <<= 1U;
    if (highCycles > lowCycles) {
      data[bitIndex / 8U] |= 1U;
    }
  }

  uint8_t checksum =
      (uint8_t)(data[0] + data[1] + data[2] + data[3]);
  if (data[4] != checksum) {
    reading->isValid = 0U;
    lastResult = 0U;
    dhtDebugStatus = DHT11_STATUS_ERROR;
    return 0U;
  }

  reading->humidity = data[0];
  reading->temperature = data[2];
  reading->isValid = 1U;
  lastResult = 1U;
  dhtDebugStatus = DHT11_STATUS_OK;
  return 1U;
}
