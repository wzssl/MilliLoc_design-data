/**
  ******************************************************************************
  * @file    usbd_audio.c
  * @author  MCD Application Team
  * @brief   This file provides the Audio core functions.
  *
  * @verbatim
  *
  *          ===================================================================
  *                                AUDIO Class  Description
  *          ===================================================================
  *           This driver manages the Audio Class 1.0 following the "USB Device Class Definition for
  *           Audio Devices V1.0 Mar 18, 98".
  *           This driver implements the following aspects of the specification:
  *             - Device descriptor management
  *             - Configuration descriptor management
  *             - Standard AC Interface Descriptor management
  *             - 1 Audio Streaming Interface (with single channel, PCM, Stereo mode)
  *             - 1 Audio Streaming Endpoint
  *             - 1 Audio Terminal Input (1 channel)
  *             - Audio Class-Specific AC Interfaces
  *             - Audio Class-Specific AS Interfaces
  *             - AudioControl Requests: only SET_CUR and GET_CUR requests are supported (for Mute)
  *             - Audio Feature Unit (limited to Mute control)
  *             - Audio Synchronization type: Asynchronous
  *             - Single fixed audio sampling rate (configurable in usbd_conf.h file)
  *          The current audio class version supports the following audio features:
  *             - Pulse Coded Modulation (PCM) format
  *             - sampling rate: 48KHz.
  *             - Bit resolution: 16
  *             - Number of channels: 2
  *             - No volume control
  *             - Mute/Unmute capability
  *             - Asynchronous Endpoints
  *
  * @note     In HS mode and when the DMA is used, all variables and data structures
  *           dealing with the DMA during the transaction process should be 32-bit aligned.
  *
  *
  *  @endverbatim
  *
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2015 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under Ultimate Liberty license
  * SLA0044, the "License"; You may not use this file except in compliance with
  * the License. You may obtain a copy of the License at:
  *                      www.st.com/SLA0044
  *
  ******************************************************************************
  */

/* BSPDependencies
- "stm32xxxxx_{eval}{discovery}.c"
- "stm32xxxxx_{eval}{discovery}_io.c"
- "stm32xxxxx_{eval}{discovery}_audio.c"
EndBSPDependencies */

/* Includes ------------------------------------------------------------------*/
#include "usbd_audio.h"
#include "usbd_ctlreq.h"
#include "pdm2pcm.h"


/** @addtogroup STM32_USB_DEVICE_LIBRARY
  * @{
  */


/** @defgroup USBD_AUDIO
  * @brief usbd core module
  * @{
  */

/** @defgroup USBD_AUDIO_Private_TypesDefinitions
  * @{
  */
/**
  * @}
  */


/** @defgroup USBD_AUDIO_Private_Defines
  * @{
  */
extern SAI_HandleTypeDef hsai_BlockA1;
extern DMA_HandleTypeDef hdma_sai1_a;

extern volatile uint8_t mic_num;

extern volatile uint8_t record_en;
extern volatile uint8_t collect_xfrc;
extern volatile uint8_t in_mute;
extern volatile uint16_t in_volumn;
extern volatile uint32_t in_freq;
extern volatile uint8_t in_play_en;

extern volatile uint16_t in_rd_ptr;  
extern volatile uint16_t in_wr_ptr; 
extern volatile uint8_t in_choose;
extern volatile uint16_t in_remain_len;
extern volatile uint8_t collect_xfrc;
extern uint32_t DataInbuffer[];//total buffer 
extern uint32_t PDMDatabufferX[];//PDM buffer
extern uint16_t xin_remain_len;

extern uint32_t DataInbufferP[];

extern volatile uint8_t firstin_xfrc;

extern uint8_t state_in_empty;
extern uint8_t state_in_freq;
extern uint8_t state_in_dmae;
extern uint8_t state_in_flhin;
extern uint8_t state_in_trans;
extern uint16_t state_in_upadj;
extern uint16_t state_in_dnadj;

extern uint16_t in_count;
extern uint16_t sof_count;
extern uint32_t data_count;

#ifdef MYDEBUG		
extern uint16_t xcount;
extern uint32_t freq[];
extern uint32_t time[];
extern uint8_t  type[];
extern uint32_t in_max_real;
extern uint32_t in_min_real;
#endif
/**
  * @}
  */


/** @defgroup USBD_AUDIO_Private_Macros
  * @{
  */
#define AUDIO_SAMPLE_FREQ(frq)      		(uint8_t)(frq), \
																				(uint8_t)((frq >> 8)), \
																				(uint8_t)((frq >> 16))
																				
#define MICPHONE_PACKET_SZE(siz)        (uint8_t)(siz & 0xFF), \
																				(uint8_t)((siz >> 8) & 0xFF)

/**
  * @}
  */


/** @defgroup USBD_AUDIO_Private_FunctionPrototypes
  * @{
  */
static uint8_t USBD_AUDIO_Init(USBD_HandleTypeDef *pdev, uint8_t cfgidx);
static uint8_t USBD_AUDIO_DeInit(USBD_HandleTypeDef *pdev, uint8_t cfgidx);

static uint8_t USBD_AUDIO_Setup(USBD_HandleTypeDef *pdev,
                                USBD_SetupReqTypedef *req);

static uint8_t *USBD_AUDIO_GetCfgDesc(uint16_t *length);
static uint8_t *USBD_AUDIO_GetDeviceQualifierDesc(uint16_t *length);
static uint8_t USBD_AUDIO_DataIn(USBD_HandleTypeDef *pdev, uint8_t epnum);
static uint8_t USBD_AUDIO_DataOut(USBD_HandleTypeDef *pdev, uint8_t epnum);
static uint8_t USBD_AUDIO_EP0_RxReady(USBD_HandleTypeDef *pdev);
static uint8_t USBD_AUDIO_EP0_TxReady(USBD_HandleTypeDef *pdev);
static uint8_t USBD_AUDIO_SOF(USBD_HandleTypeDef *pdev);

static uint8_t USBD_AUDIO_IsoINIncomplete(USBD_HandleTypeDef *pdev, uint8_t epnum);
static uint8_t USBD_AUDIO_IsoOutIncomplete(USBD_HandleTypeDef *pdev, uint8_t epnum);

static void AUDIO_REQ_GetCurrent(USBD_HandleTypeDef *pdev, USBD_SetupReqTypedef *req);
static void AUDIO_REQ_GetMinimum(USBD_HandleTypeDef *pdev, USBD_SetupReqTypedef *req);
static void AUDIO_REQ_GetMaximum(USBD_HandleTypeDef *pdev, USBD_SetupReqTypedef *req);
static void AUDIO_REQ_GetResource(USBD_HandleTypeDef *pdev, USBD_SetupReqTypedef *req);

static void AUDIO_REQ_SetCurrent(USBD_HandleTypeDef *pdev, USBD_SetupReqTypedef *req);

/**
  * @}
  */

/** @defgroup USBD_AUDIO_Private_Variables
  * @{
  */

USBD_ClassTypeDef  USBD_AUDIO =
{
  USBD_AUDIO_Init,
  USBD_AUDIO_DeInit,
  USBD_AUDIO_Setup,
  USBD_AUDIO_EP0_TxReady,
  USBD_AUDIO_EP0_RxReady,
  USBD_AUDIO_DataIn,
  USBD_AUDIO_DataOut,
  USBD_AUDIO_SOF,
  USBD_AUDIO_IsoINIncomplete,
  USBD_AUDIO_IsoOutIncomplete,
  USBD_AUDIO_GetCfgDesc,
  USBD_AUDIO_GetCfgDesc,
  USBD_AUDIO_GetCfgDesc,
  USBD_AUDIO_GetDeviceQualifierDesc,
};

