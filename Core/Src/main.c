/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
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
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "cmsis_os.h"
#include "crc.h"
#include "eth.h"
#include "i2c.h"
#include "rng.h"
#include "rtc.h"
#include "tim.h"
#include "usart.h"
#include "usb_device.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "FreeRTOS.h"
#include "cJSON.h"
//#include <stdio.h>
#include "bfc_config.h"
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

struct bf_struct beamformer[8];
struct bfc_struct bfController;
unsigned long unixTime = 0;
unsigned long nextPointTime = LONG_TIME_AWAY;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
void MX_FREERTOS_Init(void);
/* USER CODE BEGIN PFP */
void intTo6bits(int value, char *buffer);
void intToXbits(int value, char *buffer, int bits);
void gen_bitstring(int xdelays[], int ydelays[], char *outlist);
void send_bitstring_en(uint8_t bfEnable, float *bf_temp, uint8_t *bf_flags);
void sntp_SetSystemTime(uint32_t sec, uint32_t us);
void parsePointing(const uint8_t *data, int len);
void parseBFPower(const uint8_t *data, int len);
void parseDoCPower(const uint8_t *data, int len);
void doBFTest();
void sendBFPointing();

extern void sendPointingResult();
extern void sendLog(char *logLevel, char *logMsg);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

void ActivateEthPhy(void) // Release LAN8742 reset pin
{
	HAL_GPIO_WritePin(ETH_nRST_GPIO_Port, ETH_nRST_Pin, GPIO_PIN_SET);
}

void DeactivateEthPhy(void) // Hold LAN8742 in reset
{
	HAL_GPIO_WritePin(ETH_nRST_GPIO_Port, ETH_nRST_Pin, GPIO_PIN_RESET);
}

void delay_us(unsigned int us)
{
	__HAL_TIM_SET_COUNTER(&htim6,0);  // set the counter value a 0
	while (__HAL_TIM_GET_COUNTER(&htim6) < us);  // wait for the counter to reach the us input in the parameter
}

