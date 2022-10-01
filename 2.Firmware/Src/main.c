/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2020 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under Ultimate Liberty license
  * SLA0044, the "License"; You may not use this file except in compliance with
  * the License. You may obtain a copy of the License at:
  *                             www.st.com/SLA0044
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "pdm2pcm.h"
#include "usbd_audio.h"
#include "usb_device.h"

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

CRC_HandleTypeDef hcrc;

SAI_HandleTypeDef hsai_BlockA1;
DMA_HandleTypeDef hdma_sai1_a;

/* USER CODE BEGIN PV */
volatile uint8_t mic_num;

volatile uint8_t record_en;
volatile uint8_t in_mute;
volatile uint16_t in_volumn;
volatile uint32_t in_freq; 
volatile uint8_t in_play_en;

volatile uint16_t in_rd_ptr;  
volatile uint16_t in_wr_ptr; 
volatile uint8_t in_choose;
volatile uint16_t in_remain_len;
volatile uint8_t collect_xfrc;
uint32_t DataInbuffer[AUDIO_IN_BUF_WORD];//total buffer 
uint32_t PDMDatabufferX[PDM_IN_PACKET / 2];//PDM buffer
uint16_t xin_remain_len;

#ifdef ADAPTION_ENABLE
uint32_t DataInbufferP[AUDIO_IN_4MIC_PACKET / 4 + AUDIO_IN_4MIC_PACKET / 16];//Packet buffer
#else
uint32_t DataInbufferP[AUDIO_IN_4MIC_PACKET / 4];
#endif


volatile uint8_t firstin_xfrc;

uint8_t state_in_full;
uint8_t state_in_empty;
uint8_t state_in_freq;
uint8_t state_in_dmae;
uint8_t state_in_flhin;
uint8_t state_in_trans;
uint16_t state_in_upadj;
uint16_t state_in_dnadj;

uint16_t in_count = 0;
uint16_t sof_count= 0;
uint16_t sai_countl = 0;
uint16_t sai_counth = 0;

uint16_t xin_count;
uint16_t xsof_count;
uint16_t xsai_countl = 0;
uint16_t xsai_counth = 0;

#ifdef MYDEBUG		
uint16_t xcount;
uint32_t freq[64];
uint32_t time[64];
uint8_t  type[64];
uint32_t in_max_real;
uint32_t in_min_real;
#endif
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MPU_Config(void);
static void MX_GPIO_Init(void);
static void MX_DMA_Init(void);
static void MX_CRC_Init(void);
static void MX_SAI1_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
void HAL_SAI_RxHalfCpltCallback(SAI_HandleTypeDef *hsai)
{
	if(hsai->Instance == SAI1_Block_A)
	{
		sai_countl++;
		in_choose = 0x00;
		collect_xfrc = 0x01;
	}
}

void HAL_SAI_RxCpltCallback(SAI_HandleTypeDef *hsai)
{
	if(hsai->Instance == SAI1_Block_A)
	{
		sai_counth++;
		in_choose = 0x01;
		collect_xfrc = 0x01;
	}
}

void MIC_Number_Init()
{
	if(GPIO_PIN_RESET == HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_11))
	{
		mic_num = 2;
		HAL_GPIO_WritePin(GPIOC, GPIO_PIN_10, GPIO_PIN_RESET);
	}
	else
	{
		mic_num = 4;
		HAL_GPIO_WritePin(GPIOC, GPIO_PIN_10, GPIO_PIN_SET);
	}
}
/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* USER CODE BEGIN 1 */

	uint16_t* PDMDatabuffer0;
	uint16_t* PDMDatabuffer1;
  /* USER CODE END 1 */
  

  /* MPU Configuration--------------------------------------------------------*/
  MPU_Config();
  
  /* Enable I-Cache---------------------------------------------------------*/
  SCB_EnableICache();

  /* Enable D-Cache---------------------------------------------------------*/
//  SCB_EnableDCache();

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */
  MX_GPIO_Init();
	MIC_Number_Init();
  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_DMA_Init();
  MX_CRC_Init();
  MX_SAI1_Init();
  MX_PDM2PCM_Init(mic_num);
	
	in_mute = 0;
	in_volumn = 0;
	record_en = 0x00;
	
	collect_xfrc = 0;
	firstin_xfrc = 0;

  /* USER CODE BEGIN 2 */
	state_in_full = 0;
	state_in_empty = 0;
	state_in_freq = 0;
	state_in_dmae = 0;
	state_in_flhin = 0;
	state_in_trans = 0;
	state_in_upadj = 0;
	state_in_dnadj = 0;
	
