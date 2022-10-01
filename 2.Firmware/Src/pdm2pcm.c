/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : pdm2pcm.c
  * @author  MCD Application Team
  * @version V3.0.0
  * @date    28-February-2017
  * @brief   Global header for PDM2PCM conversion code
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
#include "stm32h7xx.h"
#include "stm32h7xx_hal.h"
#include "usbd_audio.h"
#include "pdm2pcm.h"

/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/

/* USER CODE BEGIN PV */
/* Private variables ---------------------------------------------------------*/
extern volatile uint8_t mic_num;

/* USER CODE END PV */


/* External functions --------------------------------------------------------*/

/* USER CODE BEGIN 0 */
/* USER CODE END 0 */

/* USER CODE BEGIN PFP */
/* Private function prototypes -----------------------------------------------*/

/* USER CODE END PFP */

/* Private functions ---------------------------------------------------------*/

/* External functions --------------------------------------------------------*/
/* USER CODE BEGIN ExternalFunctions */

/* USER CODE END ExternalFunctions */

/* USER CODE BEGIN 0 */
/* PDM filters params */
PDM_Filter_Handler_t  PDM_FilterHandler[IN_CHANNELS_4MIC_NUM];
PDM_Filter_Config_t   PDM_FilterConfig[IN_CHANNELS_4MIC_NUM];
/* USER CODE END 0 */

/**
  * @brief  Initializes the PDM library.
  * @param  real_freq: Audio sampling frequency
  * @param  inchannel_num: Number of input audio channels in the PDM buffer
  * @param  outchannel_num: Number of desired output audio channels in the  resulting PCM buffer
  *         Number of audio channels (1: mono; 2: stereo)
  */
void PDMDecoder_Init(uint32_t real_freq, uint8_t inchannels_num, uint8_t outchannels_num)//(outchannel_num 1: mono; 2: stereo)
{
  uint8_t i = 0;

  /* Enable CRC peripheral to unlock the PDM library */
  __HAL_RCC_CRC_CLK_ENABLE();

	for(i = 0; i < inchannels_num; i++)
	{
		/* Init PDM filters */
		PDM_FilterHandler[i].bit_order  = PDM_FILTER_BIT_ORDER_MSB;
		PDM_FilterHandler[i].endianness = PDM_FILTER_ENDIANNESS_LE;//LE and BE is same?
		PDM_FilterHandler[i].high_pass_tap = 2104533974;
		PDM_FilterHandler[i].in_ptr_channels  = inchannels_num;
		PDM_FilterHandler[i].out_ptr_channels = outchannels_num;
		
		PDM_Filter_Init((PDM_Filter_Handler_t *)(&PDM_FilterHandler[i]));

		/* PDM lib config phase */
		if(inchannels_num == 4)
		{
			PDM_FilterConfig[i].output_samples_number =  (uint16_t)(real_freq / 1000 * PDM_IN_PACKET / 2 / AUDIO_IN_4MIC_PACKET);
		}
		else
		{
			PDM_FilterConfig[i].output_samples_number =  (uint16_t)(real_freq / 1000 * PDM_IN_PACKET / 2 / AUDIO_IN_2MIC_PACKET);
		}
		PDM_FilterConfig[i].mic_gain = 5;
		PDM_FilterConfig[i].decimation_factor = PDM_FILTER_DEC_FACTOR_32; //PDM 32bit per PCM 1 Data(16bit)
		PDM_Filter_setConfig((PDM_Filter_Handler_t *)&PDM_FilterHandler[i], &PDM_FilterConfig[i]);
	}
}

/**
  * @brief  Converts audio format from PDM to PCM. 
  * @param  PDMBuf: Pointer to data PDM buffer
  * @param  PCMBuf: Pointer to data PCM buffer
  * @retval AUDIO_OK if correct communication, else wrong communication
  */
uint8_t app_pdm[PDM_IN_PACKET];
HAL_StatusTypeDef PDM_To_PCM_4(uint16_t* PDMBuf, uint16_t* PCMBuf)
{
  uint16_t i; 
  
  /* PDM Demux */
	uint8_t *pdmbuf8 = (uint8_t *)(PDMBuf);
  for(i = 0; i < PDM_IN_PACKET; i += 4)
  {
		app_pdm[i + 0] = pdmbuf8[i + 0];
    app_pdm[i + 1] = pdmbuf8[i + 1];
    app_pdm[i + 2] = pdmbuf8[i + 2];
    app_pdm[i + 3] = pdmbuf8[i + 3];
  }
	
  for(i = 0; i < 4; i++)
  {
    /* PDM to PCM filter */
    PDM_Filter((uint8_t*)&app_pdm[i], (uint16_t*)&(PCMBuf[i]), &PDM_FilterHandler[i]);
  }
	
  /* Return AUDIO_OK when all operations are correctly done */
  return HAL_OK; 
}

HAL_StatusTypeDef PDM_To_PCM_2(uint16_t* PDMBuf, uint16_t* PCMBuf)
{
  uint16_t i; 
  
  /* PDM Demux */
	uint8_t *pdmbuf8 = (uint8_t *)(PDMBuf);
  for(i = 0; i < PDM_IN_PACKET; i += 2)
  {
		app_pdm[i + 0] = pdmbuf8[i + 0];
    app_pdm[i + 1] = pdmbuf8[i + 1];
  }
	
  for(i = 0; i < 2; i++)
  {
    /* PDM to PCM filter */
    PDM_Filter((uint8_t*)&app_pdm[i], (uint16_t*)&(PCMBuf[i]), &PDM_FilterHandler[i]);
  }
	
  /* Return AUDIO_OK when all operations are correctly done */
  return HAL_OK; 
}

/* PDM2PCM init function */
void MX_PDM2PCM_Init(uint8_t mic_num)
{
  /* USER CODE BEGIN 2 */
  /* USER CODE END 2 */

   /** 
  */
	PDMDecoder_Init(USBD_AUDIO_MICROPHONE_FREQ, mic_num, mic_num);

  /* USER CODE BEGIN 3 */
  /* USER CODE END 3 */

}
/* USER CODE BEGIN 1 */

/* USER CODE END 1 */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
