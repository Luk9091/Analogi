/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
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
#include "adc.h"
#include "dac.h"
#include "dma.h"
#include "i2c.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "display.h"

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define TRIANGLE_SAMPLE_NUMBER  128
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
uint32_t triangle_LUT[(TRIANGLE_SAMPLE_NUMBER*2)];
int16_t measure_table[4096];
int16_t avg_meas[256];
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

void DAC_trigGen(uint16_t amplitude, uint16_t offset){
    uint16_t step = amplitude / TRIANGLE_SAMPLE_NUMBER;

    triangle_LUT[0] = offset - amplitude/2;
    for (int i = 1; i < TRIANGLE_SAMPLE_NUMBER+1; i++){
        triangle_LUT[i] = triangle_LUT[i-1] + step;
    }
    triangle_LUT[TRIANGLE_SAMPLE_NUMBER]--;

    for (int i = TRIANGLE_SAMPLE_NUMBER+1; i < TRIANGLE_SAMPLE_NUMBER*2; i++){
        triangle_LUT[i] = triangle_LUT[i-1] - step;
    }

}

// int avgMeasure(int sampleNumber, uint16_t zeroScale){
//     int64_t sum = 0;
//     char buff[32];
//     for (int i = 0; i < sampleNumber; i++){
//         HAL_ADC_Start(&hadc2);
//         int32_t sample = 0;
//         if(HAL_ADC_PollForConversion(&hadc2, 10) == HAL_OK){
//             sample = HAL_ADC_GetValue(&hadc2);
//             snprintf(buff, 32, "Tmp:%- 5li", sample);
//             sample -= 512;
//             Display_print(0, 0, buff);
//         } else {
//             Display_print(0, 0, "Error");
//         }
//         sum += sample;
//         Display_draw();
//     }
//     return sum / sampleNumber;
// }
volatile bool adc_measure_wait;

void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef* hadc) {
    if (hadc->Instance == ADC2) {
        adc_measure_wait = false;
    }
}

float avgMeasure(int sampleNumber, uint16_t zeroScale){
    adc_measure_wait = true;
    HAL_ADC_Start_DMA(&hadc2, (uint32_t*)measure_table, sampleNumber);
    while(adc_measure_wait);

    int64_t sum = 0;
    for (int i = 0; i < sampleNumber; i++){
        sum += measure_table[i] - zeroScale;
    }
    return sum / (float)(sampleNumber);

}

void sendBuffer(uint16_t sample_number, int16_t *buffer){
    char buff[16];
    for (uint16_t i = 0; i < sample_number; i++){
        if (i % 16 == 15){
            snprintf(buff, 16, "%i,\n", buffer[i]);
        } else {
            snprintf(buff, 16, "%i,", buffer[i]);
        }
        HAL_UART_Transmit(&huart2, (uint8_t*)buff, strlen(buff), 100);
    }
    HAL_UART_Transmit(&huart2, (uint8_t*)"\n\n", 2, 100);
}

bool dithering_on(){
    char msg[] = "Dithering ON\n";
    HAL_UART_Transmit(&huart2, (uint8_t*)msg, strlen(msg), 1000);
    if(HAL_DAC_Start_DMA(&hdac1, DAC1_CHANNEL_2, triangle_LUT, TRIANGLE_SAMPLE_NUMBER*2, DAC_ALIGN_12B_R) != HAL_OK){
        Display_clear();
        Display_print(0, 0, "DMA error");
        Display_draw();
        Error_Handler();
        return false;
    }
    HAL_Delay(2000);
    return true;
}

void dithering_off(){
    HAL_DAC_Stop_DMA(&hdac1, DAC1_CHANNEL_2);
}

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
  MX_DAC1_Init();
  MX_ADC2_Init();
  MX_I2C2_Init();
  MX_TIM6_Init();
  MX_USART2_UART_Init();
  /* USER CODE BEGIN 2 */
    if (HAL_TIM_Base_Start(&htim6) != HAL_OK){
        Display_clear();
        Display_print(0, 0, "TIM6 error");
        Display_draw();
        Error_Handler();
    }

    Display_Init();
    Display_clear();
    // if (!zeroScaling()){
    //     Display_clear();
    //     Display_print(0, 0, "Zero scaling:");
    //     Display_print(0, 16, "FAIL!");
    //     Display_draw();
    //     Error_Handler();
    // }
    // Display_clear();
    Display_draw();

    DAC_trigGen(4096, 2048);
    // dithering_on();

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
    int      cnt = 4096;
    uint16_t zero_current = roundf(avgMeasure(cnt, 0));
    char buff[32];
    int size = snprintf(buff, 32, "Zero current: %i\n", zero_current);
    HAL_UART_Transmit(&huart2, (uint8_t*)buff, size, 100);
    volatile bool run = true;
    int i = 0;
    while (run)
    {
        HAL_GPIO_TogglePin(STATE_LED_GPIO_Port, STATE_LED_Pin);
        float avg = avgMeasure(cnt, zero_current);
        // sendBuffer(cnt, measure_table);
        int dev = avg;
        int rem = abs(avg * 100);
        if (dev == 0 && avg < 0){
            snprintf(buff, 32, "D0:   -0.%02i", rem);
        } else {
            snprintf(buff, 32, "D0:% 5i.%02i", dev, rem);
        }
        Display_print(0, 0, buff);


        Display_print(0, 16, buff);
        HAL_GPIO_TogglePin(STATE_LED_GPIO_Port, STATE_LED_Pin);
        avg = avgMeasure(cnt, zero_current);
        // sendBuffer(cnt, measure_table);
        dev = avg;
        rem = abs(avg * 100);
        rem = rem % 100;
        if (dev == 0 && avg < 0){
            snprintf(buff, 32, "D1:   -0.%02i", rem);
        } else {
            snprintf(buff, 32, "D1:% 5i.%02i", dev, rem);
        }
        Display_print(0, 16, buff);

        int voltage = avg * 2000 / 25;
        snprintf(buff, 32, "V:% 6iuV", voltage);
        Display_print(0, 32, buff);
        int current = voltage;
        snprintf(buff, 32, "I:% 6inA", current);
        Display_print(0, 48, buff);
        Display_draw();
        HAL_Delay(50);

        avg_meas[i] = avg*100;
        if (i >= 256){
            HAL_UART_Transmit(&huart2, (uint8_t*)"\n", 1, 100);
            sendBuffer(256, avg_meas);
            i = 0;
        } else {
            snprintf(buff, 32, "[%03i]\r", i);
            HAL_UART_Transmit(&huart2, (uint8_t*)buff, strlen(buff), 100);
        }
        i++;

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
  HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = RCC_PLLM_DIV1;
  RCC_OscInitStruct.PLL.PLLN = 24;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = RCC_PLLQ_DIV2;
  RCC_OscInitStruct.PLL.PLLR = RCC_PLLR_DIV2;
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
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_3) != HAL_OK)
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
