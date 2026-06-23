/*
 * utils.h
 *
 *  Created on: Jun 11, 2026
 *      Author: oleg
 */

 // 0x4002 3800 AHB1 RCC START
// 0X30  + RCC BASE = RCC_AHB1ENR
/*
0x40023800 - 0x4002 3BFF RCC
0x40022800 - 0x4002 2BFF GPIOK
0x40022400 - 0x4002 27FF GPIOJ
0x40022000 - 0x4002 23FF GPIOI
0x40021C00 - 0x4002 1FFF GPIOH
0x40021800 - 0x4002 1BFF GPIOG
0x40021400 - 0x4002 17FF GPIOF
0x40021000 - 0x4002 13FF GPIOE
0x40020C00 - 0x4002 0FFF GPIOD
0x40020800 - 0x4002 0BFF GPIOC
0x40020400 - 0x4002 07FF GPIOB
0x40020000 - 0x4002 03FF GPIOA*/


/*GPIO port mode register (GPIOx_MODER) 
Address offset: 0x00*

00: Input (reset state)
01: General purpose output mode
10: Alternate function mode
11: Analog mode

-----------

GPIO port output speed register (GPIOx_OSPEEDR)
Address offset: 0x08 

00: Low speed
01: Medium speed
10: High speed
11: Very high speed

---------

GPIO port pull-up/pull-down register (GPIOx_PUPDR)
(x = A..I/J/K)
Address offset: 0x0C

00: No pull-up, pull-down
01: Pull-up
10: Pull-down
11: Reserved

--------

//read
GPIO port input data register (GPIOx_IDR) (x = A..I/J/K)
Address offset: 0x10


GPIO port output data register (GPIOx_ODR) (x = A..I/J/K)
Address offset: 0x14

*/


#ifndef UTILS_H_
#define UTILS_H_

#include <stdint.h>

#define PERIPH_BASE_ADDR 0x40000000UL
#define AHB1PERIPH_BASE_ADDR 0x40020000UL
#define APB2PERIPH_BASE_ADDR 0x40010000UL

#define GPIOA_BASE_ADDR 0x40020000UL
#define GPIOB_BASE_ADDR 0x40020400UL
#define GPIOC_BASE_ADDR 0x40020800UL
#define GPIOD_BASE_ADDR 0x40020C00UL
#define GPIOE_BASE_ADDR 0x40021000UL
#define GPIOF_BASE_ADDR 0x40021400UL
#define GPIOG_BASE_ADDR 0x40021800UL
#define GPIOH_BASE_ADDR 0x40021C00UL
#define GPIOI_BASE_ADDR 0x40022000UL
#define GPIOJ_BASE_ADDR 0x40022400UL
#define GPIOK_BASE_ADDR 0x40022800UL
#define RCC_BASE_ADDR 0x40023800UL
#define TIM3_BASE_ADDR 0x40000400UL
#define ADC1_BASE_ADDR 0x40012000UL
#define ADC_COMMON_BASE_ADDR 0x40012300UL

#define PIN_0 0U
#define PIN_1 1U
#define PIN_2 2U
#define PIN_3 3U
#define PIN_4 4U
#define PIN_5 5U
#define PIN_6 6U
#define PIN_7 7U
#define PIN_8 8U
#define PIN_9 9U
#define PIN_10 10U
#define PIN_11 11U
#define PIN_12 12U
#define PIN_13 13U
#define PIN_14 14U
#define PIN_15 15U

#define SHIFT_REGISTER_GPIO GPIOA
#define SHIFT_REGISTER_DATA_PIN PIN_2
#define SHIFT_REGISTER_CLOCK_PIN PIN_4
#define SHIFT_REGISTER_LATCH_PIN PIN_3
#define SHIFT_REGISTER_OUTPUT_ENABLE_PIN PIN_6

#define SHIFT_REGISTER_Q0_BIT 0U
#define SHIFT_REGISTER_Q1_BIT 1U
#define SHIFT_REGISTER_Q2_BIT 2U
#define SHIFT_REGISTER_Q3_BIT 3U
#define SHIFT_REGISTER_Q4_BIT 4U
#define SHIFT_REGISTER_Q5_BIT 5U
#define SHIFT_REGISTER_Q6_BIT 6U
#define SHIFT_REGISTER_Q7_BIT 7U

#define BOARD1_HOURS_32_BIT SHIFT_REGISTER_Q0_BIT
#define BOARD1_HOURS_16_BIT SHIFT_REGISTER_Q1_BIT
#define BOARD1_HOURS_8_BIT SHIFT_REGISTER_Q2_BIT
#define BOARD1_HOURS_4_BIT SHIFT_REGISTER_Q3_BIT
#define BOARD1_HOURS_2_BIT SHIFT_REGISTER_Q4_BIT
#define BOARD1_HOURS_1_BIT SHIFT_REGISTER_Q5_BIT
#define BOARD1_MINUTES_32_BIT SHIFT_REGISTER_Q6_BIT
#define BOARD1_MINUTES_16_BIT SHIFT_REGISTER_Q7_BIT

