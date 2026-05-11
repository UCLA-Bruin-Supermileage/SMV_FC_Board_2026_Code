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
#include "smv_ads1118.h"
#include "smv_board_enums.h"
#include "smv_can_error.h"
#include "smv_canbus.h"
#include "stm32f446xx.h"
#include "stm32f4xx_hal_can.h"
#include "stm32f4xx_hal_gpio.h"
#include <stdbool.h>
#include <stdio.h>

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define CAN_TX_DELAY 10
#define BLINK_INTERVAL 500

#define PRESSURE 0
#define BRAKE 1
#define THROTTLE 2

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
CAN_HandleTypeDef hcan1;

SPI_HandleTypeDef hspi3;

/* USER CODE BEGIN PV */

// ADC, CAN
static double adc_readings[4] = {0};
static uint32_t lastTxTime;

// PERIPHERAL STATES
static volatile bool blink_left_active = false;
static volatile bool blink_right_active = false;
static volatile bool hazards_active = false;
static volatile bool headlights_active = false;
static volatile bool horn_active = false;
// static volatile bool wiper_active = false;

// Miscellaneous
CANBUS can1;
CAN_Error can_error;
SMV_ADS1118 adc1;
static uint8_t i = 0;
static uint8_t error_flag = 0;
volatile int sender;
volatile int type;
volatile double val;
// volatile char type_string[20] = {0};

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
/* USER CODE BEGIN PFP */
void blink_error(CAN_ErrorCode code);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef *CanHandle)

{
  /* Get RX message from FIFO0 and fill the data on the related FIFO0 user
     declared header (RxHeaderFIFO0) and table (RxDataFIFO0) */
  if (HAL_CAN_GetRxMessage(CanHandle, CAN_RX_FIFO0, &(can1.RxHeaderFIFO0),
                           can1.RxDataFIFO0) != HAL_OK) {
    /* Reception Error */
    CAN_Error_Record(can1.err, CAN_ERR_RX_FAILED);
  } else {
    CAN_Interrupt_Helper(&can1);

    sender = can1.getHardwareRaw(&can1);
    type = can1.getDataTypeRaw(&can1);
    val = can1.getData(&can1);
    // strcpy(type_string, can1.getDataType(&can1));

    if (sender == UI) {
      // Horn
      if (type == Horn) {
        horn_active = (val > 0.5) ? true : false;
      }
      // Wipers
      // else if (type == Wipers) {
      //   wiper_active = (val > 0.5) ? true : false;
      // }
      // Headlights
      else if (type == Headlights) {
        headlights_active = (val > 0.5) ? true : false;
      }
      // Hazard Lights
      else if (type == Hazard) {
        hazards_active = (val > 0.5) ? true : false;
      }
      // Turn Signals
      else if (type == Blink_Left) {
        blink_left_active = (val > 0.5) ? true : false;
      } else if (type == Blink_Right) {
        blink_right_active = (val > 0.5) ? true : false;
      }
    }
  }
}
/* USER CODE END 0 */

/**
 * @brief  The application entry point.
 * @retval int
 */