/* USB AUDIO device Configuration Descriptor */
__ALIGN_BEGIN static uint8_t USBD_AUDIO_2MIC_CfgDesc[USB_TOTAL_CONFIG_2MIC_DESC_SIZE] __ALIGN_END =
{
  /* USB Audio Configuration */
  USB_CONFIG_DESC_SIZE,                 /* bLength */
  USB_DESC_TYPE_CONFIGURATION,          /* bDescriptorType */
  LOBYTE(USB_TOTAL_CONFIG_2MIC_DESC_SIZE),   /* wTotalLength */
  HIBYTE(USB_TOTAL_CONFIG_2MIC_DESC_SIZE),      
  0x02,                                 /* bNumInterfaces */                              //共2个接口
  0x01,                                 /* bConfigurationValue */
  0x00,                                 /* iConfiguration */
  0xC0,                                 /* bmAttributes  BUS Powred*/
  0x64,                                 /* bMaxPower = 200 mA*/
  /* 09 byte*/
  
  /* Standard USB Audio Control interface descriptor */                                   //输出接口
  AUDIO_INTERFACE_DESC_SIZE,            /* bLength */
  USB_DESC_TYPE_INTERFACE,              /* bDescriptorType */
  AUDIO_INTERFACE_CONTROL,             	/* bInterfaceNumber */                            //接口序号0                         
  0x00,                                 /* bAlternateSetting */                           //备用编号0
  0x00,                                 /* bNumEndpoints */                               //使用端点数目为0  
  USB_DEVICE_CLASS_AUDIO,               /* bInterfaceClass */                             //音频接口类0x01
  AUDIO_SUBCLASS_AUDIOCONTROL,          /* bInterfaceSubClass */                          //音频控制接口子类0x01
  AUDIO_PROTOCOL_UNDEFINED,             /* bInterfaceProtocol */                          //不使用协议
  0x00,                                 /* iInterface */                                  //接口字符串引索
  /* 09 byte*/
  
  /* Class-specific USB Audio Control Interface Descriptor */      
  AUDIO_INTERFACE_DESC_SIZE,   					/* bLength */                                     //2个流接口:输入和输出
  AUDIO_INTERFACE_DESCRIPTOR_TYPE,      /* bDescriptorType */
  AUDIO_CONTROL_HEADER,                 /* bDescriptorSubtype */                          //描述符子类:控制头
  0x00,                                 /* bcdADC */                                      //版本 1.00
  0x01,
  AUDIO_HEADER_2MIC_SIZE,               /* wTotalLength = 40*/
  0x00,
  0x01,                                 /* bInCollection */                               //流接口数量
  AUDIO_INTERFACE_IN,                 	/* baInterfaceNr */                               //属于本接口的流接口编号
  /* 9 byte*/

	/* USB Microphone Input Terminal Descriptor */
  AUDIO_INPUT_TERMINAL_DESC_SIZE,       /* bLength */
  AUDIO_INTERFACE_DESCRIPTOR_TYPE,      /* bDescriptorType */
  AUDIO_CONTROL_INPUT_TERMINAL,         /* bDescriptorSubtype */                          //控制输入终端
  AUDIO_IN_INTERM_ID,                   /* bTerminalID */                                 //终端ID 1
  0x01,                                 /* wTerminalType */                               //输入终端类型: 0x0201
  0x02,
  0x00,                                 /* bAssocTerminal */                              //与其结合在一起的输出终端:无
  IN_CHANNELS_2MIC_NUM,                 /* bNrChannels */                                 //音频簇的通道数目:1
  IN_CHANNELS_2MIC_CONFIG,              /* wChannelConfig */                              //2声道 0:Left Front 1:Right Front
	0x00,
  0x00,                                 /* iChannelNames */                               //逻辑通道的字符串索引号:无
  0x00,                                 /* iTerminal */                                   //输入终端的字符串索引号:无
  /* 12 byte*/
	
	/* USB Microphone Feature Unit Descriptor */                                         		//控制单元
  AUDIO_FEATURE_UNIT_2MIC_DESC_SIZE,    /* bLength */
  AUDIO_INTERFACE_DESCRIPTOR_TYPE,      /* bDescriptorType */
  AUDIO_CONTROL_FEATURE_UNIT,           /* bDescriptorSubtype */
  AUDIO_IN_FU_ID,              					/* bUnitID */                                     //单元ID 2
  AUDIO_IN_INTERM_ID,                   /* bSourceID */                                   //连接的ID:3
  0x01,                                 /* bControlSize */                                //控制大小1个字节
  AUDIO_CONTROL_MUTE, 									/* bmaControls(Mute) */
  0x00,                 								/* bmaControls(0)(Volume) */
  0x00,                 								/* bmaControls(1)(Volume) */
  0x00,                                 /* iTerminal */                                   //特征单元的字符串索引号:无
  /* 10 byte*/
  
	/* USB Microphone Output Terminal Descriptor */
  AUDIO_OUTPUT_TERMINAL_DESC_SIZE,      /* bLength */
  AUDIO_INTERFACE_DESCRIPTOR_TYPE,      /* bDescriptorType */
  AUDIO_CONTROL_OUTPUT_TERMINAL,        /* bDescriptorSubtype */
  AUDIO_IN_OUTTERM_ID,                  /* bTerminalID */                                 //终端ID 3
  0x01,                                 /* wTerminalType */                        			  //输出终端类型 0x0101 USB Streaming
  0x01,
  0x00,                                 /* bAssocTerminal */                              //与其结合在一起的输入终端:无
  AUDIO_IN_FU_ID,                       /* bSourceID */                                   //连接的ID:2
  0x00,                                 /* iTerminal */                                   //输出终端的字符串索引号:无
  /* 09 byte*/
  
	//-------------------Microphone  interface---------------------//
	
	/* USB Microphone Standard AS Interface Descriptor Interface 1, Alternate Setting 0 */
  AUDIO_INTERFACE_DESC_SIZE,            /* bLength */	                                    //输出接口
  USB_DESC_TYPE_INTERFACE,              /* bDescriptorType */
  AUDIO_INTERFACE_IN,                  	/* bInterfaceNumber */                            //接口号:1
  0x00,                                 /* bAlternateSetting */                           //备用编号0
  0x00,                                 /* bNumEndpoints */                               //端点数量 0  
  USB_DEVICE_CLASS_AUDIO,               /* bInterfaceClass */                             //            
  AUDIO_SUBCLASS_AUDIOSTREAMING,        /* bInterfaceSubClass */                      
  AUDIO_PROTOCOL_UNDEFINED,             /* bInterfaceProtocol */
  0x00,                                 /* iInterface */                                  //接口字符串索引号:无
  /* 09 byte*/
  
  /* USB Microphone Standard AS Interface Descriptor Interface 1, Alternate Setting 1 */
  AUDIO_INTERFACE_DESC_SIZE,            /* bLength */
  USB_DESC_TYPE_INTERFACE,              /* bDescriptorType */
  AUDIO_INTERFACE_IN,                  	/* bInterfaceNumber */                            //接口号:2              
  0x01,                                 /* bAlternateSetting */                           //备用编号1
  0x01,                                 /* bNumEndpoints */                               //端点数量 1 
  USB_DEVICE_CLASS_AUDIO,               /* bInterfaceClass */                             //音频接口类0x01
  AUDIO_SUBCLASS_AUDIOSTREAMING,        /* bInterfaceSubClass */                          //音频控制接口子类0x02
  AUDIO_PROTOCOL_UNDEFINED,             /* bInterfaceProtocol */                          //不使用协议
  0x00,                                 /* iInterface */                                  //接口字符串索引号:无
  /* 09 byte*/
  
  /* USB Microphone Class-specific Audio Streaming Interface Descriptor */
  AUDIO_STREAMING_INTERFACE_DESC_SIZE,  /* bLength */
  AUDIO_INTERFACE_DESCRIPTOR_TYPE,      /* bDescriptorType */
  AUDIO_STREAMING_DESCRIPTOR,           /* bDescriptorSubtype */
  AUDIO_IN_OUTTERM_ID,                  /* bTerminalLink */                               //连接的ID:6
  0x01,                                 /* bDelay */
  0x01,                                 /* wFormatTag AUDIO_FORMAT_PCM  0x0001*/          //PCM
  0x00,
  /* 07 byte*/
  
  /* USB Microphone Class-specific AS Type I Format Interface Descriptor */
  AUDIO_INTERFACE_DESC_SIZE - 1 + 3,    /* bLength */
  AUDIO_INTERFACE_DESCRIPTOR_TYPE,      /* bDescriptorType */
  AUDIO_STREAMING_FORMAT_TYPE,          /* bDescriptorSubtype */
  AUDIO_FORMAT_TYPE_I,                  /* bFormatType */                                 //TYPE I
  IN_CHANNELS_2MIC_NUM,                 /* bNrChannels */                                 //通道
  IN_SUBFRAMESIZE,                      /* bSubFrameSize :  2 Bytes per frame (16bits) */ //字节
  IN_BITRESOLUTION,                    	/* bBitResolution (16-bits per sample) */         //16bit
  0x01,                                 /* bSamFreqType only one frequency supported */   //支持的采样种类
  AUDIO_SAMPLE_FREQ(USBD_AUDIO_MICROPHONE_FREQ),   /* Audio sampling frequency coded on 3 bytes */   //具体采样说明
  /* 11 byte*/
  
  /* USB Microphone Standard AS ISOC Endpoint Descriptor */
  AUDIO_STANDARD_ENDPOINT_DESC_SIZE,    /* bLength */
  USB_DESC_TYPE_ENDPOINT,               /* bDescriptorType */
  AUDIO_IN_EP,                          /* bEndpointAddress 3 IN endpoint*/              //端点3输入
#ifdef ADAPTION_ENABLE
	USBD_EP_TYPE_ISOC | 0x08,        			/* bmAttributes */                                //同步端点 D3:2 01:异步;10:适应;11:同步;
	MICPHONE_PACKET_SZE(AUDIO_IN_2MIC_PACKET + (AUDIO_IN_2MIC_PACKET / 4)), /* wMaxPacketSize in Bytes (Freq(Samples)*2(Stereo)*SubSize(HalfWord)) */ //每个包的长度
#else
	USBD_EP_TYPE_ISOC,        						/* bmAttributes */                                //同步端点 D3:2 01:异步;10:适应;11:同步;
	MICPHONE_PACKET_SZE(AUDIO_IN_PACKET), /* wMaxPacketSize in Bytes (Freq(Samples)*2(Stereo)*SubSize(HalfWord)) */ //每个包的长度
#endif
	0x01,                                 /* bInterval */                                   //1ms
  0x00,                                 /* bRefresh */
  0x00,                                 /* bSynchAddress */
  /* 09 byte*/
  
  /* USB Microphone Class-specific AS ISOC Endpoint Descriptor*/
  AUDIO_STREAMING_ENDPOINT_DESC_SIZE,   /* bLength */
  AUDIO_ENDPOINT_DESCRIPTOR_TYPE,       /* bDescriptorType */
  AUDIO_ENDPOINT_DESCRIPTOR,            /* bDescriptor */           
  0x00,                                 /* bmAttributes */
  0x00,                                 /* bLockDelayUnits */
  0x00,                                 /* wLockDelay */
  0x00
  /* 07 byte*/
};

