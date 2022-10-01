/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * File Name          : pdm2pcm.h
  * Description        : This file provides code for the configuration
  *                      of the pdm2pcm instances.
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

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __pdm2pcm_H
#define __pdm2pcm_H
#ifdef __cplusplus
  extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "pdm2pcm_glo.h"

/* USER CODE BEGIN 0 */
#define PDM_IN_PACKET  																(32 / 8) * 512 / 2   //1024B

#define CHANNEL_DEMUX_MASK                  								((uint8_t)0x11)
/* USER CODE END 0 */

/* Global variables ---------------------------------------------------------*/

/* USER CODE BEGIN 1 */
/* USER CODE END 1 */

/* PDM2PCM init function */
void MX_PDM2PCM_Init(uint8_t mic_num);

/* USER CODE BEGIN 2 */

void PDMDecoder_Init(uint32_t real_freq, uint8_t in_channels_num, uint8_t out_channels_num);
HAL_StatusTypeDef PDM_To_PCM_4(uint16_t* PDMBuf, uint16_t* PCMBuf);
HAL_StatusTypeDef PDM_To_PCM_2(uint16_t* PDMBuf, uint16_t* PCMBuf);

/* USER CODE END 2 */

/* USER CODE BEGIN 3 */
/* USER CODE END 3 */

#ifdef __cplusplus
}
#endif
#endif /*__pdm2pcm_H */

/**
  * @}
  */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