#define BOARD2_MINUTES_8_BIT SHIFT_REGISTER_Q0_BIT
#define BOARD2_MINUTES_4_BIT SHIFT_REGISTER_Q1_BIT
#define BOARD2_MINUTES_2_BIT SHIFT_REGISTER_Q2_BIT
#define BOARD2_MINUTES_1_BIT SHIFT_REGISTER_Q3_BIT
#define BOARD2_MODE_HOURS_BIT SHIFT_REGISTER_Q4_BIT
#define BOARD2_MODE_DATE_BIT SHIFT_REGISTER_Q5_BIT
#define BOARD2_MODE_ENVIRONMENT_BIT SHIFT_REGISTER_Q6_BIT
#define BOARD2_ALARM_SET_BIT SHIFT_REGISTER_Q7_BIT

#define BUTTONS_GPIO GPIOA
#define BUTTONS_PIN PIN_1
#define BUTTONS_ADC_CHANNEL 1U

#define LIGHT_SENSOR_GPIO GPIOA
#define LIGHT_SENSOR_PIN PIN_0
#define LIGHT_SENSOR_ADC_CHANNEL 0U

#define BUTTON_MODE_ADC_MAX 520U
#define BUTTON_SET_ADC_MAX 990U
#define BUTTON_UP_ADC_MAX 1670U
#define BUTTON_DOWN_ADC_MAX 2380U
#define BUTTON_OFF_ADC_MAX 3410U

#define LIGHT_SENSOR_ADC_DARK 1000U
#define LIGHT_SENSOR_ADC_BRIGHT 3200U
#define LIGHT_SENSOR_BRIGHTNESS_MIN 5U
#define LIGHT_SENSOR_BRIGHTNESS_MAX 70U
#define LIGHT_SENSOR_BRIGHTNESS_LEVELS 6U
#define LIGHT_SENSOR_ADC_HYSTERESIS 80U

#define DS1302_GPIO GPIOA
#define DS1302_CLK_PIN PIN_5
#define DS1302_DATA_PIN PIN_7
#define DS1302_RST_PIN PIN_8

#define DHT11_GPIO GPIOA
#define DHT11_DATA_PIN PIN_10

#define BUZZER_GPIO GPIOA
#define BUZZER_PIN PIN_15

#define DS1302_SECONDS_WRITE 0x80U
#define DS1302_SECONDS_READ 0x81U
#define DS1302_MINUTES_WRITE 0x82U
#define DS1302_MINUTES_READ 0x83U
#define DS1302_HOURS_WRITE 0x84U
#define DS1302_HOURS_READ 0x85U
#define DS1302_DATE_WRITE 0x86U
#define DS1302_DATE_READ 0x87U
#define DS1302_MONTH_WRITE 0x88U
#define DS1302_MONTH_READ 0x89U
#define DS1302_CONTROL_WRITE 0x8EU
#define DS1302_RAM_WRITE_BASE 0xC0U
#define DS1302_RAM_READ_BASE 0xC1U
#define DS1302_WRITE_PROTECT_DISABLE 0x00U

#define ADC_SR_EOC (1UL << 1)
#define ADC_CR2_ADON (1UL << 0)
#define ADC_CR2_EOCS (1UL << 10)
#define ADC_CR2_SWSTART (1UL << 30)

#define BRIGHTNESS_100 100U
#define BRIGHTNESS_80 80U
#define BRIGHTNESS_50 50U
#define BRIGHTNESS_30 30U
#define BRIGHTNESS_10 10U
#define PWM_MAX_DUTY 100U
#define GPIO_AF_TIM3 2U

typedef struct {
  volatile uint32_t GPIOA_EN : 1; // bit 0
  volatile uint32_t GPIOB_EN : 1; // bit 1
  volatile uint32_t GPIOC_EN : 1; // bit 2
  volatile uint32_t GPIOD_EN : 1; // bit 3
  volatile uint32_t GPIOE_EN : 1; // bit 4
  volatile uint32_t GPIOF_EN : 1; // bit 5
  volatile uint32_t GPIOG_EN : 1; // bit 6
  volatile uint32_t GPIOH_EN : 1; // bit 7
  volatile uint32_t GPIOI_EN : 1; // bit 8

  volatile uint32_t RESERVED0 : 3; // 9-11

  volatile uint32_t CRCEN : 1; // bit 12

  volatile uint32_t RESERVED1 : 5; // 13-17

  volatile uint32_t BKPSRAMEN : 1; // bit 18

  volatile uint32_t RESERVED2 : 1; // 19

  volatile uint32_t CCMDATARAMEN : 1; // 20
  volatile uint32_t DMA1_EN : 1;      // bit 21
  volatile uint32_t DMA2_EN : 1;      // bit 22

  volatile uint32_t RESERVED3 : 2; // 23-24

  volatile uint32_t ETHMAC_EN : 1;   // bit 25
  volatile uint32_t ETHMACTX_EN : 1; // bit 26
  volatile uint32_t ETHMACRX_EN : 1; // bit 27

  volatile uint32_t ETHMACPTP_EN : 1; // bit 28

  volatile uint32_t OTGHS_EN : 1;     // bit 29
  volatile uint32_t OTGHSULPI_EN : 1; // bit 30

  volatile uint32_t RESERVED4 : 1; // bit 31

} RCC_AHB1ENR_t;