__ALIGN_BEGIN static uint8_t USBD_AUDIO_4MIC_CfgDesc[USB_TOTAL_CONFIG_4MIC_DESC_SIZE] __ALIGN_END =
{
  /* USB Audio Configuration */
  USB_CONFIG_DESC_SIZE,                 /* bLength */
  USB_DESC_TYPE_CONFIGURATION,          /* bDescriptorType */
  LOBYTE(USB_TOTAL_CONFIG_4MIC_DESC_SIZE),   /* wTotalLength */
  HIBYTE(USB_TOTAL_CONFIG_4MIC_DESC_SIZE),      
  0x02,                                 /* bNumInterfaces */                              //共2个接口
  0x01,                                 /* bConfigurationValue */
  0x00,                                 /* iConfiguration */
  0xC0,                                 /* bmAttributes  BUS Powred*/
  0x64,                                 /* bMaxPower = 200 mA*/
  /* 09 byte*/
  
  /* Standard USB Audio Control interface descriptor */                                   //输出接口
  AUDIO_INTERFACE_DESC_SIZE,            /* bLength */
  USB_DESC_TYPE_INTERFACE,              /* bDescriptorType */
  AUDIO_INTERFACE_CONTROL,             	/* bInterfaceNumber */                            //接口序号0                         
  0x00,                                 /* bAlternateSetting */                           //备用编号0
  0x00,                                 /* bNumEndpoints */                               //使用端点数目为0  
  USB_DEVICE_CLASS_AUDIO,               /* bInterfaceClass */                             //音频接口类0x01
  AUDIO_SUBCLASS_AUDIOCONTROL,          /* bInterfaceSubClass */                          //音频控制接口子类0x01
  AUDIO_PROTOCOL_UNDEFINED,             /* bInterfaceProtocol */                          //不使用协议
  0x00,                                 /* iInterface */                                  //接口字符串引索
  /* 09 byte*/
  
  /* Class-specific USB Audio Control Interface Descriptor */      
  AUDIO_INTERFACE_DESC_SIZE,   					/* bLength */                                     //2个流接口:输入和输出
  AUDIO_INTERFACE_DESCRIPTOR_TYPE,      /* bDescriptorType */
  AUDIO_CONTROL_HEADER,                 /* bDescriptorSubtype */                          //描述符子类:控制头
  0x00,                                 /* bcdADC */                                      //版本 1.00
  0x01,
  AUDIO_HEADER_4MIC_SIZE,               /* wTotalLength = 42*/
  0x00,
  0x01,                                 /* bInCollection */                               //流接口数量
  AUDIO_INTERFACE_IN,                 	/* baInterfaceNr */                               //属于本接口的流接口编号
  /* 9 byte*/

	/* USB Microphone Input Terminal Descriptor */
  AUDIO_INPUT_TERMINAL_DESC_SIZE,       /* bLength */
  AUDIO_INTERFACE_DESCRIPTOR_TYPE,      /* bDescriptorType */
  AUDIO_CONTROL_INPUT_TERMINAL,         /* bDescriptorSubtype */                          //控制输入终端
  AUDIO_IN_INTERM_ID,                   /* bTerminalID */                                 //终端ID 1
  0x05,                                 /* wTerminalType */                               //输入终端类型: 0x0205
  0x02,
  0x00,                                 /* bAssocTerminal */                              //与其结合在一起的输出终端:无
  IN_CHANNELS_4MIC_NUM,                 /* bNrChannels */                                 //音频簇的通道数目:4
  IN_CHANNELS_4MIC_CONFIG,              /* wChannelConfig */                              //4声道 0:Left Front 1:Right Front 2:Center Front 3:Low Freq 4:Left Surround 5:Right Surround 6:Center Left 7:Center Right
	0x00,
  0x00,                                 /* iChannelNames */                               //逻辑通道的字符串索引号:无
  0x00,                                 /* iTerminal */                                   //输入终端的字符串索引号:无
  /* 12 byte*/
	
	/* USB Microphone Feature Unit Descriptor */                                         		//控制单元
  AUDIO_FEATURE_UNIT_4MIC_DESC_SIZE,    /* bLength */
  AUDIO_INTERFACE_DESCRIPTOR_TYPE,      /* bDescriptorType */
  AUDIO_CONTROL_FEATURE_UNIT,           /* bDescriptorSubtype */
  AUDIO_IN_FU_ID,              					/* bUnitID */                                     //单元ID 2
  AUDIO_IN_INTERM_ID,                   /* bSourceID */                                   //连接的ID:3
  0x01,                                 /* bControlSize */                                //控制大小1个字节
  AUDIO_CONTROL_MUTE, 									/* bmaControls(Mute) */
  0x00,                 								/* bmaControls(0)(Volume) */
  0x00,                 								/* bmaControls(1)(Volume) */
  0x00,                 								/* bmaControls(2)(Volume) */
  0x00,                 								/* bmaControls(3)(Volume) */
  0x00,                                 /* iTerminal */                                   //特征单元的字符串索引号:无
  /* 12 byte*/
  
	/* USB Microphone Output Terminal Descriptor */
  AUDIO_OUTPUT_TERMINAL_DESC_SIZE,      /* bLength */
  AUDIO_INTERFACE_DESCRIPTOR_TYPE,      /* bDescriptorType */
  AUDIO_CONTROL_OUTPUT_TERMINAL,        /* bDescriptorSubtype */
  AUDIO_IN_OUTTERM_ID,                  /* bTerminalID */                                 //终端ID 3
  0x01,                                 /* wTerminalType */                        			  //输出终端类型 0x0101 USB Streaming
  0x01,
  0x00,                                 /* bAssocTerminal */                              //与其结合在一起的输入终端:无
  AUDIO_IN_FU_ID,                       /* bSourceID */                                   //连接的ID:2
  0x00,                                 /* iTerminal */                                   //输出终端的字符串索引号:无
  /* 09 byte*/
  
	//-------------------Microphone  interface---------------------//
	
	/* USB Microphone Standard AS Interface Descriptor Interface 1, Alternate Setting 0 */
  AUDIO_INTERFACE_DESC_SIZE,            /* bLength */	                                    //输出接口
  USB_DESC_TYPE_INTERFACE,              /* bDescriptorType */
  AUDIO_INTERFACE_IN,                  	/* bInterfaceNumber */                            //接口号:1
  0x00,                                 /* bAlternateSetting */                           //备用编号0
  0x00,                                 /* bNumEndpoints */                               //端点数量 0  
  USB_DEVICE_CLASS_AUDIO,               /* bInterfaceClass */                             //            
  AUDIO_SUBCLASS_AUDIOSTREAMING,        /* bInterfaceSubClass */                      
  AUDIO_PROTOCOL_UNDEFINED,             /* bInterfaceProtocol */
  0x00,                                 /* iInterface */                                  //接口字符串索引号:无
  /* 09 byte*/
  
  /* USB Microphone Standard AS Interface Descriptor Interface 1, Alternate Setting 1 */
  AUDIO_INTERFACE_DESC_SIZE,            /* bLength */
  USB_DESC_TYPE_INTERFACE,              /* bDescriptorType */
  AUDIO_INTERFACE_IN,                  	/* bInterfaceNumber */                            //接口号:2              
  0x01,                                 /* bAlternateSetting */                           //备用编号1
  0x01,                                 /* bNumEndpoints */                               //端点数量 1 
  USB_DEVICE_CLASS_AUDIO,               /* bInterfaceClass */                             //音频接口类0x01
  AUDIO_SUBCLASS_AUDIOSTREAMING,        /* bInterfaceSubClass */                          //音频控制接口子类0x02
  AUDIO_PROTOCOL_UNDEFINED,             /* bInterfaceProtocol */                          //不使用协议
  0x00,                                 /* iInterface */                                  //接口字符串索引号:无
  /* 09 byte*/
  
  /* USB Microphone Class-specific Audio Streaming Interface Descriptor */
  AUDIO_STREAMING_INTERFACE_DESC_SIZE,  /* bLength */
  AUDIO_INTERFACE_DESCRIPTOR_TYPE,      /* bDescriptorType */
  AUDIO_STREAMING_DESCRIPTOR,           /* bDescriptorSubtype */
  AUDIO_IN_OUTTERM_ID,                  /* bTerminalLink */                               //连接的ID:6
  0x01,                                 /* bDelay */
  0x01,                                 /* wFormatTag AUDIO_FORMAT_PCM  0x0001*/          //PCM
  0x00,
  /* 07 byte*/
  
  /* USB Microphone Class-specific AS Type I Format Interface Descriptor */
  AUDIO_INTERFACE_DESC_SIZE - 1 + 3,    /* bLength */
  AUDIO_INTERFACE_DESCRIPTOR_TYPE,      /* bDescriptorType */
  AUDIO_STREAMING_FORMAT_TYPE,          /* bDescriptorSubtype */
  AUDIO_FORMAT_TYPE_I,                  /* bFormatType */                                 //TYPE I
  IN_CHANNELS_4MIC_NUM,                 /* bNrChannels */                                 //通道
  IN_SUBFRAMESIZE,                      /* bSubFrameSize :  2 Bytes per frame (16bits) */ //字节
  IN_BITRESOLUTION,                    	/* bBitResolution (16-bits per sample) */         //16bit
  0x01,                                 /* bSamFreqType only one frequency supported */   //支持的采样种类
  AUDIO_SAMPLE_FREQ(USBD_AUDIO_MICROPHONE_FREQ),   /* Audio sampling frequency coded on 3 bytes */   //具体采样说明
  /* 11 byte*/
  
  /* USB Microphone Standard AS ISOC Endpoint Descriptor */
  AUDIO_STANDARD_ENDPOINT_DESC_SIZE,    /* bLength */
  USB_DESC_TYPE_ENDPOINT,               /* bDescriptorType */
  AUDIO_IN_EP,                          /* bEndpointAddress 3 IN endpoint*/              //端点3输入
#ifdef ADAPTION_ENABLE
	USBD_EP_TYPE_ISOC | 0x08,        			/* bmAttributes */                                //同步端点 D3:2 01:异步;10:适应;11:同步;
	MICPHONE_PACKET_SZE(AUDIO_IN_4MIC_PACKET + (AUDIO_IN_4MIC_PACKET / 4)), /* wMaxPacketSize in Bytes (Freq(Samples)*2(Stereo)*SubSize(HalfWord)) */ //每个包的长度
#else
	USBD_EP_TYPE_ISOC,        						/* bmAttributes */                                //同步端点 D3:2 01:异步;10:适应;11:同步;
	MICPHONE_PACKET_SZE(AUDIO_IN_PACKET), /* wMaxPacketSize in Bytes (Freq(Samples)*2(Stereo)*SubSize(HalfWord)) */ //每个包的长度
#endif
	0x01,                                 /* bInterval */                                   //1ms
  0x00,                                 /* bRefresh */
  0x00,                                 /* bSynchAddress */
  /* 09 byte*/
  
  /* USB Microphone Class-specific AS ISOC Endpoint Descriptor*/
  AUDIO_STREAMING_ENDPOINT_DESC_SIZE,   /* bLength */
  AUDIO_ENDPOINT_DESCRIPTOR_TYPE,       /* bDescriptorType */
  AUDIO_ENDPOINT_DESCRIPTOR,            /* bDescriptor */           
  0x00,                                 /* bmAttributes */
  0x00,                                 /* bLockDelayUnits */
  0x00,                                 /* wLockDelay */
  0x00
  /* 07 byte*/
};

