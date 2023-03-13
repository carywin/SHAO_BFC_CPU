/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2023 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f4xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */
struct bf_struct {
	int doc_pwr;
	int bf_pwr;
	float volts;
	int amps;
	int fault;
	int last_read;
	uint32_t lastPointTime;
	int lastXDelays[16];
	int lastYDelays[16];
	float lastTemp;
	int lastFlags;
	uint32_t nextPointTime;
	int nextXDelays[16];
	int nextYDelays[16];
	char outstring[255];
} ;

struct bfc_struct {
	float temp;
	uint32_t lastPointTime;
	float lastBFTemps[8];
	uint8_t lastBFFlags[8];
	uint8_t lastBFEnable;
} ;
/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */
void sntp_SetSystemTime(uint32_t sec, uint32_t us);
void parsePointing(const uint8_t *data, int len);
void parseBFPower(const uint8_t *data, int len);
void parseDoCPower(const uint8_t *data, int len);
void doBFTest();
void sendBFPointing();
/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define DoCPwrCtrl3_Pin GPIO_PIN_2
#define DoCPwrCtrl3_GPIO_Port GPIOE
#define DoCPwrCtrl4_Pin GPIO_PIN_3
#define DoCPwrCtrl4_GPIO_Port GPIOE
#define DoCPwrCtrl5_Pin GPIO_PIN_4
#define DoCPwrCtrl5_GPIO_Port GPIOE
#define DoCPwrCtrl6_Pin GPIO_PIN_5
#define DoCPwrCtrl6_GPIO_Port GPIOE
#define DoCPwrCtrl7_Pin GPIO_PIN_6
#define DoCPwrCtrl7_GPIO_Port GPIOE
#define PF0_Pin GPIO_PIN_0
#define PF0_GPIO_Port GPIOF
#define PF1_Pin GPIO_PIN_1
#define PF1_GPIO_Port GPIOF
#define PF2_Pin GPIO_PIN_2
#define PF2_GPIO_Port GPIOF
#define PF3_Pin GPIO_PIN_3
#define PF3_GPIO_Port GPIOF
#define PA3_Pin GPIO_PIN_3
#define PA3_GPIO_Port GPIOA
#define PA4_Pin GPIO_PIN_4
#define PA4_GPIO_Port GPIOA
#define PA5_Pin GPIO_PIN_5
#define PA5_GPIO_Port GPIOA
#define PA6_Pin GPIO_PIN_6
#define PA6_GPIO_Port GPIOA
#define LED1_Pin GPIO_PIN_13
#define LED1_GPIO_Port GPIOF
#define LED2_Pin GPIO_PIN_14
#define LED2_GPIO_Port GPIOF
#define LED3_Pin GPIO_PIN_15
#define LED3_GPIO_Port GPIOF
#define RxData1_Pin GPIO_PIN_0
#define RxData1_GPIO_Port GPIOG
#define RxData2_Pin GPIO_PIN_1
#define RxData2_GPIO_Port GPIOG
#define DoCPwrCtrl8_Pin GPIO_PIN_7
#define DoCPwrCtrl8_GPIO_Port GPIOE
#define BFPwrCtrl1_Pin GPIO_PIN_8
#define BFPwrCtrl1_GPIO_Port GPIOE
#define BFPwrCtrl2_Pin GPIO_PIN_9
#define BFPwrCtrl2_GPIO_Port GPIOE
#define BFPwrCtrl3_Pin GPIO_PIN_10
#define BFPwrCtrl3_GPIO_Port GPIOE
#define BFPwrCtrl4_Pin GPIO_PIN_11
#define BFPwrCtrl4_GPIO_Port GPIOE
#define BFPwrCtrl5_Pin GPIO_PIN_12
#define BFPwrCtrl5_GPIO_Port GPIOE
#define BFPwrCtrl6_Pin GPIO_PIN_13
#define BFPwrCtrl6_GPIO_Port GPIOE
#define BFPwrCtrl7_Pin GPIO_PIN_14
#define BFPwrCtrl7_GPIO_Port GPIOE
#define BFPwrCtrl8_Pin GPIO_PIN_15
#define BFPwrCtrl8_GPIO_Port GPIOE
#define ETH_nRST_Pin GPIO_PIN_10
#define ETH_nRST_GPIO_Port GPIOB
#define TxData1_Pin GPIO_PIN_8
#define TxData1_GPIO_Port GPIOD
#define TxData2_Pin GPIO_PIN_9
#define TxData2_GPIO_Port GPIOD
#define TxData3_Pin GPIO_PIN_10
#define TxData3_GPIO_Port GPIOD
#define TxData4_Pin GPIO_PIN_11
#define TxData4_GPIO_Port GPIOD
#define TxData5_Pin GPIO_PIN_12
#define TxData5_GPIO_Port GPIOD
#define TxData6_Pin GPIO_PIN_13
#define TxData6_GPIO_Port GPIOD
#define TxData7_Pin GPIO_PIN_14
#define TxData7_GPIO_Port GPIOD
#define TxData8_Pin GPIO_PIN_15
#define TxData8_GPIO_Port GPIOD
#define RxData3_Pin GPIO_PIN_2
#define RxData3_GPIO_Port GPIOG
#define RxData4_Pin GPIO_PIN_3
#define RxData4_GPIO_Port GPIOG
#define RxData5_Pin GPIO_PIN_4
#define RxData5_GPIO_Port GPIOG
#define RxData6_Pin GPIO_PIN_5
#define RxData6_GPIO_Port GPIOG
#define RxData7_Pin GPIO_PIN_6
#define RxData7_GPIO_Port GPIOG
#define RxData8_Pin GPIO_PIN_7
#define RxData8_GPIO_Port GPIOG
#define _48VPwrOn_Pin GPIO_PIN_8
#define _48VPwrOn_GPIO_Port GPIOG
#define TxClock1_Pin GPIO_PIN_0
#define TxClock1_GPIO_Port GPIOD
#define TxClock2_Pin GPIO_PIN_1
#define TxClock2_GPIO_Port GPIOD
#define TxClock3_Pin GPIO_PIN_2
#define TxClock3_GPIO_Port GPIOD
#define TxClock4_Pin GPIO_PIN_3
#define TxClock4_GPIO_Port GPIOD
#define TxClock5_Pin GPIO_PIN_4
#define TxClock5_GPIO_Port GPIOD
#define TxClock6_Pin GPIO_PIN_5
#define TxClock6_GPIO_Port GPIOD
#define TxClock7_Pin GPIO_PIN_6
#define TxClock7_GPIO_Port GPIOD
#define TxClock8_Pin GPIO_PIN_7
#define TxClock8_GPIO_Port GPIOD
#define PRAlarm_Pin GPIO_PIN_9
#define PRAlarm_GPIO_Port GPIOG
#define _48VAlarm_Pin GPIO_PIN_10
#define _48VAlarm_GPIO_Port GPIOG
#define DigIn1_Pin GPIO_PIN_11
#define DigIn1_GPIO_Port GPIOG
#define _12VPwrOn_Pin GPIO_PIN_12
#define _12VPwrOn_GPIO_Port GPIOG
#define DigOut1_Pin GPIO_PIN_13
#define DigOut1_GPIO_Port GPIOG
#define DigOut2_Pin GPIO_PIN_14
#define DigOut2_GPIO_Port GPIOG
#define _12VAlarm_Pin GPIO_PIN_15
#define _12VAlarm_GPIO_Port GPIOG
#define OvrTemp_Pin GPIO_PIN_5
#define OvrTemp_GPIO_Port GPIOB
#define DoCPwrCtrl1_Pin GPIO_PIN_0
#define DoCPwrCtrl1_GPIO_Port GPIOE
#define DoCPwrCtrl2_Pin GPIO_PIN_1
#define DoCPwrCtrl2_GPIO_Port GPIOE

/* USER CODE BEGIN Private defines */
// LTC4151 V & I sensor addresses
// NB: These are write addresses, increment by 1 for read addresses
#define BF1_SENSOR_ADDR 0xD0
#define BF2_SENSOR_ADDR 0xD2
#define BF3_SENSOR_ADDR 0xD4
#define BF4_SENSOR_ADDR 0xD6
#define BF5_SENSOR_ADDR 0xD8
#define BF6_SENSOR_ADDR 0xDA
#define BF7_SENSOR_ADDR 0xDC
#define BF8_SENSOR_ADDR 0xDE

#define TEMP_SENSOR_ADDR 0x90

#define EVENT_FLAG 0x01
#define I2C_FLAG 0x02

#define TELEMETRY_PERIOD 60000 // ms

#define I2C_TIMEOUT 1000 // ms

#define SNTP_UNIX_TIMESTAMP_DIFF 2208988800UL

//MQTT command topic IDs
#define CMD_TOPICERROR 0
#define CMD_BFTEST 20
#define CMD_DOCPWR 30
#define CMD_BFPWR 40
#define CMD_POINT 50
#define CMD_STATUS 60
#define CMD_SETTIME 70

#define LONG_TIME_AWAY 3700000000UL;
/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
