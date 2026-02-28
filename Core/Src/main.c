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
#include "dma.h"
#include "fatfs.h"
#include "sdio.h"
#include "spi.h"
#include "tim.h"
#include "usart.h"
#include "usb_device.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "led_driver1.h"
#include "led_driver2.h"
#include "tf_driver.h"
#include <stdio.h>
#include "usbd_core.h"   
#include "usb_device.h"
#include "usbd_msc.h"
#include "usbd_msc_scsi.h"
#include "lcd_driver.h"

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
// 添加串口接收相关变量
uint8_t rx_byte; // 用于接收 1 个字节的临时变量


volatile SystemMode app_mode = MODE_USB_MSC; // 默认 USB
volatile uint8_t key_pressed = 0; // 按键标志位


extern USBD_HandleTypeDef hUsbDeviceFS; // <--- 新增，解决 hUsbDeviceFS 报错
// 记录上次 USB 设备状态以便检测变化
volatile uint8_t last_usb_dev_state = 0;
// 记录上次 MSC 媒体状态以便检测主机在资源管理器中弹出
volatile uint8_t last_scsi_medium_state = 0xFF;


/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

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
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_SDIO_SD_Init();
  MX_USART1_UART_Init();
  MX_FATFS_Init();
  MX_TIM5_Init();
  MX_USB_DEVICE_Init();
  MX_SPI3_Init();
  MX_TIM3_Init();
  /* USER CODE BEGIN 2 */

  HAL_UART_Receive_IT(&huart1, &rx_byte, 1);// 启动 USART1 中断接收

  // 0.初始化 LED 数据结构 (我觉得没有必要初始化，如果初始化会生成初始buffer，占用内存比较大)
  // LED2_Init(); // 建议显式调用
  // LED1_Init(); // 必须调用，清空 TIM3 的缓冲区


  // 1. 初始化 TF 卡底层 (如果不初始化，USB 也没法用)
  // 注意：在 USB 模式下不需要 mount，但底层硬件要 init
  // 这里可以先做一个简单的 TF_Init 检查卡是否在
  if (TF_Init() != 0) {
      printf("TF Card Error!\r\n");
  }
  
  // 2. 默认进入 USB 模式
  // 手动调用一次初始化，确保卡进入 Transfer 状态
  // 这样当 USB 插入时，STORAGE_Init_FS 发现状态是对的，就会直接返回
  if (HAL_SD_Init(&hsd) == HAL_OK)
  {

    printf("SD Card Pre-Init OK.\r\n");
    // 可以在这里顺手把速度提上去
    __HAL_SD_DISABLE(&hsd);
     hsd.Init.ClockDiv = 4; // 8MHz
    SDIO->CLKCR = (SDIO->CLKCR & 0xFFFFFF00) | 4;
     __HAL_SD_ENABLE(&hsd);
  } 
  else 
  {
    printf("SD Card Pre-Init Failed!\r\n");
  }
  MX_USB_DEVICE_Init(); 
  HAL_Delay(2000);
  printf("System Start: USB Mode\r\n");
  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_1, GPIO_PIN_RESET); // 绿灯亮 (USB)
  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_0, GPIO_PIN_SET);   // 红灯灭





  // 3.初始化LCD屏幕
  LCD_Init();
  LCD_Display_Status(app_mode);







  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {



      // 核心状态切换机制
      if (key_pressed)
      {
          key_pressed = 0;
          HAL_Delay(100); 

          // === 模式切换逻辑 ===
          if (app_mode == MODE_USB_MSC)
          {
              printf("[Switch] Switching to PLAYER Mode...\r\n");
              
              // 1. 停止 USB
              USBD_Stop(&hUsbDeviceFS);
              HAL_Delay(500); 
              
              // 2. 【关键修正】在重新挂载前，把 SDIO 速度降回 400kHz
              // 否则 f_mount 里的初始化可能会因为速度太快而失败
              __HAL_SD_DISABLE(&hsd);
              hsd.Init.ClockDiv = 118; // 降回 400kHz (48MHz / 120)
              SDIO->CLKCR = (SDIO->CLKCR & 0xFFFFFF00) | 118; 
              __HAL_SD_ENABLE(&hsd);
              
              // 3. 重新挂载 FatFs
              f_mount(NULL, TF_PATH, 0); 
              if (f_mount(&fs_tf, TF_PATH, 1) == FR_OK) {
                  printf("[Switch] FatFs Mount OK (Low Speed)\r\n");
                  
                  // 4. 【再次提速】挂载成功后，再次提速以便流畅播放 CSV
                  __HAL_SD_DISABLE(&hsd);
                  hsd.Init.ClockDiv = 4; // 提速到 8MHz
                  SDIO->CLKCR = (SDIO->CLKCR & 0xFFFFFF00) | 4; 
                  __HAL_SD_ENABLE(&hsd);
                  printf("[Switch] SDIO Speed Up to 8MHz\r\n");
                  
              } else {
                  printf("[Switch] FatFs Mount Error!\r\n");
              }
              
              // 5. 切换状态
              app_mode = MODE_PLAYER;
              stop_play_flag = 0; 
              
              HAL_GPIO_WritePin(GPIOC, GPIO_PIN_1, GPIO_PIN_SET);  //绿灯灭 
              HAL_GPIO_WritePin(GPIOC, GPIO_PIN_0, GPIO_PIN_RESET); //红灯亮
              // 更新LCD显示
              LCD_Display_Status(app_mode);
              // 显示 Player 模式下已识别的 c1..c4
              LCD_Display_PlayerFiles();
          }
          else
          {
              // [切换到 USB 模式]
              printf("Switching to USB Mode...\r\n");
              
              // 1. 停止播放
              stop_play_flag = 1; 
              // 2. 卸载 FatFs (释放 TF 卡控制权)
              f_mount(NULL, TF_PATH, 0);
              // 3. 启动 USB
              USBD_Start(&hUsbDeviceFS);
              
              app_mode = MODE_USB_MSC;
              HAL_GPIO_WritePin(GPIOC, GPIO_PIN_1, GPIO_PIN_RESET); // 绿灯亮
              HAL_GPIO_WritePin(GPIOC, GPIO_PIN_0, GPIO_PIN_SET);   // 红灯灭
              // 更新LCD显示
              LCD_Display_Status(app_mode);
          }
      }

      // === 播放模式下的任务 ===
      if (app_mode == MODE_PLAYER)
      {
          // 只有当没有按下停止键时才播放
          // 注意：TF_PlayCSV_Multi 是阻塞的，它里面会自己检查 stop_play_flag
          // 如果按键按下，stop_play_flag 变 1，函数立即返回
          if (stop_play_flag == 0) 
          {
               TF_PlayCSV_Multi(100); // 循环播放，直到被按键打断
          }

          // 如果函数返回了，说明被按键打断了。
          // 此时什么都不用做，下一轮循环会进入上面的 key_pressed 处理逻辑，切回 USB。
          
          
      }
      
      // === USB 模式下的任务 ===
      if (app_mode == MODE_USB_MSC)
      {
          // 如果 USB 设备状态发生了变化，更新 LCD 显示
          if (hUsbDeviceFS.dev_state != last_usb_dev_state) {
            last_usb_dev_state = hUsbDeviceFS.dev_state;
            LCD_Display_Status(app_mode);
          }

          // 轮询 MSC 的 scsi_medium_state（若 MSC 已注册）以检测主机在资源管理器中的弹出操作
          uint8_t cur_scsi = 0xFF;
          for (uint8_t i = 0; i < USBD_MAX_SUPPORTED_CLASS; i++) {
            if (hUsbDeviceFS.pClassDataCmsit[i] != NULL) {
              USBD_MSC_BOT_HandleTypeDef *hmsc = (USBD_MSC_BOT_HandleTypeDef *)hUsbDeviceFS.pClassDataCmsit[i];
              if (hmsc != NULL) {
                cur_scsi = hmsc->scsi_medium_state;
                break;
              }
            }
          }
          if (cur_scsi != 0xFF && cur_scsi != last_scsi_medium_state) {
            last_scsi_medium_state = cur_scsi;
            LCD_Display_Status(app_mode);
          }

          
          HAL_Delay(100);
      }

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
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 4;
  RCC_OscInitStruct.PLL.PLLN = 96;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 4;
  RCC_OscInitStruct.PLL.PLLR = 2;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_3) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