/* USB Standard Device Descriptor */
__ALIGN_BEGIN static uint8_t USBD_AUDIO_DeviceQualifierDesc[USB_LEN_DEV_QUALIFIER_DESC] __ALIGN_END =
{
  USB_LEN_DEV_QUALIFIER_DESC,
  USB_DESC_TYPE_DEVICE_QUALIFIER,
  0x00,
  0x02,
  0x00,
  0x00,
  0x00,
  USB_MAX_EP0_SIZE,
  USBD_MAX_NUM_INTERFACES,
  0x00,
};


void init_in_parameter(void)
{
	in_play_en = 0;
	
	in_choose = 0;
	in_remain_len = 0;
	collect_xfrc = 0;
	in_rd_ptr = 0;
	in_wr_ptr = (uint16_t)(AUDIO_IN_BUF_WORD / 2);
}
/**
  * @}
  */

/** @defgroup USBD_AUDIO_Private_Functions
  * @{
  */

/**
  * @brief  USBD_AUDIO_Init
  *         Initialize the AUDIO interface
  * @param  pdev: device instance
  * @param  cfgidx: Configuration index
  * @retval status
  */
volatile uint8_t Audio_ClassData[sizeof (USBD_AUDIO_HandleTypeDef)];//llhhxy
static uint8_t  USBD_AUDIO_Init(USBD_HandleTypeDef *pdev, uint8_t cfgidx)
{
  USBD_AUDIO_HandleTypeDef   *haudio;

  /* Open EP IN */
	if(mic_num == 2)
	{
		USBD_LL_OpenEP(pdev,
									 AUDIO_IN_EP,
									 USBD_EP_TYPE_ISOC,
#ifdef ADAPTION_ENABLE
									 (AUDIO_IN_2MIC_PACKET + AUDIO_IN_2MIC_PACKET / 4));
#else
									 AUDIO_IN_2MIC_PACKET);
#endif
	}
	else if(mic_num == 4)
	{
		USBD_LL_OpenEP(pdev,
									 AUDIO_IN_EP,
									 USBD_EP_TYPE_ISOC,
#ifdef ADAPTION_ENABLE
									(AUDIO_IN_4MIC_PACKET + AUDIO_IN_4MIC_PACKET / 4));
#else
									 AUDIO_IN_4MIC_PACKET);
