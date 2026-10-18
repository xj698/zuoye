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
#include "usart.h"
#include "gpio.h"
#include <stdio.h>
#include <string.h>
/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

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
// 1. 定义档位枚举
typedef enum
{
    SPEED_LOW  = 0,
    SPEED_MID  = 1,
    SPEED_HIGH = 2,
    SPEED_COUNT            /* 自动等于 3，方便做循环和数组大小 */
} speed_t;

// 2. 定义闪烁参数的结构体
typedef struct
{
    uint16_t on_ms;
    uint16_t off_ms;
} blink_t;

// 3. 核心配置表（只读），想改速度只改这里
static const blink_t speed_table[SPEED_COUNT] =
{
    /* 想改速度？只改这一张表 */
    { 800u, 800u },        /* 0：LOW  - 慢闪 */
    { 300u, 300u },        /* 1：MID  - 中速 */
    { 120u, 120u }         /* 2：HIGH - 快闪 */
};

// 4. 全局变量
speed_t current_speed = SPEED_LOW; // 当前速度
uint32_t last_led_tick = 0;        // LED时间戳
uint8_t  led_state = 0;            // LED当前状态：0-灭，1-亮
uint32_t key_press_count = 0;      // 记录按键按下的总次数
char uart_buf[64];                 // 串口格式化缓冲区

// 5. 按键专属变量
uint32_t key_press_tick = 0;       // 记录按键真正按下的那一瞬间的时间戳
uint8_t  key_pressed_flag = 0;     // 标记按键当前是否正被按住
uint8_t  key_released = 1;         // 标记按键当前是否处于松开状态（用于消抖）
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
  MX_USART1_UART_Init();
  /* USER CODE BEGIN 2 */
  // 开机提示
   sprintf(uart_buf, "System Boot: mode=SPEED_LOW\r\n");
   HAL_UART_Transmit(&huart1, (uint8_t *)uart_buf, strlen(uart_buf), 100);
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

      uint32_t current_tick = HAL_GetTick();

      // ==========================================
      // 1. 按键扫描：区分长按和短按
      // ==========================================
      if (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_0) == GPIO_PIN_RESET)
      {
          // --- 阶段A：按键刚按下的瞬间 ---
          if (key_released)
          {
              key_press_tick = current_tick; // 记录按下的时间
              key_released = 0;              // 标记现在不是松开状态了
              key_pressed_flag = 1;          // 标记按键正被按住
          }
      }
      else
      {
          // --- 阶段B：按键被松开了 ---
          // 如果刚才在按着(key_pressed_flag==1)，现在松开了，并且稳定了20ms(防抖)
          if (key_pressed_flag && (current_tick - key_press_tick > 300))
          {
              uint32_t press_duration = current_tick - key_press_tick; // 计算按了多久

              if (press_duration >= 1000)
              {
                  // ==========================================
                  // 【长按逻辑】：灯快闪两下提示，不切换模式
                  // ==========================================
                  for(int i = 0; i < 2; i++) {
                      HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_SET);
                      HAL_Delay(100);
                      HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_RESET);
                      HAL_Delay(100);
                  }

                  // 恢复LED状态并重置计时，防止状态错乱
                  led_state = 0;
                  last_led_tick = HAL_GetTick();

                  snprintf(uart_buf, sizeof(uart_buf), "Long Press! (%lums) No Mode Change.\r\n", press_duration);
                  HAL_UART_Transmit(&huart1, (uint8_t *)uart_buf, strlen(uart_buf), 100);
              }
              else
              {
                  // ==========================================
                  // 【短按逻辑】：正常切换模式
                  // ==========================================
                  current_speed = (current_speed + 1) % SPEED_COUNT;
                  key_press_count++;

                  const char* speed_names[] = {"SPEED_LOW", "SPEED_MID", "SPEED_HIGH"};
                  snprintf(uart_buf, sizeof(uart_buf), "Short Press! Total: %lu | mode=%s\r\n",
                           key_press_count, speed_names[current_speed]);
                  HAL_UART_Transmit(&huart1, (uint8_t *)uart_buf, strlen(uart_buf), 100);

                  // 切换模式时，强制重置LED状态
                  if (current_speed == SPEED_HIGH)
                  {
                      HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_SET);
                      led_state = 1;
                  }
                  else
                  {
                      HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_RESET);
                      led_state = 0;
                  }
              }

              // 动作处理完了，复位标志位，准备迎接下一次按键
              key_pressed_flag = 0;
              key_released = 1;
          }
          else if (key_pressed_flag == 0)
          {
              key_released = 1; // 确保空闲时标志位是对的
          }
      }

      // ==========================================
      // 2. LED 查表控制逻辑（没有任何魔法数字）
      // ==========================================
      if (current_speed == SPEED_HIGH)
      {
          // 常亮模式
          HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_SET);
          led_state = 1;
      }
      else
      {
          // 闪烁模式（从表里取出对应的时间）
          uint16_t wait_time = speed_table[current_speed].on_ms;

          if (current_tick - last_led_tick >= wait_time)
          {
              last_led_tick = current_tick;
              led_state = !led_state; // 状态翻转
              HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, led_state ? GPIO_PIN_SET : GPIO_PIN_RESET);
          }
      }

  /* USER CODE END 3 */
/* USER CODE BEGIN 3 */
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

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
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

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

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