typedef struct {
  volatile uint32_t pin_0 : 2;
  volatile uint32_t pin_1 : 2;
  volatile uint32_t pin_2 : 2;
  volatile uint32_t pin_3 : 2;
  volatile uint32_t pin_4 : 2;
  volatile uint32_t pin_5 : 2;
  volatile uint32_t pin_6 : 2;
  volatile uint32_t pin_7 : 2;
  volatile uint32_t pin_8 : 2;
  volatile uint32_t pin_9 : 2;
  volatile uint32_t pin_10 : 2;
  volatile uint32_t pin_11 : 2;
  volatile uint32_t pin_12 : 2;
  volatile uint32_t pin_13 : 2;
  volatile uint32_t pin_14 : 2;
  volatile uint32_t pin_15 : 2;
} GPIOx_MODER;

typedef struct {
  volatile uint32_t pin_0 : 1;
  volatile uint32_t pin_1 : 1;
  volatile uint32_t pin_2 : 1;
  volatile uint32_t pin_3 : 1;
  volatile uint32_t pin_4 : 1;
  volatile uint32_t pin_5 : 1;
  volatile uint32_t pin_6 : 1;
  volatile uint32_t pin_7 : 1;
  volatile uint32_t pin_8 : 1;
  volatile uint32_t pin_9 : 1;
  volatile uint32_t pin_10 : 1;
  volatile uint32_t pin_11 : 1;
  volatile uint32_t pin_12 : 1;
  volatile uint32_t pin_13 : 1;
  volatile uint32_t pin_14 : 1;
  volatile uint32_t pin_15 : 1;
  volatile uint32_t reserved : 16;
} GPIOx_IO_STATE;

typedef struct {
  volatile uint32_t MODER;
  volatile uint32_t OTYPER;
  volatile uint32_t OSPEEDR;
  volatile uint32_t PUPDR;
  volatile uint32_t IDR;
  volatile uint32_t ODR;
  volatile uint32_t BSRR;
  volatile uint32_t LCKR;
  volatile uint32_t AFRL;
  volatile uint32_t AFRH;
} GPIO_RegDef_t;

typedef struct {
  volatile uint32_t CR;
  volatile uint32_t PLLCFGR;
  volatile uint32_t CFGR;
  volatile uint32_t CIR;
  volatile uint32_t AHB1RSTR;
  volatile uint32_t AHB2RSTR;
  volatile uint32_t AHB3RSTR;
  volatile uint32_t RESERVED0;
  volatile uint32_t APB1RSTR;
  volatile uint32_t APB2RSTR;
  volatile uint32_t RESERVED1[2];
  volatile uint32_t AHB1ENR;
  volatile uint32_t AHB2ENR;
  volatile uint32_t AHB3ENR;
  volatile uint32_t RESERVED2;
  volatile uint32_t APB1ENR;
  volatile uint32_t APB2ENR;
} RCC_RegDef_t;

typedef struct {
  volatile uint32_t SR;
  volatile uint32_t CR1;
  volatile uint32_t CR2;
  volatile uint32_t SMPR1;
  volatile uint32_t SMPR2;
  volatile uint32_t JOFR1;
  volatile uint32_t JOFR2;
  volatile uint32_t JOFR3;
  volatile uint32_t JOFR4;
  volatile uint32_t HTR;
  volatile uint32_t LTR;
  volatile uint32_t SQR1;
  volatile uint32_t SQR2;
  volatile uint32_t SQR3;
  volatile uint32_t JSQR;
  volatile uint32_t JDR1;
  volatile uint32_t JDR2;
  volatile uint32_t JDR3;
  volatile uint32_t JDR4;
  volatile uint32_t DR;
} ADC_RegDef_t;

typedef struct {
  volatile uint32_t CSR;
  volatile uint32_t CCR;
  volatile uint32_t CDR;
} ADC_CommonRegDef_t;

typedef struct {
  volatile uint32_t CR1;
  volatile uint32_t CR2;
  volatile uint32_t SMCR;
  volatile uint32_t DIER;
  volatile uint32_t SR;
  volatile uint32_t EGR;
  volatile uint32_t CCMR1;
  volatile uint32_t CCMR2;
  volatile uint32_t CCER;
  volatile uint32_t CNT;
  volatile uint32_t PSC;
  volatile uint32_t ARR;
  volatile uint32_t RESERVED0;
  volatile uint32_t CCR1;
  volatile uint32_t CCR2;
  volatile uint32_t CCR3;
  volatile uint32_t CCR4;
  volatile uint32_t RESERVED1;
  volatile uint32_t DCR;
  volatile uint32_t DMAR;
} TIM_RegDef_t;