/* GCC 编译器重定向 printf 需要重写 _write 函数 */
int _write(int file, char *ptr, int len)
{
    // 将数据通过串口1发送，使用轮询模式，超时时间设为 1000ms
    HAL_UART_Transmit(&huart1, (uint8_t *)ptr, len, 1000);
    return len;
}


// 串口接收完成回调函数
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
	
	
		//HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_0);  //闪烁红灯
    // 判断是不是 USART1 触发的
    if (huart->Instance == USART1)
    {
			
				//HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_0);  //闪烁红灯
			   
				// 收到任意字符，置位标志
    
        
        // 翻转 LED 提示收到命令
       // HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_0);   //闪烁红灯
			 //HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_0); // 修复：使用 GPIO_PIN_0 而不是 0，且改为 Toggle 便于中断中提示 LED 状态
			
				
			

			  //HAL_UART_Transmit(&huart1, &rx_byte, 1, HAL_MAX_DELAY);
			
      HAL_UART_Receive_IT(&huart1, &rx_byte, 1);
    }
}


// 按键中断 (PC13)
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    // HAL_GPIO_WritePin(GPIOC, 1, GPIO_PIN_RESET);
    if (GPIO_Pin == GPIO_PIN_13)
    {
			//HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_1);  //闪烁绿灯
			//HAL_GPIO_WritePin(GPIOC, GPIO_PIN_1, GPIO_PIN_RESET);
      
      key_pressed = 1;
      // 如果正在播放，立即打断
      stop_play_flag = 1; 
			


    }
}