#endif
	}
  pdev->ep_in[AUDIO_IN_EP & 0xFU].is_used = 1U;

  /* Allocate Audio structure */
  pdev->pClassData = (void *)(Audio_ClassData);//USBD_malloc(sizeof (USBD_AUDIO_HandleTypeDef));//llhhxy

  if(pdev->pClassData == NULL)
  {
    return USBD_FAIL;
  }
  else
  {
    haudio = (USBD_AUDIO_HandleTypeDef*) pdev->pClassData;
    haudio->out_alt_setting = 0; 
		haudio->in_alt_setting = 0; 

    /* Initialize the Audio output Hardware layer */
    if (((USBD_AUDIO_ItfTypeDef *)pdev->pUserData)->Init(USBD_AUDIO_MICROPHONE_FREQ, 0, 0) != USBD_OK)//AUDIO_Init_HS
    {
      return USBD_FAIL;
    }
  }
  return USBD_OK;
}

/**
  * @brief  USBD_AUDIO_Init
  *         DeInitialize the AUDIO layer
  * @param  pdev: device instance
  * @param  cfgidx: Configuration index
  * @retval status
  */
static uint8_t  USBD_AUDIO_DeInit(USBD_HandleTypeDef *pdev,
                                  uint8_t cfgidx)
{
  /* Open EP OUT */
  USBD_LL_CloseEP(pdev, AUDIO_OUT_EP);
  pdev->ep_out[AUDIO_OUT_EP & 0xFU].is_used = 0U;

  /* DeInit  physical Interface components */
  if (pdev->pClassData != NULL)
  {
    ((USBD_AUDIO_ItfTypeDef *)pdev->pUserData)->DeInit(0U);//AUDIO_DeInit_HS
    USBD_free(pdev->pClassData);
    pdev->pClassData = NULL;
  }

  return USBD_OK;
}

/**
  * @brief  USBD_AUDIO_Setup
  *         Handle the AUDIO specific requests
  * @param  pdev: instance
  * @param  req: usb requests
  * @retval status
  */
static uint8_t  USBD_AUDIO_Setup (USBD_HandleTypeDef *pdev,
                                USBD_SetupReqTypedef *req)
{
  USBD_AUDIO_HandleTypeDef *haudio;
  uint16_t len;
  uint8_t *pbuf;
  uint16_t status_info = 0U;
  uint8_t ret = USBD_OK;

  haudio = (USBD_AUDIO_HandleTypeDef*) pdev->pClassData;

  switch (req->bmRequest & USB_REQ_TYPE_MASK)
  {
  case USB_REQ_TYPE_CLASS : //类请求 
    switch (req->bRequest)
    {
			case AUDIO_REQ_GET_CUR:
				AUDIO_REQ_GetCurrent(pdev, req);//获取当前音量
				break;
			
			case AUDIO_REQ_GET_MIN:
				AUDIO_REQ_GetMinimum(pdev, req);//获取最小的音量
				break;
			
			case AUDIO_REQ_GET_MAX:
				AUDIO_REQ_GetMaximum(pdev, req);//获取最大的音量
				break;
			
			case AUDIO_REQ_GET_RES:
				AUDIO_REQ_GetResource(pdev, req);//获取分辨率
				break;
				
			case AUDIO_REQ_SET_CUR:
				AUDIO_REQ_SetCurrent(pdev, req);//设置当前状态
				break;
				
			default:
				USBD_CtlError (pdev, req);
				ret = USBD_FAIL; 
    }
    break;

  case USB_REQ_TYPE_STANDARD: //标准请求
    switch (req->bRequest)
    {
    case USB_REQ_GET_STATUS:
      if (pdev->dev_state == USBD_STATE_CONFIGURED)
      {
        USBD_CtlSendData (pdev, (uint8_t *)(void *)&status_info, 2U);
      }
      else
      {
        USBD_CtlError (pdev, req);
        ret = USBD_FAIL;
      }
      break;

    case USB_REQ_GET_DESCRIPTOR:
      if(HIBYTE(req->wValue) == AUDIO_DESCRIPTOR_TYPE)
      {
				if(mic_num == 2)
					pbuf = USBD_AUDIO_2MIC_CfgDesc + 18;
				else if(mic_num == 4)
					pbuf = USBD_AUDIO_4MIC_CfgDesc + 18;
        len = MIN(USB_AUDIO_DESC_SIZE , req->wLength);

        USBD_CtlSendData (pdev, pbuf, len);
      }
      break;

    case USB_REQ_GET_INTERFACE :
      if (pdev->dev_state == USBD_STATE_CONFIGURED)
      {
        if(req->wIndex == AUDIO_INTERFACE_OUT)
				{
					USBD_CtlSendData (pdev, (uint8_t *)&(haudio->out_alt_setting), 1);
				}
				else if(req->wIndex == AUDIO_INTERFACE_IN)
				{
					USBD_CtlSendData (pdev, (uint8_t *)&(haudio->in_alt_setting), 1);
				}
      }
      else
      {
        USBD_CtlError (pdev, req);
        ret = USBD_FAIL;
      }
      break;

    case USB_REQ_SET_INTERFACE :
      if (pdev->dev_state == USBD_STATE_CONFIGURED)
      {
         if ((uint8_t)(req->wValue) <= USBD_MAX_NUM_INTERFACES)
         {
						if(req->wIndex == AUDIO_INTERFACE_IN)
						{
							haudio->in_alt_setting = (uint8_t)(req->wValue);
							if(haudio->in_alt_setting == 1)//启用工作设置
							{
								record_en = 0x01;
								firstin_xfrc = 1;
							}
							else
							{
								record_en = 0x00;
								firstin_xfrc = 0;
							}
							init_in_parameter();
						}
         }
         else
         {
           /* Call the error management function (command will be nacked */
           USBD_CtlError (pdev, req);
           ret = USBD_FAIL;
         }
      }
      else
      {
        USBD_CtlError (pdev, req);
        ret = USBD_FAIL;
      }
      break;

    default:
      USBD_CtlError (pdev, req);
      ret = USBD_FAIL;
      break;
    }
    break;
  default:
    USBD_CtlError (pdev, req);
    ret = USBD_FAIL;
    break;
  }

  return ret;
}