#ifdef __GNUC__
#define PUTCHAR_PROTOTYPE int __io_putchar(int ch)
#else
#define PUTCHAR_PROTOTYPE int fputc(int ch, FILE *f)
#endif
PUTCHAR_PROTOTYPE
{
	// Uncomment one line to select where printf() commands send their characters
	// HAL_UART for UART1 pins, or ITM_SendChar for the SWV debugger
	//HAL_UART_Transmit(&huart1, (uint8_t *)&ch, 1, HAL_MAX_DELAY);
	ITM_SendChar(ch);
	return ch;
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
  ActivateEthPhy();
  GPIOD->BSRR = 0xFFFF0000; // Set Port D all bits/clocks low
  HAL_Delay(200);
  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_I2C1_Init();
  MX_I2C3_Init();
  MX_USART1_UART_Init();
  MX_RTC_Init();
  MX_CRC_Init();
  MX_RNG_Init();
  MX_TIM6_Init();
  MX_TIM2_Init();
  MX_ETH_Init();
  /* USER CODE BEGIN 2 */
  HAL_TIM_Base_Start(&htim6); // Start the 1 us timer
  HAL_TIM_Base_Start_IT(&htim2); // Start the 1 s overflow unixTime timer, enable interrupt



  /* USER CODE END 2 */

  /* Init scheduler */
  osKernelInitialize();  /* Call init function for freertos objects (in freertos.c) */
  MX_FREERTOS_Init();

  /* Start scheduler */
  osKernelStart();

  /* We should never get here as control is now taken by the scheduler */
  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
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
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE|RCC_OSCILLATORTYPE_LSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.LSEState = RCC_LSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 4;
  RCC_OscInitStruct.PLL.PLLN = 168;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 7;
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
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */
void gen_bitstring(int xdelays[], int ydelays[], char *outstring)
/* Given two arrays of 16x 16-bit integers representing the X and Y delays, return a string containing 253
 * characters each '1' or '0' representing the bitstream to send to the beamformer.
 */
{
	int i=0, x=0, y=0, chksum=0;
	char buffer[sizeof(int)*8+1] = "";
	char xdelays_b[16*6+1] = "", ydelays_b[16*6+1] = ""; // Packed 16 x 6-bit binary delays (96 bits as string)
	int chk_words[12];

	strcpy(outstring, "00000000111100000000000000000000"); // Preamble 32 bits
	i = 32; // Start adding bits after preamble at pos 32

	// Convert 16x xdelay integers into 6-bit binary values packed in a 96 bit string
	for (x=0; x<=15; x++) {
		intToXbits(xdelays[x], buffer, 6);
		strcat(xdelays_b, buffer);
	}
	// Convert 16x ydelay integers into 6-bit binary values packed in a 96 bit string
	for (y=0; y<=15; y++) {
		intToXbits(ydelays[y], buffer, 6);
		strcat(ydelays_b, buffer);
	}

	// Extract first 6x 16-bit integers from x delay packed string and add to output
	for (x=0; x<=5; x++) {
		for (y=0; y<=16; y++) {
			if (y != 16) {
				outstring[i] = xdelays_b[x*16+y];
				buffer[y] = xdelays_b[x*16+y];
			}
			else {
				outstring[i] = '1'; // Add 1's for spacing between words on output
			}
			i++;
		}
		buffer[16] = '\0';
		chk_words[x] = (int) strtol(buffer, NULL ,2);
	}
	// Extract the second 6x 16-bit integers from y delay packed string and add to output
	for (x=6; x<=11; x++) {
		for (y=0; y<=16; y++) {
			if (y != 16) {
				outstring[i] = ydelays_b[(x-6)*16+y];
				buffer[y] = ydelays_b[(x-6)*16+y];
			}
			else {
				outstring[i] = '1'; // Add 1's for spacing between words on output
			}
			i++;
		}
		buffer[16] = '\0';
		chk_words[x] = (int) strtol(buffer, NULL ,2);
	}

	// Calculate checksum: the 12x 16-bit packed delay words XORed together
	for (x=0; x<=11; x++) {
		chksum = chksum ^ chk_words[x]; // Bitwise XOR
	}

	// Add 16-bit checksum to output string
	intToXbits(chksum, buffer, 16);
	for (x=0; x<=15; x++) {
		if(buffer[x] == '0' || buffer[x] == '1') outstring[i] = buffer[x];
		else outstring[i] = '0';
		i++;
	}

	// Add one more 1 for good measure
	outstring[i] = '1';
	i++;

	// Add termination null
	outstring[i] = '\0';

	printf("Outstring %d bits: %s\n", strlen(outstring), outstring);
}

void intToXbits(int value, char *buffer, int bits) {
	int i = 0;
	for (int mask = 0x01 << (bits - 1); mask > 0; mask >>= 1) {
		buffer[i++] = (value & mask) ? '1':'0';
	}
	buffer[i++] = '\0';
}

void send_bitstring_en(uint8_t bfEnable, float *bf_temp, uint8_t *bf_flags)
/*
 * outstring is a 253 char array of '0' or '1' representing the bitstream to send. This is clocked out of all
 * enabled pins simultaneously followed by 24 more clocks to read back the temperature and flags from the beamformers.
 * Each beamformer has an outstring in its struct to send.
 * Sending to each BF is enabled by setting the corresponding bit in bfEnable.
 */
// bit time 20 us
{
	const int bittime = 20; // us
	const uint32_t clockHigh = bfEnable;
	const uint32_t clockLow = bfEnable << 16;
	int i,j, indata[30], rawtemps[8];
	const uint32_t bitmask[32] = { 0x1, 0x2, 0x4, 0x8, \
							  0x10, 0x20, 0x40, 0x80,\
							  0x100, 0x200, 0x400, 0x800,\
							  0x1000, 0x2000, 0x4000, 0x8000,\
							  0x10000, 0x20000, 0x40000, 0x80000,\
							  0x100000, 0x200000, 0x400000, 0x800000,\
							  0x1000000, 0x2000000, 0x4000000, 0x8000000,\
							  0x10000000, 0x20000000, 0x40000000, 0x80000000};

	taskENTER_CRITICAL(); // Disable interrupts while we send/receive data
	// Clock out 253 bits on all data/clock lines
	for (i=0; i<253; i++) {
		int portD = 0;
		for (j=0; j<=7; j++) {
			if (beamformer[j].outstring[i] == '1' && (bfEnable && bitmask[j])) {
				portD |= bitmask[j+8];
			}
			else {
				portD |= bitmask[j+24];
			}
		}
		GPIOD->BSRR = portD;
		delay_us(bittime / 4);
		GPIOD->BSRR = clockHigh;
		delay_us(bittime / 2);
		GPIOD->BSRR = clockLow;
		delay_us(bittime / 4);
	}

	// Clock in 24 bits of temperature and flags data
	GPIOD->BSRR = 0xFFFF0000; // Set Port D all bits/clocks low
	for (i=0; i<24; i++) {
		delay_us(bittime / 4);
		GPIOD->BSRR = clockHigh;
		delay_us(bittime / 4);
		indata[i] = GPIOG->IDR & 0xFF; // Read data from input pins
		delay_us(bittime / 4);
		GPIOD->BSRR = clockLow;
		delay_us(bittime / 4);
	}

	GPIOD->BSRR = 0xFFFF0000; // Set Port D all bits/clocks low
	taskEXIT_CRITICAL(); // Enable interrupts again, whole process should take ~5 ms

	osThreadYield();

	// Calculate BF temps
	for (i=0; i<=15; i++) { // First 16 bits are temperature (signed 12-bit value) MSB first
		for (j=0; j<=7; j++) {// Do for all enabled beamformers
			int bit = indata[i] & bitmask[j];
			if (bit) rawtemps[j] |= bitmask[15-i];
			else rawtemps[j] &= ~bitmask[15-i];
		}
	}
	for (i=0; i<=7; i++) {
		if (bfEnable & bitmask[i]) {
			float temp = 0.0625 * (rawtemps[i] & 0xFFF);
			if (rawtemps[i] & 0x1000) temp -= 256.0;
			bf_temp[i] = temp;
			beamformer[i].lastTemp = temp;
		} else {
			bf_temp[i] = -256.0;
		}
	}
	// Next 8 bits are flags (7:4 - checksum ok(128)/bad(224 or 0), 3:0 - invalid, set to 0)
	for (i=16; i<=23; i++) { // Next 8 bits of indata
		for (j=0; j<=7; j++) { // Do for all enabled beamformers
			if (bfEnable & bitmask[j]) {
				int bit = indata[i] & bitmask[j];
				if (bit && i < 20) bf_flags[j] |= bitmask[23-i];
				else bf_flags[j] &= ~bitmask[23-i];
			} else {
				bf_flags[j] = 0;
			}
		}
	}
	for (i=0; i<=7; i++) {
		if (bfEnable & bitmask[i]) beamformer[i].lastFlags = bf_flags[i];
	}
} // send_bitstring_en

void parsePointing(const uint8_t *data, int len) {
	int xdelays[16] = {0}, ydelays[16] = {0};

	if (len > 0) {
		cJSON *command = cJSON_ParseWithLength((const char*) data, len);
		if(command != NULL) {
			cJSON *sendTimeJSON = cJSON_GetObjectItem(command, "time");
			cJSON *bfNumJSON = cJSON_GetObjectItem(command, "bf");
			if (cJSON_IsNumber(bfNumJSON) && cJSON_IsNumber(sendTimeJSON)) {
				if (bfNumJSON->valueint >= 1 && bfNumJSON->valueint <= 8 && sendTimeJSON->valuedouble > unixTime + 2) {
					beamformer[bfNumJSON->valueint - 1].nextPointTime = sendTimeJSON->valueint;
					if (sendTimeJSON->valuedouble < nextPointTime) nextPointTime = sendTimeJSON->valuedouble;
					cJSON *xdelayArray = cJSON_GetObjectItem(command, "xdelays");
					if(cJSON_IsArray(xdelayArray)) {
						cJSON *delayValue = NULL;
						int i = 0;
						cJSON_ArrayForEach(delayValue, xdelayArray) {
							xdelays[i] = delayValue->valueint;
							i++;
						}
					}
					cJSON *ydelayArray = cJSON_GetObjectItem(command, "ydelays");
					if(cJSON_IsArray(ydelayArray)) {
						cJSON *delayValue = NULL;
						int i = 0;
						cJSON_ArrayForEach(delayValue, ydelayArray) {
							ydelays[i] = delayValue->valueint;
							i++;
						}
					}
					gen_bitstring(xdelays, ydelays, beamformer[bfNumJSON->valueint - 1].outstring);
				} else {
					//sendLog("Error", "MQTT Pointing Command Error: \'bf\' and/or \'time\' not a valid number");
					printf("MQTT Pointing Command Error: \'bf\' and/or \'time\' not a valid number");
				}
			}
		} else {
			const char *error_ptr = cJSON_GetErrorPtr();
			if (error_ptr != NULL) {
				printf("JSON Parse error before: %s\n", error_ptr);
			}
		}
		cJSON_Delete(command);
	}
}

void parseBFPower(const uint8_t *data, int len){

	if (len > 0) {
		cJSON *command = cJSON_ParseWithLength((const char*) data, len);
		if(command != NULL) {
			cJSON *bf_power_array = cJSON_GetObjectItem(command, "bf_power");
			if(cJSON_IsArray(bf_power_array) && cJSON_GetArraySize(bf_power_array) == 8) {
				cJSON *bf_pwr = NULL;
				int i = 0x0100, j = 0;
				int portE = 0;
				cJSON_ArrayForEach(bf_pwr, bf_power_array) {
					if(cJSON_IsTrue(bf_pwr) && beamformer[j].doc_pwr) {
						portE |= i;
						beamformer[j].bf_pwr = 1;
					} else {
						portE |= i << 16;
						beamformer[j].bf_pwr = 0;
					}
					i <<= 1;
					j++;
				}
				GPIOE->BSRR = portE; // Set bit in lowest 16 bits to set pin, set in highest 16 bits to clear pin
			}
		} else {
			const char *error_ptr = cJSON_GetErrorPtr();
			if (error_ptr != NULL) {
				printf("BFPower JSON Parse error before: %s\n", error_ptr);
			}
		}
		cJSON_Delete(command);
	}
}

void parseDoCPower(const uint8_t *data, int len){

	if (len > 0) {
		cJSON *command = cJSON_ParseWithLength((const char*) data, len);
		if(command != NULL) {
			cJSON *doc_power_array = cJSON_GetObjectItem(command, "doc_power");
			if(cJSON_IsArray(doc_power_array) && cJSON_GetArraySize(doc_power_array) == 8) {
				cJSON *doc_pwr = NULL;
				int i = 0x01, j = 0;
				int portE = 0;
				cJSON_ArrayForEach(doc_pwr, doc_power_array) {
					if(cJSON_IsTrue(doc_pwr)) {
						portE |= i;
						beamformer[j].doc_pwr = 1;
					} else {
						portE |= i << 16;
						beamformer[j].doc_pwr = 0;
						if(beamformer[j].bf_pwr) {
							beamformer[j].bf_pwr = 0;
							portE |= i << 24;
						}
					}
					i <<= 1;
					j++;
				}
				GPIOE->BSRR = portE; // Set bit in lowest 16 bits to set pin, set in highest 16 bits to clear pin
			}
		} else {
			const char *error_ptr = cJSON_GetErrorPtr();
			if (error_ptr != NULL) {
				printf("DoCPower JSON Parse error before: %s\n", error_ptr);
			}
		}
		cJSON_Delete(command);
	}
}

void doBFTest() {
	int testxdelays[16] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
	int testydelays[16] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
	char output[255];
	char saved_bitstrings[8][255];
	int i, j = 0;

	if (nextPointTime - unixTime > 10) {
		gen_bitstring(testxdelays, testydelays, output);
		// Save the current beamformer outstrings and set them to 0 delays
		for (i=0; i<=7; i++) {
			for (j=0; j<=255; j++) {
				saved_bitstrings[i][j] = beamformer[i].outstring[j];
				beamformer[i].outstring[j] = output[j];
			}
		}
		send_bitstring_en(0xFF, bfController.lastBFTemps, bfController.lastBFFlags);
		bfController.lastPointTime = unixTime;
		// Restore the previous beamformer outstrings
		for (i=0; i<=7; i++) {
			for (j=0; j<=255; j++) {
				beamformer[i].outstring[j] = saved_bitstrings[i][j];
			}
		}
		for (i=0; i<=7; i++) {
			for (j=0; j<=15; j++) {
				beamformer[i].lastXDelays[j] = 0;
				beamformer[i].lastYDelays[j] = 0;
			}
			beamformer[i].lastPointTime = unixTime;
		}
		sendPointingResult();
	} else {
		printf("Can't do BF Test, next pointing is too soon\n");
		//sendLog("Error", "Can't do BF Test, next pointing is too soon");
	}
}

void sendBFPointing() {

	uint8_t bfEnable = 0;
	for (int i = 0; i <= 7; i++) {
		if (beamformer[i].nextPointTime <= unixTime) {
			bfEnable |= (0x01 << i);
			beamformer[i].lastPointTime = unixTime;
			for (int j = 0; j <= 15; j++) {
				beamformer[i].lastXDelays[j] = beamformer[i].nextXDelays[j];
				beamformer[i].lastYDelays[j] = beamformer[i].nextYDelays[j];
				beamformer[i].nextXDelays[j] = 0;
				beamformer[i].nextYDelays[j] = 0;
				beamformer[i].nextPointTime = LONG_TIME_AWAY;
			}
		}
	}
	send_bitstring_en(bfEnable, bfController.lastBFTemps, bfController.lastBFFlags);
	bfController.lastPointTime = unixTime;
	bfController.lastBFEnable = bfEnable;
	sendPointingResult();
}

/* USER CODE END 4 */

/**
  * @brief  Period elapsed callback in non blocking mode
  * @note   This function is called  when TIM7 interrupt took place, inside
  * HAL_TIM_IRQHandler(). It makes a direct call to HAL_IncTick() to increment
  * a global variable "uwTick" used as application time base.
  * @param  htim : TIM handle
  * @retval None
  */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  /* USER CODE BEGIN Callback 0 */

  /* USER CODE END Callback 0 */
  if (htim->Instance == TIM7) {
    HAL_IncTick();
  }
  /* USER CODE BEGIN Callback 1 */
  if (htim->Instance == TIM2) {
	  unixTime++;
  }
  /* USER CODE END Callback 1 */
}

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
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