#define GPIOA ((GPIO_RegDef_t *)GPIOA_BASE_ADDR)
#define GPIOB ((GPIO_RegDef_t *)GPIOB_BASE_ADDR)
#define GPIOC ((GPIO_RegDef_t *)GPIOC_BASE_ADDR)
#define GPIOD ((GPIO_RegDef_t *)GPIOD_BASE_ADDR)
#define GPIOE ((GPIO_RegDef_t *)GPIOE_BASE_ADDR)
#define GPIOF ((GPIO_RegDef_t *)GPIOF_BASE_ADDR)
#define GPIOG ((GPIO_RegDef_t *)GPIOG_BASE_ADDR)
#define GPIOH ((GPIO_RegDef_t *)GPIOH_BASE_ADDR)
#define GPIOI ((GPIO_RegDef_t *)GPIOI_BASE_ADDR)
#define GPIOJ ((GPIO_RegDef_t *)GPIOJ_BASE_ADDR)
#define GPIOK ((GPIO_RegDef_t *)GPIOK_BASE_ADDR)
#define RCC ((RCC_RegDef_t *)RCC_BASE_ADDR)
#define TIM3 ((TIM_RegDef_t *)TIM3_BASE_ADDR)
#define ADC1 ((ADC_RegDef_t *)ADC1_BASE_ADDR)
#define ADC_COMMON ((ADC_CommonRegDef_t *)ADC_COMMON_BASE_ADDR)

enum PORT_TYPE { INPUT = 0x0, OUTPUT = 0x1, ALT = 0x2, ANALOG = 0x3 };
enum PUDR { NO_PUDR = 0x0, PUP = 0x1, PDOWN = 0x2, RESERVED = 0x3 };
enum LED_STATE { ON = 0x1, OFF = 0x0 };
enum OUTPUT_TYPE { PUSH_PULL = 0x0, OPEN_DRAIN = 0x1 };
enum GPIO_SPEED { LOW_SPEED = 0x0, MEDIUM_SPEED = 0x1, HIGH_SPEED = 0x2, VERY_HIGH_SPEED = 0x3 };

typedef enum {
  BUTTON_NONE = 0,
  BUTTON_MODE,
  BUTTON_SET,
  BUTTON_UP,
  BUTTON_DOWN,
  BUTTON_OFF
} ButtonCode_t;

typedef struct {
  uint8_t seconds;
  uint8_t minutes;
  uint8_t hours;
  uint8_t day;
  uint8_t month;
} DateTime_t;

static inline void RCC_EnableGPIO(GPIO_RegDef_t *GPIOx) {
  if (GPIOx == GPIOA) {
    RCC->AHB1ENR |= (1U << 0);
  } else if (GPIOx == GPIOB) {
    RCC->AHB1ENR |= (1U << 1);
  } else if (GPIOx == GPIOC) {
    RCC->AHB1ENR |= (1U << 2);
  } else if (GPIOx == GPIOD) {
    RCC->AHB1ENR |= (1U << 3);
  } else if (GPIOx == GPIOE) {
    RCC->AHB1ENR |= (1U << 4);
  } else if (GPIOx == GPIOF) {
    RCC->AHB1ENR |= (1U << 5);
  } else if (GPIOx == GPIOG) {
    RCC->AHB1ENR |= (1U << 6);
  } else if (GPIOx == GPIOH) {
    RCC->AHB1ENR |= (1U << 7);
  } else if (GPIOx == GPIOI) {
    RCC->AHB1ENR |= (1U << 8);
  }

  (void)RCC->AHB1ENR;
}

static inline void GPIO_SetPinMode(GPIO_RegDef_t *GPIOx, uint8_t pinNumber,
                                   enum PORT_TYPE mode) {
  GPIOx->MODER &= ~(0x3UL << (pinNumber * 2U));
  GPIOx->MODER |= ((uint32_t)mode << (pinNumber * 2U));
}

static inline void GPIO_SetPinPull(GPIO_RegDef_t *GPIOx, uint8_t pinNumber,
                                   enum PUDR pull) {
  GPIOx->PUPDR &= ~(0x3UL << (pinNumber * 2U));
  GPIOx->PUPDR |= ((uint32_t)pull << (pinNumber * 2U));
}

static inline void GPIO_SetPinOutputType(GPIO_RegDef_t *GPIOx, uint8_t pinNumber,
                                         enum OUTPUT_TYPE outputType) {
  GPIOx->OTYPER &= ~(0x1UL << pinNumber);
  GPIOx->OTYPER |= ((uint32_t)outputType << pinNumber);
}

static inline void GPIO_SetPinSpeed(GPIO_RegDef_t *GPIOx, uint8_t pinNumber,
                                    enum GPIO_SPEED speed) {
  GPIOx->OSPEEDR &= ~(0x3UL << (pinNumber * 2U));
  GPIOx->OSPEEDR |= ((uint32_t)speed << (pinNumber * 2U));
}

static inline void GPIO_SetPinAlternateFunction(GPIO_RegDef_t *GPIOx,
                                                uint8_t pinNumber,
                                                uint8_t alternateFunction) {
  if (pinNumber < 8U) {
    GPIOx->AFRL &= ~(0xFUL << (pinNumber * 4U));
    GPIOx->AFRL |= ((uint32_t)alternateFunction << (pinNumber * 4U));
  } else {
    uint8_t highPinNumber = (uint8_t)(pinNumber - 8U);
    GPIOx->AFRH &= ~(0xFUL << (highPinNumber * 4U));
    GPIOx->AFRH |= ((uint32_t)alternateFunction << (highPinNumber * 4U));
  }
}