int main(void) {

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick.
   */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();

  /* USER CODE BEGIN 2 */
  can1 = CAN_new();
  can1.init(&can1, FC, &hcan1, &can_error);
  can1.addFilterDevice(&can1, UI, CAN_RX_FIFO0);
  can1.begin(&can1);

  adc1 = ADS_new();
  adc1.init(&adc1, &hspi3, SPI3, CS_GPIO_Port, CS_Pin, GPIOC, GPIO_PIN_11);

  lastTxTime = HAL_GetTick();
  uint32_t currentTime = 0;
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1) {
    currentTime = HAL_GetTick();

    // ADC, CAN TX
    adc1.sweep(&adc1, adc_readings);
    if (currentTime - lastTxTime >= CAN_TX_DELAY) {
      can1.send(&can1, adc_readings[PRESSURE], FC_Pressure); // FC to DAQ
      can1.send(&can1, adc_readings[BRAKE], Brake);          // FC to RC
      can1.send(&can1, adc_readings[THROTTLE], Gas);         // FC to MC
      lastTxTime = currentTime;
    }

    // TURN SIGNAL IMPLEMENTATION
    if (blink_left_active) {
      HAL_GPIO_WritePin(BlinkLeft_GPIO_Port, BlinkLeft_Pin, GPIO_PIN_SET);
    } else {
      HAL_GPIO_WritePin(BlinkLeft_GPIO_Port, BlinkLeft_Pin, GPIO_PIN_RESET);
    }

    if (blink_right_active) {
      HAL_GPIO_WritePin(BlinkRight_GPIO_Port, BlinkRight_Pin, GPIO_PIN_SET);
    } else {
      HAL_GPIO_WritePin(BlinkRight_GPIO_Port, BlinkRight_Pin, GPIO_PIN_RESET);
    }

    // HAZARD & HEADLIGHTS IMPLEMENTATION
    if (hazards_active) {
      HAL_GPIO_WritePin(HLR_GPIO_Port, HLR_Pin, GPIO_PIN_SET);
      HAL_GPIO_WritePin(HLL_GPIO_Port, HLL_Pin, GPIO_PIN_SET);

    } else {
      HAL_GPIO_WritePin(HLR_GPIO_Port, HLR_Pin,
                        (headlights_active) ? GPIO_PIN_SET : GPIO_PIN_RESET);
      HAL_GPIO_WritePin(HLL_GPIO_Port, HLL_Pin,
                        (headlights_active) ? GPIO_PIN_SET : GPIO_PIN_RESET);
    }

    // WIPER IMPLEMENTATION --- doesn't work since PA2 and PA3 are used for
    // USART2 HAL_GPIO_WritePin(Wiper2_GPIO_Port, Wiper2_Pin,
    //                   (wiper_active) ? GPIO_PIN_SET : GPIO_PIN_RESET);
    // HAL_GPIO_WritePin(Wiper4_GPIO_Port, Wiper4_Pin,
    //                   (wiper_active) ? GPIO_PIN_SET : GPIO_PIN_RESET);

    // HORN IMPLEMENTATION
    HAL_GPIO_WritePin(Horn_GPIO_Port, Horn_Pin,
                      (horn_active) ? GPIO_PIN_SET : GPIO_PIN_RESET);

    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
    if (can1.err->code != CAN_ERR_NONE) {
      blink_error(can1.err->code);
    }
  }
  /* USER CODE END 3 */
}

/**
 * @brief System Clock Configuration
 * @retval None
 */
void SystemClock_Config(void) {
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
   */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE3);

  /** Initializes the RCC Oscillators according to the specified parameters
   * in the RCC_OscInitTypeDef structure.
   */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 4;
  RCC_OscInitStruct.PLL.PLLN = 72;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 2;
  RCC_OscInitStruct.PLL.PLLR = 2;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
   */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK |
                                RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK) {
    Error_Handler();
  }
}

/**
 * @brief GPIO Initialization Function
 * @param None
 * @retval None
 */
static void MX_GPIO_Init(void) {
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  /* USER CODE BEGIN MX_GPIO_Init_1 */

  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA,
                    Wiper2_Pin | Wiper4_Pin | GPIO_PIN_5 | Horn_Pin | HLL_Pin |
                        BlinkLeft_Pin,
                    GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(BlinkRight_GPIO_Port, BlinkRight_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(CS_GPIO_Port, CS_Pin, GPIO_PIN_SET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(HLR_GPIO_Port, HLR_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin : B1_Pin */
  GPIO_InitStruct.Pin = B1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(B1_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : Wiper2_Pin Wiper4_Pin PA5 Horn_Pin
                           HLL_Pin BlinkLeft_Pin */
  GPIO_InitStruct.Pin =
      Wiper2_Pin | Wiper4_Pin | GPIO_PIN_5 | Horn_Pin | HLL_Pin | BlinkLeft_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pin : BlinkRight_Pin */
  GPIO_InitStruct.Pin = BlinkRight_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(BlinkRight_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : CS_Pin */
  GPIO_InitStruct.Pin = CS_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(CS_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : HLR_Pin */
  GPIO_InitStruct.Pin = HLR_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(HLR_GPIO_Port, &GPIO_InitStruct);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

/* Error handler viz */
void blink_error(CAN_ErrorCode code) {
  if (code == CAN_ERR_NONE)
    return;

  // turn off first so blink pattern is clean
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_RESET);
  HAL_Delay(1000); // 1s pause before pattern starts

  for (int i = 0; i < (int)code; i++) {
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_SET);
    HAL_Delay(400);
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_RESET);
    HAL_Delay(400);
  }

  HAL_Delay(1500); // long pause so you can count before it repeats
}
/* USER CODE END 4 */

/**
 * @brief  This function is executed in case of error occurrence.
 * @retval None
 */
void Error_Handler(void) {
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  error_flag = adc1.checkFlag(&adc1);
  while (1) {
    i += 1;
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
void assert_failed(uint8_t *file, uint32_t line) {
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line
     number, ex: printf("Wrong parameters value: file %s on line %d\r\n", file,
     line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
