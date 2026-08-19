/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "alarm_manager.h"
#include "board.h"
#include "display_renderer.h"
#include "environment_manager.h"
#include "oled_128x32.h"
#include "ui_controller.h"

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define MAIN_LOOP_DELAY_MS 20U
#define RTC_REFRESH_INTERVAL_MS 1000U
#define BLINK_INTERVAL_MS 800U
#define MAIN_POWER_STANDBY_ENABLED 1U
#define IDLE_AUTO_MODE_TIME_SEEN 0x01U
#define IDLE_AUTO_MODE_DATE_SEEN 0x02U
#define IDLE_AUTO_MODE_ENVIRONMENT_SEEN 0x04U
#define IDLE_AUTO_MODE_ALL_SEEN \
  (IDLE_AUTO_MODE_TIME_SEEN | IDLE_AUTO_MODE_DATE_SEEN | \
   IDLE_AUTO_MODE_ENVIRONMENT_SEEN)
#define LOW_BATTERY_BLINK_INTERVAL_MS 1000U
#define APP_MSI_ACTIVE_RANGE RCC_ICSCR_MSIRANGE_6
#define APP_MSI_ACTIVE_HZ 4194304UL
#define APP_MSI_IDLE_RANGE RCC_ICSCR_MSIRANGE_5
#define APP_MSI_IDLE_HZ 2097152UL

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN PV */
RTC_HandleTypeDef hrtc;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_ADC_Init(void);
/* USER CODE BEGIN PFP */
static uint8_t App_RTC_Init(void);
static uint8_t App_Tick(void);
static void App_SetMsiClock(uint32_t msiRange, uint32_t coreClockHz);

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  if (HAL_InitTick(TICK_INT_PRIORITY) != HAL_OK)
  {
    Error_Handler();
  }
  HAL_MspInit();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_ADC_Init();
  /* USER CODE BEGIN 2 */
  (void)App_RTC_Init();
  Board_Init();
  Oled128x32_Init();

  UiController_Init();
  AlarmManager_Init();
  EnvironmentManager_Init();

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    (void)App_Tick();

    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  /** Configure the main internal regulator output voltage
  */
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  RCC->CFGR &= ~(RCC_CFGR_SW | RCC_CFGR_HPRE | RCC_CFGR_PPRE1 |
                 RCC_CFGR_PPRE2);
  App_SetMsiClock(APP_MSI_ACTIVE_RANGE, APP_MSI_ACTIVE_HZ);
}

/**
  * @brief ADC Initialization Function
  * @param None
  * @retval None
  */
static void MX_ADC_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  __HAL_RCC_ADC1_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  GPIO_InitStruct.Pin = GPIO_PIN_1;
  GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  ADC1_COMMON->CCR = (ADC1_COMMON->CCR & ~(ADC_CCR_PRESC | ADC_CCR_LFMEN)) |
                     ADC_CCR_LFMEN;
  ADC1->CR |= ADC_CR_ADVREGEN;
  ADC1->CFGR1 = 0U;
  ADC1->CFGR2 = ADC_CFGR2_CKMODE;
  ADC1->SMPR = ADC_SMPR_SMPR_1;
  ADC1->CHSELR = ADC_CHSELR_CHSEL1 | ADC_CHSELR_CHSEL9;

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  /* USER CODE BEGIN MX_GPIO_Init_1 */

  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_2|GPIO_PIN_3|GPIO_PIN_4, GPIO_PIN_RESET);

  /*Configure GPIO pins : PA2 PA3 PA4 */
  GPIO_InitStruct.Pin = GPIO_PIN_2|GPIO_PIN_3|GPIO_PIN_4;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pin : PA5 */
  GPIO_InitStruct.Pin = GPIO_PIN_5;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  GPIO_InitStruct.Alternate = GPIO_AF5_TIM2;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */
HAL_StatusTypeDef HAL_InitTick(uint32_t TickPriority)
{
  if (TickPriority < (1UL << __NVIC_PRIO_BITS)) {
    uwTickPrio = TickPriority;
  }

  SysTick->LOAD = (SystemCoreClock / 1000U) - 1UL;
  SysTick->VAL = 0UL;
  SysTick->CTRL = SysTick_CTRL_CLKSOURCE_Msk | SysTick_CTRL_TICKINT_Msk |
                  SysTick_CTRL_ENABLE_Msk;
  return HAL_OK;
}