#ifdef MYDEBUG
	in_max_real = (uint16_t)(AUDIO_IN_BUF_WORD / 2);
	in_min_real = (uint16_t)(AUDIO_IN_BUF_WORD / 2);
#endif
	
  MX_USB_DEVICE_Init();
  /* USER CODE BEGIN 2 */

	PDMDatabuffer0 = (uint16_t *)(&PDMDatabufferX[0]);
	PDMDatabuffer1 = (uint16_t *)(&PDMDatabufferX[PDM_IN_PACKET / 4]);
  /* USER CODE END 2 */
 
 

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */
#if 0
		in_count = 0;
		sof_count = 0;
		sai_countl = 0;
		sai_counth = 0;
		HAL_Delay(1000);
		xin_count = in_count;
		xsof_count = sof_count;
		xsai_countl = sai_countl;
		xsai_counth = sai_counth;
#endif
    /* USER CODE BEGIN 3 */
		
#if 1	
		if(collect_xfrc == 1)//SAI COLLECT
		{	
			collect_xfrc = 0;
			
			//PDM to PCM
			if(mic_num == 2)
			{
				if(in_choose == 0x00)
					PDM_To_PCM_2(PDMDatabuffer0, (uint16_t *)(&DataInbuffer[in_wr_ptr & (uint16_t)(AUDIO_IN_BUF_WORD - 1)]));
				else
					PDM_To_PCM_2(PDMDatabuffer1, (uint16_t *)(&DataInbuffer[in_wr_ptr & (uint16_t)(AUDIO_IN_BUF_WORD - 1)]));
			}
			else if(mic_num == 4)
			{
				if(in_choose == 0x00)
					PDM_To_PCM_4(PDMDatabuffer0, (uint16_t *)(&DataInbuffer[in_wr_ptr & (uint16_t)(AUDIO_IN_BUF_WORD - 1)]));
				else
					PDM_To_PCM_4(PDMDatabuffer1, (uint16_t *)(&DataInbuffer[in_wr_ptr & (uint16_t)(AUDIO_IN_BUF_WORD - 1)]));
			}
			
			//Update wr_ptr
			in_wr_ptr += (uint16_t)(PDM_IN_PACKET / 2 / 4);
			
			//Check Full
			if((uint16_t)(in_wr_ptr - in_rd_ptr) >= (uint16_t)(AUDIO_IN_BUF_WORD - AUDIO_IN_4MIC_PACKET / 4))//FULL
			{
				init_in_parameter();
#ifdef MYDEBUG	
				time[xcount] = HAL_GetTick();
				type[xcount] = 0x01;
				xcount++;
				if(xcount == 64)
					xcount = 63;
#endif
				HAL_GPIO_WritePin(GPIOD, GPIO_PIN_0, GPIO_PIN_RESET);
				state_in_full++;
			}
		}