// PWM 传输完成回调
void HAL_TIM_PWM_PulseFinishedCallback(TIM_HandleTypeDef *htim)
{
    if (htim->Instance == TIM5)
    {
        //HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_0);
        //HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_2);
        if (htim->Channel == HAL_TIM_ACTIVE_CHANNEL_1)
        {
            //HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_0);
            HAL_TIM_PWM_Stop_DMA(&htim5, TIM_CHANNEL_1);
            dma_finished_1 = 1;
        }
        else if (htim->Channel == HAL_TIM_ACTIVE_CHANNEL_2)
        { 
            //HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_1);
            HAL_TIM_PWM_Stop_DMA(&htim5, TIM_CHANNEL_2);
            dma_finished_2 = 1;
        }
        else if (htim->Channel == HAL_TIM_ACTIVE_CHANNEL_3)
        {
            //HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_2);
            HAL_TIM_PWM_Stop_DMA(&htim5, TIM_CHANNEL_3);
            dma_finished_3 = 1;
        }
        else if (htim->Channel == HAL_TIM_ACTIVE_CHANNEL_4)
        {
            HAL_TIM_PWM_Stop_DMA(&htim5, TIM_CHANNEL_4);
            dma_finished_4 = 1;
        }
    }
    // 处理 TIM3 (新增逻辑)
    if (htim->Instance == TIM3) {
        // 注意：led_driver1 用的是 TIM3 CH2 和 CH3
        if (htim->Channel == HAL_TIM_ACTIVE_CHANNEL_2) { // 对应 CH5
            HAL_TIM_PWM_Stop_DMA(htim, TIM_CHANNEL_2);
            dma_finished_5 = 1;
        }
        if (htim->Channel == HAL_TIM_ACTIVE_CHANNEL_3) { // 对应 CH6
            HAL_TIM_PWM_Stop_DMA(htim, TIM_CHANNEL_3);
            dma_finished_6 = 1;
        }
    }
}

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
    printf("During the Error_Handler!\r\n");
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