static void App_SetMsiClock(uint32_t msiRange, uint32_t coreClockHz)
{
  RCC->CR |= RCC_CR_MSION;
  while ((RCC->CR & RCC_CR_MSIRDY) == 0U) {
  }

  RCC->ICSCR = (RCC->ICSCR & ~RCC_ICSCR_MSIRANGE) | msiRange;
  while ((RCC->CR & RCC_CR_MSIRDY) == 0U) {
  }

  SystemCoreClock = coreClockHz;
  if (HAL_InitTick(TICK_INT_PRIORITY) != HAL_OK) {
    Error_Handler();
  }
}

static uint8_t App_RTC_Init(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};

  HAL_PWR_EnableBkUpAccess();

  if (__HAL_RCC_GET_RTC_SOURCE() != RCC_RTCCLKSOURCE_LSE) {
    __HAL_RCC_BACKUPRESET_FORCE();
    __HAL_RCC_BACKUPRESET_RELEASE();
  }

  __HAL_RCC_LSEDRIVE_CONFIG(RCC_LSEDRIVE_HIGH);

  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_LSE;
  RCC_OscInitStruct.LSEState = RCC_LSE_ON;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    return 0U;
  }

  if (__HAL_RCC_GET_FLAG(RCC_FLAG_LSERDY) == RESET) {
    return 0U;
  }

  __HAL_RCC_RTC_CONFIG(RCC_RTCCLKSOURCE_LSE);
  __HAL_RCC_RTC_ENABLE();

  hrtc.Instance = RTC;
  hrtc.Init.HourFormat = RTC_HOURFORMAT_24;
  hrtc.Init.AsynchPrediv = 127;
  hrtc.Init.SynchPrediv = 255;
  hrtc.Init.OutPut = RTC_OUTPUT_DISABLE;
  hrtc.Init.OutPutPolarity = RTC_OUTPUT_POLARITY_HIGH;
  hrtc.Init.OutPutType = RTC_OUTPUT_TYPE_OPENDRAIN;
  if (HAL_RTC_Init(&hrtc) != HAL_OK)
  {
    return 0U;
  }

  return 1U;
}