/**
  * @brief  USBD_AUDIO_GetCfgDesc
  *         return configuration descriptor
  * @param  speed : current device speed
  * @param  length : pointer data length
  * @retval pointer to descriptor buffer
  */
static uint8_t  *USBD_AUDIO_GetCfgDesc (uint16_t *length)
{
	if(mic_num == 2)		
	{
		*length = sizeof (USBD_AUDIO_2MIC_CfgDesc);
		return USBD_AUDIO_2MIC_CfgDesc;
	}
	else if(mic_num == 4)	
	{
		*length = sizeof (USBD_AUDIO_4MIC_CfgDesc);
		return USBD_AUDIO_4MIC_CfgDesc;
	}
}

/**
  * @brief  USBD_AUDIO_EP0_RxReady
  *         handle EP0 Rx Ready event
  * @param  pdev: device instance
  * @retval status
  */
static uint8_t  USBD_AUDIO_EP0_RxReady(USBD_HandleTypeDef *pdev)
{
  USBD_AUDIO_HandleTypeDef   *haudio;
  haudio = (USBD_AUDIO_HandleTypeDef*) pdev->pClassData;

  /* In this driver, to simplify code, only SET_CUR request is managed */
  if (haudio->control.cmd == AUDIO_REQ_SET_FREQ)//SET_FREQ命令
  {
		//设置定时器频率
		if (haudio->control.unit == AUDIO_IN_EP)//Microphone Uint
		{
//			haudio->control.data[3] = 0;
//			in_freq = *((uint32_t *)(haudio->control.data));
			in_freq = ((uint32_t)(haudio->control.data[2]) << 16);
			in_freq += ((uint16_t)(haudio->control.data[1]) << 8);
			in_freq += haudio->control.data[0];
		}
	}
	else if (haudio->control.cmd == AUDIO_REQ_SET_VOLU)//SET_VOLU命令
	{
    if (haudio->control.unit == AUDIO_IN_FU_ID)//Microphone Uint
    {
			if(HIBYTE(haudio->control.type) == AUDIO_CS_MUTE)
			{
				in_mute = haudio->control.data[0];
				((USBD_AUDIO_ItfTypeDef *)pdev->pUserData)->MuteCtl(in_mute); //AUDIO_MuteCtl_FS 01:静音 00:非静音   
			}
			else if(HIBYTE(haudio->control.type) == AUDIO_CS_VOLUME)
			{
				in_volumn = ((uint16_t)(haudio->control.data[1]) << 8) + haudio->control.data[0];
				((USBD_AUDIO_ItfTypeDef *)pdev->pUserData)->VolumeCtl(in_volumn);
			}
    }
		haudio->control.cmd = 0;
		haudio->control.len = 0;
  } 

  return USBD_OK;
}
/**
  * @brief  USBD_AUDIO_EP0_TxReady
  *         handle EP0 TRx Ready event
  * @param  pdev: device instance
  * @retval status
  */
static uint8_t  USBD_AUDIO_EP0_TxReady(USBD_HandleTypeDef *pdev)
{
  /* Only OUT control data are processed */
  return USBD_OK;
}

/**
  * @brief  USBD_AUDIO_IsoINIncomplete
  *         handle data ISO IN Incomplete event
  * @param  pdev: device instance
  * @param  epnum: endpoint index
  * @retval status
  */
static uint8_t  USBD_AUDIO_IsoINIncomplete(USBD_HandleTypeDef *pdev, uint8_t epnum)
{
  return USBD_OK;
}

/**
  * @brief  USBD_AUDIO_IsoOutIncomplete
  *         handle data ISO OUT Incomplete event
  * @param  pdev: device instance
  * @param  epnum: endpoint index
  * @retval status
  */
static uint8_t  USBD_AUDIO_IsoOutIncomplete(USBD_HandleTypeDef *pdev, uint8_t epnum)
{
  return USBD_OK;
}

/**
  * @brief  AUDIO_REQ_GetMinimum
  *         Handles the GET_MIN Audio control request.
  * @param  pdev: instance
  * @param  req: setup class request
  * @retval status
  */
static void AUDIO_REQ_GetMinimum(USBD_HandleTypeDef *pdev, USBD_SetupReqTypedef *req)
{  
  USBD_AUDIO_HandleTypeDef   *haudio;
  haudio = (USBD_AUDIO_HandleTypeDef*) pdev->pClassData;
  
  memset(haudio->control.data, 0, req->wLength);
	
	if(HIBYTE(req->wValue) == AUDIO_CS_VOLUME)//Volumn
	{
		if(HIBYTE(req->wIndex) == AUDIO_IN_FU_ID)//Microphone Uint
		{
			haudio->control.data[0] = 0x00;
			haudio->control.data[1] = 0x80;
		}
	}
	
	/* Send the current mute state */
	USBD_CtlSendData (pdev, 
										haudio->control.data,
										req->wLength);
}

/**
  * @brief  AUDIO_REQ_GetMaximum
  *         Handles the GET_MAX Audio control request.
  * @param  pdev: instance
  * @param  req: setup class request
  * @retval status
  */
static void AUDIO_REQ_GetMaximum(USBD_HandleTypeDef *pdev, USBD_SetupReqTypedef *req)
{  
  USBD_AUDIO_HandleTypeDef   *haudio;
  haudio = (USBD_AUDIO_HandleTypeDef*) pdev->pClassData;
  
  memset(haudio->control.data, 0, req->wLength);
	
	if(HIBYTE(req->wValue) == AUDIO_CS_VOLUME)//Volumn
	{
		if(HIBYTE(req->wIndex) == AUDIO_IN_FU_ID)//Microphone Uint
		{
			haudio->control.data[0] = 0xFF;
			haudio->control.data[1] = 0x7F;
		}
	}
	
	/* Send the current mute state */
	USBD_CtlSendData (pdev, 
										haudio->control.data,
										req->wLength);
}