static inline void GPIO_WritePin(GPIO_RegDef_t *GPIOx, uint8_t pinNumber,
                                 enum LED_STATE state) {
  if (state == ON) {
    GPIOx->BSRR = (1UL << pinNumber);
  } else {
    GPIOx->BSRR = (1UL << (pinNumber + 16U));
  }
}

static inline void GPIO_SetPin(GPIO_RegDef_t *GPIOx, uint8_t pinNumber) {
  GPIO_WritePin(GPIOx, pinNumber, ON);
}

static inline void GPIO_ResetPin(GPIO_RegDef_t *GPIOx, uint8_t pinNumber) {
  GPIO_WritePin(GPIOx, pinNumber, OFF);
}

static inline void GPIO_TogglePin(GPIO_RegDef_t *GPIOx, uint8_t pinNumber) {
  GPIOx->ODR ^= (1UL << pinNumber);
}

static inline uint8_t GPIO_ReadPin(GPIO_RegDef_t *GPIOx, uint8_t pinNumber) {
  return (uint8_t)((GPIOx->IDR >> pinNumber) & 0x1U);
}

static inline void Buzzer_Init(void) {
  RCC_EnableGPIO(BUZZER_GPIO);

  GPIO_SetPinMode(BUZZER_GPIO, BUZZER_PIN, OUTPUT);
  GPIO_SetPinPull(BUZZER_GPIO, BUZZER_PIN, NO_PUDR);
  GPIO_SetPinOutputType(BUZZER_GPIO, BUZZER_PIN, PUSH_PULL);
  GPIO_SetPinSpeed(BUZZER_GPIO, BUZZER_PIN, LOW_SPEED);
  GPIO_ResetPin(BUZZER_GPIO, BUZZER_PIN);
}

static inline void Buzzer_SetEnabled(uint8_t isEnabled) {
  GPIO_WritePin(BUZZER_GPIO, BUZZER_PIN, isEnabled ? ON : OFF);
}

static inline uint8_t BcdToDecimal(uint8_t bcdValue) {
  return (uint8_t)(((bcdValue >> 4U) * 10U) + (bcdValue & 0x0FU));
}

static inline uint8_t DecimalToBcd(uint8_t decimalValue) {
  return (uint8_t)(((decimalValue / 10U) << 4U) | (decimalValue % 10U));
}

static inline void ShiftRegister_Init(void) {
  RCC_EnableGPIO(SHIFT_REGISTER_GPIO);

  GPIO_SetPinMode(SHIFT_REGISTER_GPIO, SHIFT_REGISTER_DATA_PIN, OUTPUT);
  GPIO_SetPinMode(SHIFT_REGISTER_GPIO, SHIFT_REGISTER_CLOCK_PIN, OUTPUT);
  GPIO_SetPinMode(SHIFT_REGISTER_GPIO, SHIFT_REGISTER_LATCH_PIN, OUTPUT);
  GPIO_SetPinMode(SHIFT_REGISTER_GPIO, SHIFT_REGISTER_OUTPUT_ENABLE_PIN, OUTPUT);

  GPIO_SetPinPull(SHIFT_REGISTER_GPIO, SHIFT_REGISTER_DATA_PIN, NO_PUDR);
  GPIO_SetPinPull(SHIFT_REGISTER_GPIO, SHIFT_REGISTER_CLOCK_PIN, NO_PUDR);
  GPIO_SetPinPull(SHIFT_REGISTER_GPIO, SHIFT_REGISTER_LATCH_PIN, NO_PUDR);
  GPIO_SetPinPull(SHIFT_REGISTER_GPIO, SHIFT_REGISTER_OUTPUT_ENABLE_PIN, NO_PUDR);

  GPIO_SetPinOutputType(SHIFT_REGISTER_GPIO, SHIFT_REGISTER_DATA_PIN, PUSH_PULL);
  GPIO_SetPinOutputType(SHIFT_REGISTER_GPIO, SHIFT_REGISTER_CLOCK_PIN, PUSH_PULL);
  GPIO_SetPinOutputType(SHIFT_REGISTER_GPIO, SHIFT_REGISTER_LATCH_PIN, PUSH_PULL);
  GPIO_SetPinOutputType(SHIFT_REGISTER_GPIO, SHIFT_REGISTER_OUTPUT_ENABLE_PIN, PUSH_PULL);

  GPIO_SetPinSpeed(SHIFT_REGISTER_GPIO, SHIFT_REGISTER_DATA_PIN, LOW_SPEED);
  GPIO_SetPinSpeed(SHIFT_REGISTER_GPIO, SHIFT_REGISTER_CLOCK_PIN, LOW_SPEED);
  GPIO_SetPinSpeed(SHIFT_REGISTER_GPIO, SHIFT_REGISTER_LATCH_PIN, LOW_SPEED);
  GPIO_SetPinSpeed(SHIFT_REGISTER_GPIO, SHIFT_REGISTER_OUTPUT_ENABLE_PIN, LOW_SPEED);

  GPIO_ResetPin(SHIFT_REGISTER_GPIO, SHIFT_REGISTER_DATA_PIN);
  GPIO_ResetPin(SHIFT_REGISTER_GPIO, SHIFT_REGISTER_CLOCK_PIN);
  GPIO_ResetPin(SHIFT_REGISTER_GPIO, SHIFT_REGISTER_LATCH_PIN);
  GPIO_ResetPin(SHIFT_REGISTER_GPIO, SHIFT_REGISTER_OUTPUT_ENABLE_PIN);
}

