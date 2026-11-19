 /**
  ******************************************************************************
  * @file    Templates/Src/main.c 
  * @author  MCD Application Team
  * @brief   STM32F7xx HAL API Template project 
  *
  * @note    modified by ARM
  *          The modifications allow to use this file as User Code Template
  *          within the Device Family Pack.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2016 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "rl_net.h"


extern void BSP_SDRAM_Init(void);

DMA2D_HandleTypeDef hdma2d;

#ifdef _RTE_
#include "RTE_Components.h"             // Component selection
#endif
#ifdef RTE_CMSIS_RTOS2                  // when RTE component CMSIS RTOS2 is used
#include "cmsis_os2.h"                  // ::CMSIS:RTOS2
#endif

#ifdef RTE_CMSIS_RTOS2_RTX5
/**
  * @brief  Surcharge de HAL_GetTick() pour utiliser le compteur de ticks du RTOS RTX5.
  *         Avant le démarrage du noyau (osKernelStart), utilise une boucle NOP
  *         d'environ 1 ms pour les délais d'initialisation HAL.
  */
uint32_t HAL_GetTick (void) {
  static uint32_t ticks = 0U;
         uint32_t i;

  if (osKernelGetState () == osKernelRunning) {
    return ((uint32_t)osKernelGetTickCount ()); /* Tick exact du noyau RTX5 (1 ms) */
  }

  /* Avant démarrage du RTOS : temporisation logicielle approximative de 1 ms */
  for (i = (SystemCoreClock >> 14U); i > 0U; i--) {
    __NOP(); __NOP(); __NOP(); __NOP(); __NOP(); __NOP();
    __NOP(); __NOP(); __NOP(); __NOP(); __NOP(); __NOP();
  }
  return ++ticks;
}

/**
  * @brief  Surcharge de HAL_InitTick() : inutile car SysTick est géré par RTX5.
  */
HAL_StatusTypeDef HAL_InitTick(uint32_t TickPriority) {
  
  UNUSED(TickPriority);

  return HAL_OK;
}
#endif

/** @addtogroup STM32F7xx_HAL_Examples
  * @{
  */

/** @addtogroup Templates
  * @{
  */ 

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/
static void MPU_Config(void);
static void SystemClock_Config(void);
static void Error_Handler(void);
static void CPU_CACHE_Enable(void);

/**
  * @brief DMA2D Initialization Function
  * @param None
  * @retval None
  */