#endif
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
  RCC_PeriphCLKInitTypeDef PeriphClkInitStruct = {0};

  /** Supply configuration update enable 
  */
  HAL_PWREx_ConfigSupply(PWR_LDO_SUPPLY);
  /** Configure the main internal regulator output voltage 
  */
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE0);

  while(!__HAL_PWR_GET_FLAG(PWR_FLAG_VOSRDY)) {}
  /** Macro to configure the PLL clock source 
  */
  __HAL_RCC_PLL_PLLSOURCE_CONFIG(RCC_PLLSOURCE_HSE);
  /** Initializes the CPU, AHB and APB busses clocks 
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 8;
  RCC_OscInitStruct.PLL.PLLN = 312;
  RCC_OscInitStruct.PLL.PLLP = 2;///////////////////////////////
  RCC_OscInitStruct.PLL.PLLQ = 20;
  RCC_OscInitStruct.PLL.PLLR = 2;
  RCC_OscInitStruct.PLL.PLLRGE = RCC_PLL1VCIRANGE_1;
  RCC_OscInitStruct.PLL.PLLVCOSEL = RCC_PLL1VCOWIDE;
  RCC_OscInitStruct.PLL.PLLFRACN = 0;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }
  /** Initializes the CPU, AHB and APB busses clocks 
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2
                              |RCC_CLOCKTYPE_D3PCLK1|RCC_CLOCKTYPE_D1PCLK1;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.SYSCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB3CLKDivider = RCC_APB3_DIV2;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_APB1_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_APB2_DIV2;
  RCC_ClkInitStruct.APB4CLKDivider = RCC_APB4_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_4) != HAL_OK)
  {
    Error_Handler();
  }
  PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_SAI1|RCC_PERIPHCLK_USB;
  PeriphClkInitStruct.PLL2.PLL2M = 24;
  PeriphClkInitStruct.PLL2.PLL2N = 192;
	if(mic_num == 2)
		PeriphClkInitStruct.PLL2.PLL2P = 32;
	if(mic_num == 4)
		PeriphClkInitStruct.PLL2.PLL2P = 16;
  PeriphClkInitStruct.PLL2.PLL2Q = 2;
  PeriphClkInitStruct.PLL2.PLL2R = 2;
  PeriphClkInitStruct.PLL2.PLL2RGE = RCC_PLL2VCIRANGE_0;
  PeriphClkInitStruct.PLL2.PLL2VCOSEL = RCC_PLL2VCOWIDE;
  PeriphClkInitStruct.PLL2.PLL2FRACN = 0;
  PeriphClkInitStruct.PLL3.PLL3M = 16;
  PeriphClkInitStruct.PLL3.PLL3N = 125;
  PeriphClkInitStruct.PLL3.PLL3P = 4;
  PeriphClkInitStruct.PLL3.PLL3Q = 4;
  PeriphClkInitStruct.PLL3.PLL3R = 2;
  PeriphClkInitStruct.PLL3.PLL3RGE = RCC_PLL3VCIRANGE_0;
  PeriphClkInitStruct.PLL3.PLL3VCOSEL = RCC_PLL3VCOWIDE;
  PeriphClkInitStruct.PLL3.PLL3FRACN = 0;
  PeriphClkInitStruct.Sai1ClockSelection = RCC_SAI1CLKSOURCE_PLL2;
  PeriphClkInitStruct.UsbClockSelection = RCC_USBCLKSOURCE_PLL3;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct) != HAL_OK)
  {
    Error_Handler();
  }
  /** Enables the Clock Security System 
  */
  HAL_RCC_EnableCSS();
  /** Enable USB Voltage detector 
  */
  HAL_PWREx_EnableUSBVoltageDetector();
}

/**
  * @brief CRC Initialization Function
  * @param None
  * @retval None
  */
static void MX_CRC_Init(void)
{

  /* USER CODE BEGIN CRC_Init 0 */

  /* USER CODE END CRC_Init 0 */

  /* USER CODE BEGIN CRC_Init 1 */

  /* USER CODE END CRC_Init 1 */
  hcrc.Instance = CRC;
  hcrc.Init.DefaultPolynomialUse = DEFAULT_POLYNOMIAL_ENABLE;
  hcrc.Init.DefaultInitValueUse = DEFAULT_INIT_VALUE_ENABLE;
  hcrc.Init.InputDataInversionMode = CRC_INPUTDATA_INVERSION_NONE;
  hcrc.Init.OutputDataInversionMode = CRC_OUTPUTDATA_INVERSION_DISABLE;
  hcrc.InputDataFormat = CRC_INPUTDATA_FORMAT_BYTES;
  if (HAL_CRC_Init(&hcrc) != HAL_OK)
  {
    Error_Handler();
  }
  __HAL_CRC_DR_RESET(&hcrc);
  /* USER CODE BEGIN CRC_Init 2 */

  /* USER CODE END CRC_Init 2 */

}