static inline void ShiftRegister_OutputEnablePwm_Init(uint8_t brightness) {
  RCC_EnableGPIO(SHIFT_REGISTER_GPIO);
  RCC->APB1ENR |= (1U << 1);
  (void)RCC->APB1ENR;

  GPIO_SetPinMode(SHIFT_REGISTER_GPIO, SHIFT_REGISTER_OUTPUT_ENABLE_PIN, ALT);
  GPIO_SetPinPull(SHIFT_REGISTER_GPIO, SHIFT_REGISTER_OUTPUT_ENABLE_PIN, NO_PUDR);
  GPIO_SetPinOutputType(SHIFT_REGISTER_GPIO, SHIFT_REGISTER_OUTPUT_ENABLE_PIN,
                        PUSH_PULL);
  GPIO_SetPinSpeed(SHIFT_REGISTER_GPIO, SHIFT_REGISTER_OUTPUT_ENABLE_PIN,
                   LOW_SPEED);
  GPIO_SetPinAlternateFunction(SHIFT_REGISTER_GPIO,
                               SHIFT_REGISTER_OUTPUT_ENABLE_PIN, GPIO_AF_TIM3);

  TIM3->CR1 = 0U;
  TIM3->PSC = 79U;
  TIM3->ARR = 99U;
  TIM3->CCR1 = brightness;
  TIM3->CCMR1 &= ~(0xFFUL);
  TIM3->CCMR1 |= (6UL << 4) | (1UL << 3);
  TIM3->CCER &= ~(0xFUL);
  TIM3->CCER |= (1UL << 1) | (1UL << 0);
  TIM3->EGR = 1U;
  TIM3->CR1 |= (1UL << 7) | (1UL << 0);
}

static inline void ShiftRegister_SetBrightness(uint8_t brightness) {
  if (brightness > PWM_MAX_DUTY) {
    brightness = PWM_MAX_DUTY;
  }
  TIM3->CCR1 = brightness;
}

static inline void ShiftRegister_Pulse(GPIO_RegDef_t *GPIOx, uint8_t pinNumber) {
  GPIO_SetPin(GPIOx, pinNumber);
  GPIO_ResetPin(GPIOx, pinNumber);
}

static inline void ShiftRegister_WriteByte(uint8_t value) {
  GPIO_ResetPin(SHIFT_REGISTER_GPIO, SHIFT_REGISTER_LATCH_PIN);

  for (int8_t bit = 7; bit >= 0; --bit) {
    GPIO_WritePin(SHIFT_REGISTER_GPIO, SHIFT_REGISTER_DATA_PIN,
                  ((value >> bit) & 0x1U) ? ON : OFF);
    ShiftRegister_Pulse(SHIFT_REGISTER_GPIO, SHIFT_REGISTER_CLOCK_PIN);
  }

  ShiftRegister_Pulse(SHIFT_REGISTER_GPIO, SHIFT_REGISTER_LATCH_PIN);
}

static inline void ShiftRegister_WriteBoards(uint8_t board1Mask,
                                             uint8_t board2Mask) {
  GPIO_ResetPin(SHIFT_REGISTER_GPIO, SHIFT_REGISTER_LATCH_PIN);

  for (int8_t bit = 7; bit >= 0; --bit) {
    GPIO_WritePin(SHIFT_REGISTER_GPIO, SHIFT_REGISTER_DATA_PIN,
                  ((board2Mask >> bit) & 0x1U) ? ON : OFF);
    ShiftRegister_Pulse(SHIFT_REGISTER_GPIO, SHIFT_REGISTER_CLOCK_PIN);
  }

  for (int8_t bit = 7; bit >= 0; --bit) {
    GPIO_WritePin(SHIFT_REGISTER_GPIO, SHIFT_REGISTER_DATA_PIN,
                  ((board1Mask >> bit) & 0x1U) ? ON : OFF);
    ShiftRegister_Pulse(SHIFT_REGISTER_GPIO, SHIFT_REGISTER_CLOCK_PIN);
  }

  ShiftRegister_Pulse(SHIFT_REGISTER_GPIO, SHIFT_REGISTER_LATCH_PIN);
}

static inline void ShiftRegister_SetOutputEnabled(uint8_t isEnabled) {
  if (isEnabled != 0U) {
    GPIO_ResetPin(SHIFT_REGISTER_GPIO, SHIFT_REGISTER_OUTPUT_ENABLE_PIN);
  } else {
    GPIO_SetPin(SHIFT_REGISTER_GPIO, SHIFT_REGISTER_OUTPUT_ENABLE_PIN);
  }
}

static inline void DelayCycles(volatile uint32_t cycles) {
  while (cycles > 0U) {
    --cycles;
  }
}

static inline uint8_t ButtonsDecoder_ButtonToBrightness(ButtonCode_t button,
                                                        uint8_t currentBrightness) {
  switch (button) {
  case BUTTON_MODE:
    return BRIGHTNESS_100;
  case BUTTON_SET:
    return BRIGHTNESS_80;
  case BUTTON_UP:
    return BRIGHTNESS_50;
  case BUTTON_DOWN:
    return BRIGHTNESS_30;
  case BUTTON_OFF:
    return BRIGHTNESS_10;
  case BUTTON_NONE:
  default:
    return currentBrightness;
  }
}