/**
  * @brief  AUDIO_REQ_GetResource
  *         Handles the GET_RES Audio control request.
  * @param  pdev: instance
  * @param  req: setup class request
  * @retval status
  */
static void AUDIO_REQ_GetResource(USBD_HandleTypeDef *pdev, USBD_SetupReqTypedef *req)
{  
  USBD_AUDIO_HandleTypeDef   *haudio;
  haudio = (USBD_AUDIO_HandleTypeDef*) pdev->pClassData;
  
  memset(haudio->control.data, 0, req->wLength);
	
	if(HIBYTE(req->wValue) == AUDIO_CS_VOLUME)//Volumn
	{
		if(HIBYTE(req->wIndex) == AUDIO_IN_FU_ID)//Microphone Uint
		{
			haudio->control.data[0] = 0x01;
			haudio->control.data[1] = 0x00;
		}
	}
	
	/* Send the current mute state */
	USBD_CtlSendData (pdev, 
										haudio->control.data,
										req->wLength);
}

/**
  * @brief  AUDIO_Req_SetCurrent
  *         Handles the SET_CUR Audio control request.
  * @param  pdev: instance
  * @param  req: setup class request
  * @retval status
  */
static void AUDIO_REQ_SetCurrent(USBD_HandleTypeDef *pdev, USBD_SetupReqTypedef *req)
{ 
  USBD_AUDIO_HandleTypeDef   *haudio;
  haudio = (USBD_AUDIO_HandleTypeDef*) pdev->pClassData;
  
  if (req->wLength)
  {
    /* Prepare the reception of the buffer over EP0 */
    USBD_CtlPrepareRx (pdev, haudio->control.data, req->wLength);    
    
    haudio->control.type = req->wValue;  /* Set the request target type */ 
		/********* HIBYTE(req->wValue) 0x01:mute;        0x02:volume        *********/
		/********* LOBYTE(req->wValue) 0x01:left channel;0x02:right channel *********/
    switch (req->bmRequest & USB_REQ_RECIPIENT_MASK)
    {
			case USB_REQ_RECIPIENT_ENDPOINT://2
				haudio->control.cmd = AUDIO_REQ_SET_FREQ;     /* Set the request value */
				haudio->control.unit = LOBYTE(req->wIndex);  /* Set the request target unit */
				break;
					
			case USB_REQ_RECIPIENT_INTERFACE://1
				haudio->control.cmd = AUDIO_REQ_SET_VOLU;     /* Set the request value */
				haudio->control.unit = HIBYTE(req->wIndex);  /* Set the request target unit */
				break;
			
			case USB_REQ_RECIPIENT_DEVICE://0
				break;

			default:
				break;
    }
    haudio->control.len = req->wLength;          /* Set the request data length */
  }
}

/**
  * @brief  AUDIO_Req_GetCurrent
  *         Handles the GET_CUR Audio control request.
  * @param  pdev: instance
  * @param  req: setup class request
  * @retval status
  */
static void AUDIO_REQ_GetCurrent(USBD_HandleTypeDef *pdev, USBD_SetupReqTypedef *req)
{  
  USBD_AUDIO_HandleTypeDef   *haudio;
  haudio = (USBD_AUDIO_HandleTypeDef*) pdev->pClassData;
  
  memset(haudio->control.data, 0, req->wLength);
	
	switch (req->bmRequest & USB_REQ_RECIPIENT_MASK)
	{
		case USB_REQ_RECIPIENT_ENDPOINT://2
			if(HIBYTE(req->wIndex) == AUDIO_IN_EP)//Microphone EP
			{
				uint8_t freq[3] = {AUDIO_SAMPLE_FREQ(USBD_AUDIO_MICROPHONE_FREQ)};
				haudio->control.data[0] = freq[0];
				haudio->control.data[1] = freq[1];
				haudio->control.data[2] = freq[2];
			}
			break;
				
		case USB_REQ_RECIPIENT_INTERFACE://1
			if(HIBYTE(req->wValue) == AUDIO_CS_MUTE)//Mute
			{
				if(HIBYTE(req->wIndex) == AUDIO_IN_FU_ID)//Microphone Uint
				{
					haudio->control.data[0] = in_mute;
				}
			}
			else if(HIBYTE(req->wValue) == AUDIO_CS_VOLUME)//Volumn
			{
				if(HIBYTE(req->wIndex) == AUDIO_IN_FU_ID)//Microphone Uint
				{
					haudio->control.data[0] = (uint8_t)(in_volumn);
					haudio->control.data[1] = (uint8_t)(in_volumn >> 8);
				}
			}
			break;
			
		case USB_REQ_RECIPIENT_DEVICE://0
			break;

		default:
			break;
	}
		
	/* Send the current mute state */
	USBD_CtlSendData (pdev, 
										haudio->control.data,
										req->wLength);
}

/**
* @brief  DeviceQualifierDescriptor
*         return Device Qualifier descriptor
* @param  length : pointer data length
* @retval pointer to descriptor buffer
*/
static uint8_t  *USBD_AUDIO_GetDeviceQualifierDesc(uint16_t *length)
{
  *length = sizeof(USBD_AUDIO_DeviceQualifierDesc);

  return USBD_AUDIO_DeviceQualifierDesc;
}

/**
* @brief  USBD_AUDIO_RegisterInterface
* @param  fops: Audio interface callback
* @retval status
*/
uint8_t  USBD_AUDIO_RegisterInterface(USBD_HandleTypeDef *pdev,
                                      USBD_AUDIO_ItfTypeDef *fops)
{
  if (fops != NULL)
  {
    pdev->pUserData = fops;
  }

  return USBD_OK;
}

/**
  * @brief  USBD_AUDIO_SOF
  *         handle SOF event
  * @param  pdev: device instance
  * @retval status
  */
static uint8_t  USBD_AUDIO_SOF(USBD_HandleTypeDef *pdev)
{
  sof_count++;

	if(firstin_xfrc == 1)
	{
		if(USBD_OK != USBD_LL_FlushEP(pdev, AUDIO_IN_EP))
		{
			state_in_flhin++;
			HAL_GPIO_WritePin(GPIOD, GPIO_PIN_2, GPIO_PIN_RESET);
		}
		uint16_t sendlen;
		if(mic_num == 2)
			sendlen = (uint16_t)(AUDIO_IN_2MIC_PACKET);
		else if(mic_num == 4)
			sendlen = (uint16_t)(AUDIO_IN_4MIC_PACKET);
		
		if(USBD_OK != USBD_LL_Transmit(pdev, AUDIO_IN_EP, (uint8_t *)DataInbufferP, sendlen))
		{
			state_in_trans++;
			HAL_GPIO_WritePin(GPIOD, GPIO_PIN_2, GPIO_PIN_RESET);
		}
		
		firstin_xfrc = 0;
	}

  return USBD_OK;
}

/**
  * @brief  USBD_AUDIO_DataIn
  *         handle data IN Stage
  * @param  pdev: device instance
  * @param  epnum: endpoint index
  * @retval status
  */