static void MX_DMA2D_Init(void)
{

  /* USER CODE BEGIN DMA2D_Init 0 */

  /* USER CODE END DMA2D_Init 0 */

  /* USER CODE BEGIN DMA2D_Init 1 */

  /* USER CODE END DMA2D_Init 1 */
  hdma2d.Instance = DMA2D;
  hdma2d.Init.Mode = DMA2D_M2M;
  hdma2d.Init.ColorMode = DMA2D_OUTPUT_ARGB8888;
  hdma2d.Init.OutputOffset = 0;
  hdma2d.LayerCfg[1].InputOffset = 0;
  hdma2d.LayerCfg[1].InputColorMode = DMA2D_INPUT_ARGB8888;
  hdma2d.LayerCfg[1].AlphaMode = DMA2D_NO_MODIF_ALPHA;
  hdma2d.LayerCfg[1].InputAlpha = 0;
  if (HAL_DMA2D_Init(&hdma2d) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_DMA2D_ConfigLayer(&hdma2d, 1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN DMA2D_Init 2 */

  /* USER CODE END DMA2D_Init 2 */

}

/* Private functions ---------------------------------------------------------*/

/**
  * @brief  Initialisation bas niveau du STM32F746NGH :
  *         - Configuration MPU (protection mémoire SRAM et SDRAM)
  *         - Activation des caches L1 (Instructions + Données)
  *         - Initialisation HAL et SDRAM externe (pour le framebuffer LCD)
  *         - Configuration de l'horloge système à 200 MHz
  */
void configuration_overclock(void)
{
  /* Configure la MPU en mode Write-Through pour la cohérence cache/DMA */
  MPU_Config();

  /* Active les caches L1 I-Cache et D-Cache du Cortex-M7 */
  CPU_CACHE_Enable();

  /* Initialise la couche HAL (ART Accelerator, priorité NVIC groupe 4, MSP) */
  HAL_Init();
	
  /* Initialise la SDRAM externe (utilisée par l'écran LCD LTDC/emWin) */
  BSP_SDRAM_Init();
	
  //MX_DMA2D_Init();
  /* Configure les PLL pour cadencer le système à 200 MHz */
  SystemClock_Config();
  SystemCoreClockUpdate();
}

/**
  * @brief  Configuration de l'horloge système (PLL) :
  *            Source PLL         = HSE (quartz externe 25 MHz)
  *            SYSCLK / HCLK      = 200 MHz (25 / 25 * 400 / 2)
  *            APB1 (TIM6, DAC)   = 50 MHz  (horloge timers APB1 = 100 MHz)
  *            APB2 (TIM10, ADC3) = 100 MHz (horloge timers APB2 = 200 MHz)
  *            Latence Flash      = 7 Wait States
  */
static void SystemClock_Config(void)
{
  RCC_ClkInitTypeDef RCC_ClkInitStruct;
  RCC_OscInitTypeDef RCC_OscInitStruct;

  /* Active l'oscillateur externe HSE (25 MHz) et la PLL */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSIState = RCC_HSI_OFF;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 25U;  /* 25 MHz / 25 = 1 MHz en entrée VCO */
  RCC_OscInitStruct.PLL.PLLN = 400U; /* 1 MHz * 400 = 400 MHz */
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2; /* 400 MHz / 2 = 200 MHz SYSCLK */
  RCC_OscInitStruct.PLL.PLLQ = 9U;
  if(HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /* Active le mode OverDrive nécessaire pour les fréquences >= 180 MHz */
  if(HAL_PWREx_EnableOverDrive() != HAL_OK)
  {
    Error_Handler();
  }

  /* Sélectionne la PLL comme horloge système et configure les diviseurs AHB/APB */
  RCC_ClkInitStruct.ClockType = (RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2);
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;  /* HCLK  = 200 MHz */
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;   /* PCLK1 = 50 MHz  */
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;   /* PCLK2 = 100 MHz */
  if(HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_7) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief  This function is executed in case of error occurrence.
  * @param  None
  * @retval None
  */
static void Error_Handler(void)
{
  /* User may add here some code to deal with this error */
  while(1)
  {
  }
}

/**
  * @brief  Configure the MPU attributes.
  * @param  None
  * @retval None
  */
static void MPU_Config(void)
{
  MPU_Region_InitTypeDef MPU_InitStruct;
  
  /* Disable the MPU */
  HAL_MPU_Disable();

  /* Configure the Embedded SRAM1 as (default): cacheable, write-back, 
     allocate on read or write miss, non-shareable normal memory, execute never

     Note:
     DTCM starting at address 0x20000000 with size of 64 kB, regardless of 
     MPU settings behaves as non-cacheable, non-shareable normal memory */

  MPU_InitStruct.Enable           = MPU_REGION_ENABLE;
  MPU_InitStruct.BaseAddress      = 0x20000000U;
  MPU_InitStruct.Size             = MPU_REGION_SIZE_512KB;
  MPU_InitStruct.AccessPermission = MPU_REGION_FULL_ACCESS;
  MPU_InitStruct.IsBufferable     = MPU_ACCESS_BUFFERABLE;
  MPU_InitStruct.IsCacheable      = MPU_ACCESS_CACHEABLE;
  MPU_InitStruct.IsShareable      = MPU_ACCESS_NOT_SHAREABLE;
  MPU_InitStruct.Number           = MPU_REGION_NUMBER0;
  MPU_InitStruct.TypeExtField     = MPU_TEX_LEVEL1;
  MPU_InitStruct.SubRegionDisable = 0x00U;
  MPU_InitStruct.DisableExec      = MPU_INSTRUCTION_ACCESS_DISABLE;
  HAL_MPU_ConfigRegion(&MPU_InitStruct);

  /* Configure the External SDRAM as: non-cacheable, non-shareable normal memory, execute never

     Note:
     External SDRAM starting at address 0xC0000000, regardless of MPU settings 
     behaves as non-cacheable memory */

  MPU_InitStruct.Enable           = MPU_REGION_ENABLE;
  MPU_InitStruct.BaseAddress      = 0xC0000000U;
  MPU_InitStruct.Size             = MPU_REGION_SIZE_16MB;
  MPU_InitStruct.AccessPermission = MPU_REGION_FULL_ACCESS;
  MPU_InitStruct.IsBufferable     = MPU_ACCESS_NOT_BUFFERABLE;
  MPU_InitStruct.IsCacheable      = MPU_ACCESS_NOT_CACHEABLE;
  MPU_InitStruct.IsShareable      = MPU_ACCESS_NOT_SHAREABLE;
  MPU_InitStruct.Number           = MPU_REGION_NUMBER1;
  MPU_InitStruct.TypeExtField     = MPU_TEX_LEVEL1;
  MPU_InitStruct.SubRegionDisable = 0x00U;
  MPU_InitStruct.DisableExec      = MPU_INSTRUCTION_ACCESS_DISABLE;
  HAL_MPU_ConfigRegion(&MPU_InitStruct);

  /* Enable the MPU */
  HAL_MPU_Enable(MPU_PRIVILEGED_DEFAULT);
}

/**
  * @brief  CPU L1-Cache enable.
  * @param  None
  * @retval None
  */
static void CPU_CACHE_Enable(void)
{
  /* Enable I-Cache */
  SCB_EnableICache();

  /* Enable D-Cache */
  SCB_EnableDCache();
}

#ifdef  USE_FULL_ASSERT

/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t* file, uint32_t line)
{ 
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */

  /* Infinite loop */
  while (1)
  {
  }
}
#endif

/**
  * @}
  */ 

/**
  * @}
  */ 