static inline uint8_t ShiftRegister_ButtonToLedMask(ButtonCode_t button) {
  switch (button) {
  case BUTTON_MODE:
    return (uint8_t)(1U << BOARD2_MODE_HOURS_BIT);
  case BUTTON_SET:
    return (uint8_t)(1U << BOARD2_MODE_DATE_BIT);
  case BUTTON_UP:
    return (uint8_t)(1U << BOARD2_MODE_ENVIRONMENT_BIT);
  case BUTTON_DOWN:
    return (uint8_t)(1U << BOARD2_ALARM_SET_BIT);
  case BUTTON_OFF:
    return 0U;
  case BUTTON_NONE:
  default:
    return 0U;
  }
}

static inline uint16_t ADC1_ReadChannel(uint8_t adcChannel);

static inline void ButtonsDecoder_Init(void) {
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

static inline void LightSensor_Init(void) {
  ButtonsDecoder_Init();
}

static inline uint16_t ADC1_ReadChannel(uint8_t adcChannel) {
  ADC1->SQR3 = adcChannel;
  ADC1->SR = 0U;
  ADC1->CR2 |= ADC_CR2_SWSTART;
  while ((ADC1->SR & ADC_SR_EOC) == 0U) {
  }
  return (uint16_t)(ADC1->DR & 0x0FFFU);
}

static inline uint16_t ButtonsDecoder_ReadRaw(void) {
  return ADC1_ReadChannel(BUTTONS_ADC_CHANNEL);
}

static inline ButtonCode_t ButtonsDecoder_Decode(uint16_t adcValue) {
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

static inline ButtonCode_t ButtonsDecoder_Read(void) {
  return ButtonsDecoder_Decode(ButtonsDecoder_ReadRaw());
}

static inline uint16_t LightSensor_ReadRaw(void) {
  return ADC1_ReadChannel(LIGHT_SENSOR_ADC_CHANNEL);
}

static inline uint8_t LightSensor_ReadBrightness(void) {
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

static inline uint8_t LightSensor_ReadLevelMask(void) {
  uint16_t adcValue = LightSensor_ReadRaw();
  uint8_t level = 0U;
  uint8_t mask = 0U;

  if (adcValue <= LIGHT_SENSOR_ADC_DARK) {
    level = 1U;
  } else if (adcValue >= LIGHT_SENSOR_ADC_BRIGHT) {
    level = 8U;
  } else {
    uint32_t adcRange = LIGHT_SENSOR_ADC_BRIGHT - LIGHT_SENSOR_ADC_DARK;
    uint32_t adcOffset = adcValue - LIGHT_SENSOR_ADC_DARK;
    level = (uint8_t)(1U + ((adcOffset * 7U) / adcRange));
  }

  for (uint8_t bit = 0U; bit < level; ++bit) {
    mask |= (uint8_t)(1U << bit);
  }

  return mask;
}

static inline void DisableInterrupts(void) {
  __asm volatile("cpsid i" ::: "memory");
}

static inline void EnableInterrupts(void) {
  __asm volatile("cpsie i" ::: "memory");
}

static inline void DS1302_DataOutput(void) {
  GPIO_SetPinMode(DS1302_GPIO, DS1302_DATA_PIN, OUTPUT);
}

static inline void DS1302_DataInput(void) {
  GPIO_SetPinMode(DS1302_GPIO, DS1302_DATA_PIN, INPUT);
}

static inline void DS1302_Init(void) {
  RCC_EnableGPIO(DS1302_GPIO);

  GPIO_SetPinMode(DS1302_GPIO, DS1302_CLK_PIN, OUTPUT);
  GPIO_SetPinMode(DS1302_GPIO, DS1302_RST_PIN, OUTPUT);
  DS1302_DataOutput();

  GPIO_SetPinPull(DS1302_GPIO, DS1302_CLK_PIN, NO_PUDR);
  GPIO_SetPinPull(DS1302_GPIO, DS1302_RST_PIN, NO_PUDR);
  GPIO_SetPinPull(DS1302_GPIO, DS1302_DATA_PIN, NO_PUDR);

  GPIO_SetPinOutputType(DS1302_GPIO, DS1302_CLK_PIN, PUSH_PULL);
  GPIO_SetPinOutputType(DS1302_GPIO, DS1302_RST_PIN, PUSH_PULL);
  GPIO_SetPinOutputType(DS1302_GPIO, DS1302_DATA_PIN, PUSH_PULL);

  GPIO_SetPinSpeed(DS1302_GPIO, DS1302_CLK_PIN, LOW_SPEED);
  GPIO_SetPinSpeed(DS1302_GPIO, DS1302_RST_PIN, LOW_SPEED);
  GPIO_SetPinSpeed(DS1302_GPIO, DS1302_DATA_PIN, LOW_SPEED);

  GPIO_ResetPin(DS1302_GPIO, DS1302_CLK_PIN);
  GPIO_ResetPin(DS1302_GPIO, DS1302_RST_PIN);
  GPIO_ResetPin(DS1302_GPIO, DS1302_DATA_PIN);
}

static inline void DS1302_WriteByte(uint8_t value) {
  DS1302_DataOutput();

  for (uint8_t bit = 0U; bit < 8U; ++bit) {
    GPIO_WritePin(DS1302_GPIO, DS1302_DATA_PIN,
                  ((value >> bit) & 0x1U) ? ON : OFF);
    GPIO_SetPin(DS1302_GPIO, DS1302_CLK_PIN);
    DelayCycles(20U);
    GPIO_ResetPin(DS1302_GPIO, DS1302_CLK_PIN);
    DelayCycles(20U);
  }
}

static inline uint8_t DS1302_ReadByte(void) {
  uint8_t value = 0U;
  DS1302_DataInput();

  for (uint8_t bit = 0U; bit < 8U; ++bit) {
    if (GPIO_ReadPin(DS1302_GPIO, DS1302_DATA_PIN) != 0U) {
      value |= (uint8_t)(1U << bit);
    }
    GPIO_SetPin(DS1302_GPIO, DS1302_CLK_PIN);
    DelayCycles(20U);
    GPIO_ResetPin(DS1302_GPIO, DS1302_CLK_PIN);
    DelayCycles(20U);
  }

  return value;
}

static inline void DS1302_WriteRegister(uint8_t command, uint8_t value) {
  GPIO_SetPin(DS1302_GPIO, DS1302_RST_PIN);
  DelayCycles(20U);
  DS1302_WriteByte(command);
  DS1302_WriteByte(value);
  GPIO_ResetPin(DS1302_GPIO, DS1302_RST_PIN);
  GPIO_ResetPin(DS1302_GPIO, DS1302_CLK_PIN);
}

static inline uint8_t DS1302_ReadRegister(uint8_t command) {
  uint8_t value = 0U;

  GPIO_SetPin(DS1302_GPIO, DS1302_RST_PIN);
  DelayCycles(20U);
  DS1302_WriteByte(command);
  value = DS1302_ReadByte();
  GPIO_ResetPin(DS1302_GPIO, DS1302_RST_PIN);
  GPIO_ResetPin(DS1302_GPIO, DS1302_CLK_PIN);
  DS1302_DataOutput();

  return value;
}

static inline void DS1302_SetWriteProtect(uint8_t isEnabled) {
  DS1302_WriteRegister(DS1302_CONTROL_WRITE, isEnabled ? 0x80U : 0x00U);
}

static inline void DS1302_StartClock(void) {
  uint8_t seconds = DS1302_ReadRegister(DS1302_SECONDS_READ);
  DS1302_SetWriteProtect(0U);
  DS1302_WriteRegister(DS1302_SECONDS_WRITE, (uint8_t)(seconds & 0x7FU));
}

static inline void DS1302_ReadDateTime(DateTime_t *dateTime) {
  dateTime->seconds =
      BcdToDecimal((uint8_t)(DS1302_ReadRegister(DS1302_SECONDS_READ) & 0x7FU));
  dateTime->minutes =
      BcdToDecimal((uint8_t)(DS1302_ReadRegister(DS1302_MINUTES_READ) & 0x7FU));
  dateTime->hours =
      BcdToDecimal((uint8_t)(DS1302_ReadRegister(DS1302_HOURS_READ) & 0x3FU));
  dateTime->day =
      BcdToDecimal((uint8_t)(DS1302_ReadRegister(DS1302_DATE_READ) & 0x3FU));
  dateTime->month =
      BcdToDecimal((uint8_t)(DS1302_ReadRegister(DS1302_MONTH_READ) & 0x1FU));
}

static inline void DS1302_WriteTime(uint8_t hours, uint8_t minutes,
                                    uint8_t seconds) {
  DS1302_SetWriteProtect(0U);
  DS1302_WriteRegister(DS1302_SECONDS_WRITE, DecimalToBcd(seconds));
  DS1302_WriteRegister(DS1302_MINUTES_WRITE, DecimalToBcd(minutes));
  DS1302_WriteRegister(DS1302_HOURS_WRITE, DecimalToBcd((uint8_t)(hours & 0x3FU)));
}

static inline void DS1302_WriteDate(uint8_t day, uint8_t month) {
  DS1302_SetWriteProtect(0U);
  DS1302_WriteRegister(DS1302_DATE_WRITE, DecimalToBcd(day));
  DS1302_WriteRegister(DS1302_MONTH_WRITE, DecimalToBcd(month));
}

static inline uint8_t DS1302_RamCommand(uint8_t address, uint8_t isRead) {
  address %= 31U;
  return (uint8_t)(DS1302_RAM_WRITE_BASE + (address * 2U) + (isRead ? 1U : 0U));
}

static inline void DS1302_WriteRamByte(uint8_t address, uint8_t value) {
  DS1302_SetWriteProtect(0U);
  DS1302_WriteRegister(DS1302_RamCommand(address, 0U), value);
}

static inline uint8_t DS1302_ReadRamByte(uint8_t address) {
  return DS1302_ReadRegister(DS1302_RamCommand(address, 1U));
}

#endif /* UTILS_H_ */
