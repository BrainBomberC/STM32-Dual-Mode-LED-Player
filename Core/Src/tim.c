/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    tim.c
  * @brief   This file provides code for the configuration
  *          of the TIM instances.
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
#include "tim.h"

/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

TIM_HandleTypeDef htim3;
TIM_HandleTypeDef htim5;
DMA_HandleTypeDef hdma_tim3_ch2;
DMA_HandleTypeDef hdma_tim3_ch3;
DMA_HandleTypeDef hdma_tim5_ch1;
DMA_HandleTypeDef hdma_tim5_ch2;
DMA_HandleTypeDef hdma_tim5_ch3_up;
DMA_HandleTypeDef hdma_tim5_ch4_trig;

/* TIM3 init function */
void MX_TIM3_Init(void)
{

  /* USER CODE BEGIN TIM3_Init 0 */

  /* USER CODE END TIM3_Init 0 */

  TIM_MasterConfigTypeDef sMasterConfig = {0};
  TIM_OC_InitTypeDef sConfigOC = {0};

  /* USER CODE BEGIN TIM3_Init 1 */

  /* USER CODE END TIM3_Init 1 */
  htim3.Instance = TIM3;
  htim3.Init.Prescaler = 0;
  htim3.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim3.Init.Period = 119;
  htim3.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim3.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
  if (HAL_TIM_PWM_Init(&htim3) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim3, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigOC.OCMode = TIM_OCMODE_PWM1;
  sConfigOC.Pulse = 0;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  if (HAL_TIM_PWM_ConfigChannel(&htim3, &sConfigOC, TIM_CHANNEL_2) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_PWM_ConfigChannel(&htim3, &sConfigOC, TIM_CHANNEL_3) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM3_Init 2 */

  /* USER CODE END TIM3_Init 2 */
  HAL_TIM_MspPostInit(&htim3);

}
/* TIM5 init function */
void MX_TIM5_Init(void)
{

  /* USER CODE BEGIN TIM5_Init 0 */

  /* USER CODE END TIM5_Init 0 */

  TIM_MasterConfigTypeDef sMasterConfig = {0};
  TIM_OC_InitTypeDef sConfigOC = {0};

  /* USER CODE BEGIN TIM5_Init 1 */

  /* USER CODE END TIM5_Init 1 */
  htim5.Instance = TIM5;
  htim5.Init.Prescaler = 0;
  htim5.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim5.Init.Period = 119;
  htim5.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim5.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
  if (HAL_TIM_PWM_Init(&htim5) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim5, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigOC.OCMode = TIM_OCMODE_PWM1;
  sConfigOC.Pulse = 0;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_ENABLE;
  if (HAL_TIM_PWM_ConfigChannel(&htim5, &sConfigOC, TIM_CHANNEL_1) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_PWM_ConfigChannel(&htim5, &sConfigOC, TIM_CHANNEL_2) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_PWM_ConfigChannel(&htim5, &sConfigOC, TIM_CHANNEL_3) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_PWM_ConfigChannel(&htim5, &sConfigOC, TIM_CHANNEL_4) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM5_Init 2 */

  /* USER CODE END TIM5_Init 2 */
  HAL_TIM_MspPostInit(&htim5);

}

void HAL_TIM_PWM_MspInit(TIM_HandleTypeDef* tim_pwmHandle)
{

  if(tim_pwmHandle->Instance==TIM3)
  {
  /* USER CODE BEGIN TIM3_MspInit 0 */

  /* USER CODE END TIM3_MspInit 0 */
    /* TIM3 clock enable */
    __HAL_RCC_TIM3_CLK_ENABLE();

    /* TIM3 DMA Init */
    /* TIM3_CH2 Init */
    hdma_tim3_ch2.Instance = DMA1_Stream5;
    hdma_tim3_ch2.Init.Channel = DMA_CHANNEL_5;
    hdma_tim3_ch2.Init.Direction = DMA_MEMORY_TO_PERIPH;
    hdma_tim3_ch2.Init.PeriphInc = DMA_PINC_DISABLE;
    hdma_tim3_ch2.Init.MemInc = DMA_MINC_ENABLE;
    hdma_tim3_ch2.Init.PeriphDataAlignment = DMA_PDATAALIGN_WORD;
    hdma_tim3_ch2.Init.MemDataAlignment = DMA_MDATAALIGN_WORD;
    hdma_tim3_ch2.Init.Mode = DMA_NORMAL;
    hdma_tim3_ch2.Init.Priority = DMA_PRIORITY_LOW;
    hdma_tim3_ch2.Init.FIFOMode = DMA_FIFOMODE_DISABLE;
    if (HAL_DMA_Init(&hdma_tim3_ch2) != HAL_OK)
    {
      Error_Handler();
    }

    __HAL_LINKDMA(tim_pwmHandle,hdma[TIM_DMA_ID_CC2],hdma_tim3_ch2);

    /* TIM3_CH3 Init */
    hdma_tim3_ch3.Instance = DMA1_Stream7;
    hdma_tim3_ch3.Init.Channel = DMA_CHANNEL_5;
    hdma_tim3_ch3.Init.Direction = DMA_MEMORY_TO_PERIPH;
    hdma_tim3_ch3.Init.PeriphInc = DMA_PINC_DISABLE;
    hdma_tim3_ch3.Init.MemInc = DMA_MINC_ENABLE;
    hdma_tim3_ch3.Init.PeriphDataAlignment = DMA_PDATAALIGN_WORD;
    hdma_tim3_ch3.Init.MemDataAlignment = DMA_MDATAALIGN_WORD;
    hdma_tim3_ch3.Init.Mode = DMA_NORMAL;
    hdma_tim3_ch3.Init.Priority = DMA_PRIORITY_LOW;
    hdma_tim3_ch3.Init.FIFOMode = DMA_FIFOMODE_DISABLE;
    if (HAL_DMA_Init(&hdma_tim3_ch3) != HAL_OK)
    {
      Error_Handler();
    }

    __HAL_LINKDMA(tim_pwmHandle,hdma[TIM_DMA_ID_CC3],hdma_tim3_ch3);

  /* USER CODE BEGIN TIM3_MspInit 1 */

  /* USER CODE END TIM3_MspInit 1 */
  }
  else if(tim_pwmHandle->Instance==TIM5)
  {
  /* USER CODE BEGIN TIM5_MspInit 0 */

  /* USER CODE END TIM5_MspInit 0 */
    /* TIM5 clock enable */
    __HAL_RCC_TIM5_CLK_ENABLE();

    /* TIM5 DMA Init */
    /* TIM5_CH1 Init */
    hdma_tim5_ch1.Instance = DMA1_Stream2;
    hdma_tim5_ch1.Init.Channel = DMA_CHANNEL_6;
    hdma_tim5_ch1.Init.Direction = DMA_MEMORY_TO_PERIPH;
    hdma_tim5_ch1.Init.PeriphInc = DMA_PINC_DISABLE;
    hdma_tim5_ch1.Init.MemInc = DMA_MINC_ENABLE;
    hdma_tim5_ch1.Init.PeriphDataAlignment = DMA_PDATAALIGN_WORD;
    hdma_tim5_ch1.Init.MemDataAlignment = DMA_MDATAALIGN_WORD;
    hdma_tim5_ch1.Init.Mode = DMA_NORMAL;
    hdma_tim5_ch1.Init.Priority = DMA_PRIORITY_VERY_HIGH;
    hdma_tim5_ch1.Init.FIFOMode = DMA_FIFOMODE_DISABLE;
    if (HAL_DMA_Init(&hdma_tim5_ch1) != HAL_OK)
    {
      Error_Handler();
    }

    __HAL_LINKDMA(tim_pwmHandle,hdma[TIM_DMA_ID_CC1],hdma_tim5_ch1);

    /* TIM5_CH2 Init */
    hdma_tim5_ch2.Instance = DMA1_Stream4;
    hdma_tim5_ch2.Init.Channel = DMA_CHANNEL_6;
    hdma_tim5_ch2.Init.Direction = DMA_MEMORY_TO_PERIPH;
    hdma_tim5_ch2.Init.PeriphInc = DMA_PINC_DISABLE;
    hdma_tim5_ch2.Init.MemInc = DMA_MINC_ENABLE;
    hdma_tim5_ch2.Init.PeriphDataAlignment = DMA_PDATAALIGN_WORD;
    hdma_tim5_ch2.Init.MemDataAlignment = DMA_MDATAALIGN_WORD;
    hdma_tim5_ch2.Init.Mode = DMA_NORMAL;
    hdma_tim5_ch2.Init.Priority = DMA_PRIORITY_VERY_HIGH;
    hdma_tim5_ch2.Init.FIFOMode = DMA_FIFOMODE_DISABLE;
    if (HAL_DMA_Init(&hdma_tim5_ch2) != HAL_OK)
    {
      Error_Handler();
    }

    __HAL_LINKDMA(tim_pwmHandle,hdma[TIM_DMA_ID_CC2],hdma_tim5_ch2);

    /* TIM5_CH3_UP Init */
    hdma_tim5_ch3_up.Instance = DMA1_Stream0;
    hdma_tim5_ch3_up.Init.Channel = DMA_CHANNEL_6;
    hdma_tim5_ch3_up.Init.Direction = DMA_MEMORY_TO_PERIPH;
    hdma_tim5_ch3_up.Init.PeriphInc = DMA_PINC_DISABLE;
    hdma_tim5_ch3_up.Init.MemInc = DMA_MINC_ENABLE;
    hdma_tim5_ch3_up.Init.PeriphDataAlignment = DMA_PDATAALIGN_WORD;
    hdma_tim5_ch3_up.Init.MemDataAlignment = DMA_MDATAALIGN_WORD;
    hdma_tim5_ch3_up.Init.Mode = DMA_NORMAL;
    hdma_tim5_ch3_up.Init.Priority = DMA_PRIORITY_VERY_HIGH;
    hdma_tim5_ch3_up.Init.FIFOMode = DMA_FIFOMODE_DISABLE;
    if (HAL_DMA_Init(&hdma_tim5_ch3_up) != HAL_OK)
    {
      Error_Handler();
    }

    /* Several peripheral DMA handle pointers point to the same DMA handle.
     Be aware that there is only one stream to perform all the requested DMAs. */
    __HAL_LINKDMA(tim_pwmHandle,hdma[TIM_DMA_ID_CC3],hdma_tim5_ch3_up);
    __HAL_LINKDMA(tim_pwmHandle,hdma[TIM_DMA_ID_UPDATE],hdma_tim5_ch3_up);

    /* TIM5_CH4_TRIG Init */
    hdma_tim5_ch4_trig.Instance = DMA1_Stream1;
    hdma_tim5_ch4_trig.Init.Channel = DMA_CHANNEL_6;
    hdma_tim5_ch4_trig.Init.Direction = DMA_MEMORY_TO_PERIPH;
    hdma_tim5_ch4_trig.Init.PeriphInc = DMA_PINC_DISABLE;
    hdma_tim5_ch4_trig.Init.MemInc = DMA_MINC_ENABLE;
    hdma_tim5_ch4_trig.Init.PeriphDataAlignment = DMA_PDATAALIGN_WORD;
    hdma_tim5_ch4_trig.Init.MemDataAlignment = DMA_MDATAALIGN_WORD;
    hdma_tim5_ch4_trig.Init.Mode = DMA_NORMAL;
    hdma_tim5_ch4_trig.Init.Priority = DMA_PRIORITY_VERY_HIGH;
    hdma_tim5_ch4_trig.Init.FIFOMode = DMA_FIFOMODE_DISABLE;
    if (HAL_DMA_Init(&hdma_tim5_ch4_trig) != HAL_OK)
    {
      Error_Handler();
    }

    /* Several peripheral DMA handle pointers point to the same DMA handle.
     Be aware that there is only one stream to perform all the requested DMAs. */
    __HAL_LINKDMA(tim_pwmHandle,hdma[TIM_DMA_ID_CC4],hdma_tim5_ch4_trig);
    __HAL_LINKDMA(tim_pwmHandle,hdma[TIM_DMA_ID_TRIGGER],hdma_tim5_ch4_trig);

  /* USER CODE BEGIN TIM5_MspInit 1 */

  /* USER CODE END TIM5_MspInit 1 */
  }
}
void HAL_TIM_MspPostInit(TIM_HandleTypeDef* timHandle)
{

  GPIO_InitTypeDef GPIO_InitStruct = {0};
  if(timHandle->Instance==TIM3)
  {
  /* USER CODE BEGIN TIM3_MspPostInit 0 */

  /* USER CODE END TIM3_MspPostInit 0 */
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    /**TIM3 GPIO Configuration
    PA7     ------> TIM3_CH2
    PB0     ------> TIM3_CH3
    */
    GPIO_InitStruct.Pin = GPIO_PIN_7;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.Alternate = GPIO_AF2_TIM3;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_PIN_0;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.Alternate = GPIO_AF2_TIM3;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /* USER CODE BEGIN TIM3_MspPostInit 1 */

  /* USER CODE END TIM3_MspPostInit 1 */
  }
  else if(timHandle->Instance==TIM5)
  {
  /* USER CODE BEGIN TIM5_MspPostInit 0 */

  /* USER CODE END TIM5_MspPostInit 0 */

    __HAL_RCC_GPIOA_CLK_ENABLE();
    /**TIM5 GPIO Configuration
    PA0     ------> TIM5_CH1
    PA1     ------> TIM5_CH2
    PA2     ------> TIM5_CH3
    PA3     ------> TIM5_CH4
    */
    GPIO_InitStruct.Pin = GPIO_PIN_0|GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.Alternate = GPIO_AF2_TIM5;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /* USER CODE BEGIN TIM5_MspPostInit 1 */

  /* USER CODE END TIM5_MspPostInit 1 */
  }

}

void HAL_TIM_PWM_MspDeInit(TIM_HandleTypeDef* tim_pwmHandle)
{

  if(tim_pwmHandle->Instance==TIM3)
  {
  /* USER CODE BEGIN TIM3_MspDeInit 0 */

  /* USER CODE END TIM3_MspDeInit 0 */
    /* Peripheral clock disable */
    __HAL_RCC_TIM3_CLK_DISABLE();

    /* TIM3 DMA DeInit */
    HAL_DMA_DeInit(tim_pwmHandle->hdma[TIM_DMA_ID_CC2]);
    HAL_DMA_DeInit(tim_pwmHandle->hdma[TIM_DMA_ID_CC3]);
  /* USER CODE BEGIN TIM3_MspDeInit 1 */

  /* USER CODE END TIM3_MspDeInit 1 */
  }
  else if(tim_pwmHandle->Instance==TIM5)
  {
  /* USER CODE BEGIN TIM5_MspDeInit 0 */

  /* USER CODE END TIM5_MspDeInit 0 */
    /* Peripheral clock disable */
    __HAL_RCC_TIM5_CLK_DISABLE();

    /* TIM5 DMA DeInit */
    HAL_DMA_DeInit(tim_pwmHandle->hdma[TIM_DMA_ID_CC1]);
    HAL_DMA_DeInit(tim_pwmHandle->hdma[TIM_DMA_ID_CC2]);
    HAL_DMA_DeInit(tim_pwmHandle->hdma[TIM_DMA_ID_CC3]);
    HAL_DMA_DeInit(tim_pwmHandle->hdma[TIM_DMA_ID_UPDATE]);
    HAL_DMA_DeInit(tim_pwmHandle->hdma[TIM_DMA_ID_CC4]);
    HAL_DMA_DeInit(tim_pwmHandle->hdma[TIM_DMA_ID_TRIGGER]);
  /* USER CODE BEGIN TIM5_MspDeInit 1 */

  /* USER CODE END TIM5_MspDeInit 1 */
  }
}

/* USER CODE BEGIN 1 */

/* USER CODE END 1 */