uint16_t cur_slen;
static uint8_t  USBD_AUDIO_DataIn (USBD_HandleTypeDef *pdev, uint8_t epnum)
{
  if (epnum == (AUDIO_IN_EP & 0x7F))//0x03
	{
		if(record_en == 0)
			return USBD_OK;
		
		uint16_t sendlen;
		
		if(mic_num == 2)
			sendlen = (uint16_t)(AUDIO_IN_2MIC_PACKET / 4);
		else if(mic_num == 4)
			sendlen = (uint16_t)(AUDIO_IN_4MIC_PACKET / 4);
		
		in_count++;
		//Calc Length
		in_remain_len = (uint16_t)(in_wr_ptr - in_rd_ptr);
		HAL_NVIC_DisableIRQ(DMA1_Stream0_IRQn);
		if(in_play_en == 1)
		{
			uint16_t remain_len = (uint16_t)(__HAL_DMA_GET_COUNTER(&hdma_sai1_a));//not rx length
			if(remain_len < (uint16_t)(PDM_IN_PACKET / 2))
			{
				in_remain_len += (uint16_t)((PDM_IN_PACKET / 2 - remain_len) / 2);
			}
			else if(remain_len > (uint16_t)(PDM_IN_PACKET / 2))
			{
				in_remain_len += (uint16_t)((PDM_IN_PACKET - remain_len) / 2);
			}
		}
		HAL_NVIC_EnableIRQ(DMA1_Stream0_IRQn);
		
		//Save Length
		xin_remain_len = in_remain_len;
		
		//Check Record
		if(in_play_en == 0)
		{
			in_rd_ptr = 0;
			in_wr_ptr = (uint16_t)(AUDIO_IN_BUF_WORD / 2);
			if(HAL_OK != HAL_SAI_Receive_DMA(&hsai_BlockA1, (uint8_t *)PDMDatabufferX, (uint16_t)(PDM_IN_PACKET)))
			{
#ifdef MYDEBUG	
				time[xcount] = HAL_GetTick();
				type[xcount] = 0x03;
				xcount++;
				if(xcount == 64)
					xcount = 63;
#endif
				HAL_GPIO_WritePin(GPIOB, GPIO_PIN_3, GPIO_PIN_RESET);
				state_in_dmae++;
			}
			in_play_en = 1;
		}
	
		//Check Empty
		if((uint16_t)(in_wr_ptr - in_rd_ptr) <= (uint16_t)(AUDIO_IN_2MIC_PACKET / 4))//EMPTY
		{
			init_in_parameter();
#ifdef MYDEBUG	
			time[xcount] = HAL_GetTick();
			type[xcount] = 0x12;
			xcount++;
			if(xcount == 64)
				xcount = 63;
#endif
			HAL_GPIO_WritePin(GPIOD, GPIO_PIN_1, GPIO_PIN_RESET);
			state_in_empty++;
		}
			
		//Manage Data Length 
		if(in_play_en == 1)
		{
#ifdef ADAPTION_ENABLE
			if(xin_remain_len >= (uint16_t)(RECORD_MAX_COUNT))
			{
				init_in_parameter();
#ifdef MYDEBUG	
				freq[xcount] = xin_remain_len;
				time[xcount] = HAL_GetTick();
				type[xcount] = 0x14;
				xcount++;
				if(xcount == 64)
					xcount = 63;
#endif	
				HAL_GPIO_WritePin(GPIOD, GPIO_PIN_3, GPIO_PIN_RESET);
				state_in_freq++;
			}
			
			if(xin_remain_len <= (uint16_t)(RECORD_MIN_COUNT))
			{
				init_in_parameter();
#ifdef MYDEBUG	
				freq[xcount] = xin_remain_len;
				time[xcount] = HAL_GetTick();
				type[xcount] = 0x14;
				xcount++;
				if(xcount == 64)
					xcount = 63;
#endif	
				HAL_GPIO_WritePin(GPIOD, GPIO_PIN_3, GPIO_PIN_RESET);
				state_in_freq++;
			}
			
			uint16_t diff;
			if(xin_remain_len > (uint16_t)(AUDIO_IN_BUF_WORD / 2))
			{
				diff = (xin_remain_len - (uint16_t)(AUDIO_IN_BUF_WORD / 2)) / 4;
				if(diff > 24)
					diff = 24;
				sendlen +=  diff;
			}
			else
			{
				diff = ((uint16_t)(AUDIO_IN_BUF_WORD / 2) - xin_remain_len) / 4;
				if(diff > 24)
					diff = 24;
				sendlen -= diff;
			}
#else
			if(xin_remain_len > (uint16_t)(RECORD_UP_COUNT))
			{
				if(xin_remain_len >= (uint16_t)(RECORD_MAX_COUNT))
				{
					init_in_parameter();
#ifdef MYDEBUG	
					freq[xcount] = xin_remain_len;
					time[xcount] = HAL_GetTick();
					type[xcount] = 0x14;
					xcount++;
					if(xcount == 64)
						xcount = 63;
#endif	
					HAL_GPIO_WritePin(GPIOD, GPIO_PIN_3, GPIO_PIN_RESET);
					state_in_freq++;
				}
				else
				{
					in_rd_ptr++;
					state_in_upadj++;
				}
			}
			
			if(xin_remain_len < (uint16_t)(RECORD_DN_COUNT))
			{
				if(xin_remain_len <= (uint16_t)(RECORD_MIN_COUNT))
				{
					init_in_parameter();
#ifdef MYDEBUG	
					freq[xcount] = xin_remain_len;
					time[xcount] = HAL_GetTick();
					type[xcount] = 0x14;
					xcount++;
					if(xcount == 64)
						xcount = 63;
#endif	
					HAL_GPIO_WritePin(GPIOD, GPIO_PIN_3, GPIO_PIN_RESET);
					state_in_freq++;
				}
				else
				{
					in_rd_ptr--;
					state_in_dnadj++;
				}
			}
#endif
		
#ifdef MYDEBUG			
			//Save Infromation
			if(xin_remain_len < in_min_real)
				in_min_real = xin_remain_len;
			if(xin_remain_len > in_max_real)
				in_max_real = xin_remain_len;
#endif
		}	
		
		//Copy Data
		cur_slen = sendlen;
		for(uint16_t i = 0; i < sendlen; i++)
		{
			DataInbufferP[i] = DataInbuffer[in_rd_ptr & (uint16_t)(AUDIO_IN_BUF_WORD - 1)];
			in_rd_ptr++;
		}
		
		//Transfer Record Packet
		if(USBD_OK != USBD_LL_FlushEP(pdev, AUDIO_IN_EP))
		{
			state_in_flhin++;
			HAL_GPIO_WritePin(GPIOD, GPIO_PIN_2, GPIO_PIN_RESET);
		}
		if(USBD_OK != USBD_LL_Transmit(pdev, AUDIO_IN_EP, (uint8_t *)DataInbufferP, (uint16_t)(sendlen * 4)))
		{
			state_in_trans++;
			HAL_GPIO_WritePin(GPIOD, GPIO_PIN_2, GPIO_PIN_RESET);
		}
	}
  return USBD_OK;
}

/**
  * @brief  USBD_AUDIO_DataOut
  *         handle data OUT Stage
  * @param  pdev: device instance
  * @param  epnum: endpoint index
  * @retval status
  */
static uint8_t  USBD_AUDIO_DataOut (USBD_HandleTypeDef *pdev, uint8_t epnum)
{
  return USBD_OK;
}




/**
  * @}
  */


/**
  * @}
  */


/**
  * @}
  */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