/**
  * @brief SAI1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_SAI1_Init(void)
{

  /* USER CODE BEGIN SAI1_Init 0 */
	
  /* USER CODE END SAI1_Init 0 */

  /* USER CODE BEGIN SAI1_Init 1 */

  /* USER CODE END SAI1_Init 1 */
  hsai_BlockA1.Instance = SAI1_Block_A;
  hsai_BlockA1.Init.Protocol = SAI_FREE_PROTOCOL;
  hsai_BlockA1.Init.AudioMode = SAI_MODEMASTER_RX;
  hsai_BlockA1.Init.FirstBit = SAI_FIRSTBIT_LSB;
  hsai_BlockA1.Init.ClockStrobing = SAI_CLOCKSTROBING_FALLINGEDGE;
  hsai_BlockA1.Init.Synchro = SAI_ASYNCHRONOUS;
  hsai_BlockA1.Init.OutputDrive = SAI_OUTPUTDRIVE_DISABLE;
  hsai_BlockA1.Init.NoDivider = SAI_MASTERDIVIDER_DISABLE;
  hsai_BlockA1.Init.FIFOThreshold = SAI_FIFOTHRESHOLD_EMPTY;
  hsai_BlockA1.Init.MonoStereoMode = SAI_STEREOMODE;
  hsai_BlockA1.Init.CompandingMode = SAI_NOCOMPANDING;
  hsai_BlockA1.Init.PdmInit.Activation = ENABLE;
  hsai_BlockA1.Init.PdmInit.ClockEnable = SAI_PDM_CLOCK1_ENABLE;
	if(mic_num == 2)
	{
		hsai_BlockA1.Init.PdmInit.MicPairsNbr = 1;
		hsai_BlockA1.FrameInit.FrameLength = 16;
		hsai_BlockA1.Init.DataSize = SAI_DATASIZE_16;
		hsai_BlockA1.SlotInit.SlotNumber = 1;
	}
	else if(mic_num == 4)
	{
		hsai_BlockA1.Init.PdmInit.MicPairsNbr = 2;
		hsai_BlockA1.FrameInit.FrameLength = 32;
		hsai_BlockA1.Init.DataSize = SAI_DATASIZE_16;
		hsai_BlockA1.SlotInit.SlotNumber = 2;
	}
	hsai_BlockA1.FrameInit.ActiveFrameLength = 1;
  hsai_BlockA1.FrameInit.FSDefinition = SAI_FS_STARTFRAME;
  hsai_BlockA1.FrameInit.FSPolarity = SAI_FS_ACTIVE_HIGH;
  hsai_BlockA1.FrameInit.FSOffset = SAI_FS_FIRSTBIT;
  hsai_BlockA1.SlotInit.FirstBitOffset = 0;
  hsai_BlockA1.SlotInit.SlotSize = SAI_SLOTSIZE_DATASIZE; 
  hsai_BlockA1.SlotInit.SlotActive = 0x0000FFFF;
	
  if (HAL_SAI_Init(&hsai_BlockA1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN SAI1_Init 2 */

  /* USER CODE END SAI1_Init 2 */

}

/** 
  * Enable DMA controller clock
  */
static void MX_DMA_Init(void) 
{

  /* DMA controller clock enable */
  __HAL_RCC_DMA1_CLK_ENABLE();

  /* DMA interrupt init */
  /* DMA1_Stream0_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Stream0_IRQn, 2, 0);
  HAL_NVIC_EnableIRQ(DMA1_Stream0_IRQn);

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
	
  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOE_CLK_ENABLE();
  __HAL_RCC_GPIOH_CLK_ENABLE();
	
	/*Configure GPIO pin : PE3 */
	GPIO_InitStruct.Pin = GPIO_PIN_3;//RESET KEY
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);
	
  /*Configure GPIO pin : PC11 */
	GPIO_InitStruct.Pin = GPIO_PIN_11;//MODE KEY
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLDOWN;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_10, GPIO_PIN_SET);

  /*Configure GPIO pins : PC10 */
  GPIO_InitStruct.Pin = GPIO_PIN_10;//MODE LED
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/* MPU Configuration */

void MPU_Config(void)
{
  MPU_Region_InitTypeDef MPU_InitStruct = {0};

  /* Disables the MPU */
  HAL_MPU_Disable();
  /** Initializes and configures the Region and the memory to be protected 
  */
  MPU_InitStruct.Enable = MPU_REGION_ENABLE;
  MPU_InitStruct.Number = MPU_REGION_NUMBER0;
  MPU_InitStruct.BaseAddress = 0x30000000;
  MPU_InitStruct.Size = MPU_REGION_SIZE_128KB;
  MPU_InitStruct.SubRegionDisable = 0x0;
  MPU_InitStruct.TypeExtField = MPU_TEX_LEVEL0;
  MPU_InitStruct.AccessPermission = MPU_REGION_NO_ACCESS;
  MPU_InitStruct.DisableExec = MPU_INSTRUCTION_ACCESS_ENABLE;
  MPU_InitStruct.IsShareable = MPU_ACCESS_SHAREABLE;
  MPU_InitStruct.IsCacheable = MPU_ACCESS_CACHEABLE;
  MPU_InitStruct.IsBufferable = MPU_ACCESS_NOT_BUFFERABLE;

  HAL_MPU_ConfigRegion(&MPU_InitStruct);
  /* Enables the MPU */
  HAL_MPU_Enable(MPU_HFNMI_PRIVDEF);

}

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */

  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
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
     tex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
