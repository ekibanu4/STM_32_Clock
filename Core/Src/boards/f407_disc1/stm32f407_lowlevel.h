/*
 * stm32f407_lowlevel.h
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


#ifndef STM32F407_LOWLEVEL_H_
#define STM32F407_LOWLEVEL_H_

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

static inline void DelayCycles(volatile uint32_t cycles) {
  while (cycles > 0U) {
    --cycles;
  }
}

static inline uint16_t ADC1_ReadChannel(uint8_t adcChannel);

static inline uint16_t ADC1_ReadChannel(uint8_t adcChannel) {
  ADC1->SQR3 = adcChannel;
  ADC1->SR = 0U;
  ADC1->CR2 |= ADC_CR2_SWSTART;
  while ((ADC1->SR & ADC_SR_EOC) == 0U) {
  }
  return (uint16_t)(ADC1->DR & 0x0FFFU);
}

static inline void DisableInterrupts(void) {
  __asm volatile("cpsid i" ::: "memory");
}

static inline void EnableInterrupts(void) {
  __asm volatile("cpsie i" ::: "memory");
}


#endif /* STM32F407_LOWLEVEL_H_ */