static uint8_t App_Tick(void)
{
  static uint32_t lastLoopTick = 0U;
  static uint32_t lastRtcRefreshTick = 0U;
  static uint32_t lastBlinkTick = 0U;
  static DisplayMode_t lastIdleDisplayMode = DISPLAY_TIME;
  static uint8_t oledAlarmShown = 0U;
  static uint8_t blinkOn = 1U;
  static uint8_t idlePeripheralsOff = 0U;
  static uint8_t idleAutoModeSeenMask = 0U;
  static uint8_t lowBatteryMode = 0U;
  static uint8_t alarmButtonReleasePending = 0U;
  uint32_t now = HAL_GetTick();
  ClockButton_t button = CLOCK_BUTTON_NONE;
  ClockDisplay_t display = {0U, 0U};
  DisplayMode_t displayMode = DISPLAY_TIME;
  uint16_t batteryPercentTenths = 0U;
  uint8_t alarmAnyEnabled = 0U;
  uint8_t userActivity = 0U;
  uint8_t buttonActivity = 0U;

  if ((now - lastLoopTick) < MAIN_LOOP_DELAY_MS) {
    return 0U;
  }
  lastLoopTick = now;

#if MAIN_POWER_STANDBY_ENABLED
  if (Board_IsWakePowerPresent() == 0U) {
    Board_EnterStandby();
    return 1U;
  }
#endif

  if ((now - lastRtcRefreshTick) >= RTC_REFRESH_INTERVAL_MS) {
    UiController_RefreshDateTime();
    lastRtcRefreshTick = now;
  }

  AlarmManager_UpdateTrigger(UiController_DateTime());

  button = Board_ReadButton();
  buttonActivity = (button != CLOCK_BUTTON_NONE) ? 1U : 0U;
  if (buttonActivity == 0U) {
    alarmButtonReleasePending = 0U;
  } else if (AlarmManager_IsBuzzerActive() != 0U) {
    AlarmManager_StopBuzzer();
    alarmButtonReleasePending = 1U;
    button = CLOCK_BUTTON_NONE;
  } else if (alarmButtonReleasePending != 0U) {
    button = CLOCK_BUTTON_NONE;
  }
  UiController_UpdateButton(button, now);

  if (Board_IsBatteryAboveLowThreshold() == 0U) {
    if (lowBatteryMode == 0U) {
      Board_PowerDownIdleDevices();
      lowBatteryMode = 1U;
      blinkOn = 0U;
      lastBlinkTick = now;
    }
    AlarmManager_UpdateBuzzer();
    if ((now - lastBlinkTick) >= LOW_BATTERY_BLINK_INTERVAL_MS) {
      blinkOn = (blinkOn == 0U) ? 1U : 0U;
      lastBlinkTick = now;
      Board_WriteDisplay(
          (ClockDisplay_t){0U, (uint8_t)(blinkOn << CLOCK_BOARD2_ALARM_BIT)});
    }
    return 1U;
  }

  if ((now - lastBlinkTick) >= BLINK_INTERVAL_MS) {
    blinkOn = (blinkOn == 0U) ? 1U : 0U;
    lastBlinkTick = now;
  }

  displayMode = UiController_DisplayMode();

  if (lowBatteryMode != 0U) {
    lowBatteryMode = 0U;
    App_SetMsiClock(APP_MSI_ACTIVE_RANGE, APP_MSI_ACTIVE_HZ);
    idlePeripheralsOff = 0U;
    idleAutoModeSeenMask = 0U;
    oledAlarmShown = 0U;
    UiController_ShowTimeMode();
    lastIdleDisplayMode = UiController_DisplayMode();
  }

  userActivity = ((HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_7) == GPIO_PIN_SET) ||
                  (buttonActivity != 0U) ||
                  (UiController_EditTarget() != EDIT_NONE) ||
                  (AlarmManager_IsBuzzerActive() != 0U))
                     ? 1U
                     : 0U;

  if (userActivity != 0U) {
    idleAutoModeSeenMask = 0U;
    lastIdleDisplayMode = displayMode;
    if (idlePeripheralsOff != 0U) {
      App_SetMsiClock(APP_MSI_ACTIVE_RANGE, APP_MSI_ACTIVE_HZ);
      UiController_ShowTimeMode();
      lastIdleDisplayMode = UiController_DisplayMode();
      idlePeripheralsOff = 0U;
      oledAlarmShown = 0U;
    }
  } else if (idlePeripheralsOff != 0U) {
    return 1U;
  }

  batteryPercentTenths = Board_ReadBatteryPercentTenths();
  UiController_UpdateAutoModeCycle();
  displayMode = UiController_DisplayMode();
  EnvironmentManager_Update();

  AlarmManager_UpdateBuzzer();
  alarmAnyEnabled = AlarmManager_AnyEnabled();
  Board_SetBrightness(Board_ReadBrightness());
  display = DisplayRenderer_Build(
      displayMode, UiController_EditTarget(),
      UiController_DateTime(), EnvironmentManager_Current(),
      AlarmManager_Slots(), AlarmManager_SlotCount(),
      AlarmManager_SelectedSlot(), batteryPercentTenths, alarmAnyEnabled,
      UiController_AlarmErrorActive(), blinkOn);
  Board_WriteDisplay(display);

  if (AlarmManager_IsBuzzerActive() != 0U) {
    if (oledAlarmShown == 0U) {
      Oled128x32_ShowAlarm(AlarmManager_ActiveSlot());
      oledAlarmShown = 1U;
    }
  } else {
    oledAlarmShown = 0U;
    Oled128x32_Render(now, displayMode, UiController_DateTime(),
                    EnvironmentManager_Current(), AlarmManager_Slots(),
                    AlarmManager_SlotCount(), AlarmManager_SelectedSlot(),
                    UiController_EditTarget(), batteryPercentTenths,
                    alarmAnyEnabled);
  }

  if (userActivity == 0U) {
    if (displayMode != lastIdleDisplayMode) {
      if (lastIdleDisplayMode == DISPLAY_TIME) {
        idleAutoModeSeenMask |= IDLE_AUTO_MODE_TIME_SEEN;
      } else if (lastIdleDisplayMode == DISPLAY_DATE) {
        idleAutoModeSeenMask |= IDLE_AUTO_MODE_DATE_SEEN;
      } else if (lastIdleDisplayMode == DISPLAY_ENVIRONMENT) {
        idleAutoModeSeenMask |= IDLE_AUTO_MODE_ENVIRONMENT_SEEN;
      } else {
        idleAutoModeSeenMask = 0U;
      }
      lastIdleDisplayMode = displayMode;
    }

    if ((idleAutoModeSeenMask & IDLE_AUTO_MODE_ALL_SEEN) ==
        IDLE_AUTO_MODE_ALL_SEEN) {
      Board_PowerDownIdleDevices();
      App_SetMsiClock(APP_MSI_IDLE_RANGE, APP_MSI_IDLE_HZ);
      idlePeripheralsOff = 1U;
      return 1U;
    }
  }

  return 1U;
}

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
