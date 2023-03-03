/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * File Name          : freertos.c
  * Description        : Code for freertos applications
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
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "cmsis_os.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "rng.h"
#include <string.h>
#include "queue.h"
#include "crc.h"

/* FreeRTOS+TCP includes. */
#include "FreeRTOS_IP.h"
#include "FreeRTOS_Sockets.h"

/* MQTT library includes. */
#include "core_mqtt.h"

/* MQTT agent include. */
#include "core_mqtt_agent.h"
#include "core_mqtt_agent_command_functions.h"

#include "cJSON.h"
#include "i2c.h"
#include "bfc_config.h"
#include "rtc.h"
#include <stdio.h>
#include "time.h"
#include "iwdg.h"

/*********************** coreMQTT Agent Configurations **********************/
/**
 * @brief The maximum number of pending acknowledgments to track for a single
 * connection.
 *
 * @note The MQTT agent tracks MQTT commands (such as PUBLISH and SUBSCRIBE) th
 * at are still waiting to be acknowledged.  MQTT_AGENT_MAX_OUTSTANDING_ACKS set
 * the maximum number of acknowledgments that can be outstanding at any one time.
 * The higher this number is the greater the agent's RAM consumption will be.
 */
//#define MQTT_AGENT_MAX_OUTSTANDING_ACKS         ( 10U )

/**
 * @brief Time in MS that the MQTT agent task will wait in the Blocked state (so
 * not using any CPU time) for a command to arrive in its command queue before
 * exiting the blocked state so it can call MQTT_ProcessLoop().
 *
 * @note It is important MQTT_ProcessLoop() is called often if there is known
 * MQTT traffic, but calling it too often can take processing time away from
 * lower priority tasks and waste CPU time and power.
 */
//#define MQTT_AGENT_MAX_EVENT_QUEUE_WAIT_TIME    ( 1000 )

/* MQTT Agent ports. */
#include "freertos_agent_message.h"
#include "freertos_command_pool.h"

/* Exponential backoff retry include. */
#include "backoff_algorithm.h"

/* Transport interface implementation include header for plaintext connection. */
#include "transport_plaintext.h"

/* Subscription manager header include. */
//#include "subscription_manager.h"

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
/**
 * @brief Defines the structure to use as the command callback context
 */
struct MQTTAgentCommandContext
{
    MQTTStatus_t xReturnStatus;
    TaskHandle_t xTaskToNotify;
    uint32_t ulCmdCounter;
    void * pArgs;
};

/**
 * @brief Each compilation unit that consumes the NetworkContext must define it.
 * It should contain a single pointer to the type of your desired transport.
 * When using multiple transports in the same compilation unit, define this pointer as void *.
 *
 * @note Transport stacks are defined in FreeRTOS-Plus/Source/Application-Protocols/network_transport.
 */
struct NetworkContext
{
        PlaintextTransportParams_t * pParams;
};

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

#define MQTT_BROKER_ENDPOINT "10.128.0.1"
#define MQTT_BROKER_PORT 1883

/**
 * @brief This app uses task notifications to signal tasks from MQTT callback
 * functions.  mqttexampleMS_TO_WAIT_FOR_NOTIFICATION defines the time, in ticks,
 * to wait for such a callback.
 */
#define MS_TO_WAIT_FOR_NOTIFICATION            ( 10000 )

/**
 * @brief Size of statically allocated buffers for holding topic names and
 * payloads.
 */
#define STRING_BUFFER_LENGTH                   ( 1000 )

/**
 * @brief Delay for each task between publishes.
 */
//#define DELAY_BETWEEN_PUBLISH_OPERATIONS_MS    ( 1000U )

/**
 * @brief The maximum amount of time in milliseconds to wait for the commands
 * to be posted to the MQTT agent should the MQTT agent's command queue be full.
 * Tasks wait in the Blocked state, so don't use any CPU time.
 */
#define MAX_COMMAND_SEND_BLOCK_TIME_MS         ( 500 )

/**
 * @brief Dimensions the buffer used to serialize and deserialize MQTT packets.
 * @note Specified in bytes.  Must be large enough to hold the maximum
 * anticipated MQTT payload.
 */
#define MQTT_AGENT_NETWORK_BUFFER_SIZE    ( 5000U )

/**
 * @brief The length of the queue used to hold commands for the agent.
 */
#define MQTT_AGENT_COMMAND_QUEUE_LENGTH    ( 5U )

/**
 * @brief Timeout for receiving CONNACK after sending an MQTT CONNECT packet.
 * Defined in milliseconds.
 */
#define CONNACK_RECV_TIMEOUT_MS           ( 1000U )

/**
 * @brief The maximum number of retries for network operation with server.
 */
#define RETRY_MAX_ATTEMPTS                ( 5U )

/**
 * @brief The maximum back-off delay (in milliseconds) for retrying failed operation
 *  with server.
 */
#define RETRY_MAX_BACKOFF_DELAY_MS        ( 5000U )

/**
 * @brief The base back-off delay (in milliseconds) to use for network operation retry
 * attempts.
 */
#define RETRY_BACKOFF_BASE_MS             ( 500U )

/**
 * @brief The maximum time interval in seconds which is allowed to elapse
 *  between two Control Packets.
 *
 *  It is the responsibility of the Client to ensure that the interval between
 *  Control Packets being sent does not exceed the this Keep Alive value. In the
 *  absence of sending any other Control Packets, the Client MUST send a
 *  PINGREQ Packet.
 *//*_RB_ Move to be the responsibility of the agent. */
#define KEEP_ALIVE_INTERVAL_SECONDS       ( 60U )

/**
 * @brief Socket send and receive timeouts to use.  Specified in milliseconds.
 */
#define TRANSPORT_SEND_RECV_TIMEOUT_MS    ( 750 )

/**
 * @brief Used to convert times to/from ticks and milliseconds.
 */
#define MILLISECONDS_PER_SECOND           ( 1000U )
#define MILLISECONDS_PER_TICK             ( MILLISECONDS_PER_SECOND / configTICK_RATE_HZ )

/**
 * @brief The number of command structures to allocate in the pool
 * for the agent.
 */
#define MQTT_COMMAND_CONTEXTS_POOL_SIZE         5

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN Variables */

/* Application defined structure that will get typedef'ed to CommandContext_t. */
struct CommandContext
{
    TaskHandle_t xTaskToNotify;
    MQTTStatus_t xReturnStatus;
};

extern struct bf_struct beamformer[8];
extern struct bfc_struct bfController;
extern unsigned long unixTime;
extern unsigned long nextPointTime;

unsigned long *uid = (unsigned long *) 0x1FFF7A10; // Memory address of unique identifier, 3x 32-bit values
unsigned long bfc_uid = 0;
char bfc_name[10] = "bfc";
// Command topic (needs to be static while subscription is active (always))
static char cmdTopic[25] = "command/";

unsigned long cmdCounter = 0; // Counter increments every time a message is published by the BFC

// Task monitoring - the default task will check that other tasks are running, which they indicate
// by setting their flag in taskMonitor. If they don't set it, the watchdog won't get pats.
uint8_t taskMonitor = 0; // Flags set by each task to indicate they are running
#define mask_TaskFlags 0x07 // Mask that matches all flags being set
#define taskflag_MQTT 0x01
#define taskflag_Telemetry 0x02
#define taskflag_Logging 0x04

/* The MAC address array is not declared const as the MAC address will
normally be read from an EEPROM and not hard coded (in real deployed
applications).*/
static uint8_t ucMACAddress[ 6 ] = { 0x00, 0x11, 0x22, 0x33, 0x44, 0x55 };

/* Define the network addressing.  These parameters will be used if either
ipconfigUDE_DHCP is 0 or if ipconfigUSE_DHCP is 1 but DHCP auto configuration
failed. */
static uint8_t ucIPAddress[ 4 ] = { 10, 128, 60, 1 };
static const uint8_t ucNetMask[ 4 ] = { 255, 255, 0, 0 };
static const uint8_t ucGatewayAddress[ 4 ] = { 10, 128, 0, 1 };
static const uint8_t ucDNSServerAddress[ 4 ] = { 10, 128, 0, 1 };

// From mqtt-agent-task.c
static uint32_t ulGlobalEntryTimeMs;
MQTTAgentContext_t xGlobalMqttAgentContext;
static uint8_t xNetworkBuffer[ MQTT_AGENT_NETWORK_BUFFER_SIZE ];
static MQTTAgentMessageContext_t xCommandQueue;
static PlaintextTransportParams_t xPlaintextTransportParams;
static NetworkContext_t xNetworkContext;
//SubscriptionElement_t xGlobalSubscriptionList[ SUBSCRIPTION_MANAGER_MAX_SUBSCRIPTIONS ];

/* USER CODE END Variables */
/* Definitions for defaultTask */
osThreadId_t defaultTaskHandle;
const osThreadAttr_t defaultTask_attributes = {
  .name = "defaultTask",
  .stack_size = 1024 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for ManageMQTTConn */
osThreadId_t ManageMQTTConnHandle;
const osThreadAttr_t ManageMQTTConn_attributes = {
  .name = "ManageMQTTConn",
  .stack_size = 768 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for PingWatchdog */
osThreadId_t PingWatchdogHandle;
const osThreadAttr_t PingWatchdog_attributes = {
  .name = "PingWatchdog",
  .stack_size = 256 * 4,
  .priority = (osPriority_t) osPriorityLow,
};
/* Definitions for Telemetry */
osThreadId_t TelemetryHandle;
const osThreadAttr_t Telemetry_attributes = {
  .name = "Telemetry",
  .stack_size = 768 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for Logging */
osThreadId_t LoggingHandle;
const osThreadAttr_t Logging_attributes = {
  .name = "Logging",
  .stack_size = 768 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};


/* Definitions for I2CSendComplete */
osEventFlagsId_t I2CSendCompleteHandle;
const osEventFlagsAttr_t I2CSendComplete_attributes = {
  .name = "I2CSendComplete"
};
/* Definitions for I2CReceiveComplete */
osEventFlagsId_t I2CReceiveCompleteHandle;
const osEventFlagsAttr_t I2CReceiveComplete_attributes = {
  .name = "I2CReceiveComplete"
};
/* Definitions for SendTelemetry */
osEventFlagsId_t SendTelemetryHandle;
const osEventFlagsAttr_t SendTelemetry_attributes = {
  .name = "SendTelemetry"
};

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */

void I2C_Reset(I2C_HandleTypeDef *hi2c);
void readI2Csensor(int sensorNum, float *bf_volts, int *bf_amps, int *bf_fault);
int checkPowerAlarm();
float readTempSensor();
void sendPointingResult();
void sendLog(char *logLevel, char *logMsg);

/*-----------------------------------------------------------*/

/**
 * @brief Initializes an MQTT context, including transport interface and
 * network buffer.
 *
 * @return `MQTTSuccess` if the initialization succeeds, else `MQTTBadParameter`.
 */
static MQTTStatus_t prvMQTTInit( void );

/**
 * @brief Sends an MQTT Connect packet over the already connected TCP socket.
 *
 * @param[in] pxMQTTContext MQTT context pointer.
 * @param[in] xCleanSession If a clean session should be established.
 *
 * @return `MQTTSuccess` if connection succeeds, else appropriate error code
 * from MQTT_Connect.
 */
static MQTTStatus_t prvMQTTConnect( bool xCleanSession );

/**
 * @brief Connect a TCP socket to the MQTT broker.
 *
 * @param[in] pxNetworkContext Network context.
 *
 * @return `pdPASS` if connection succeeds, else `pdFAIL`.
 */
static BaseType_t prvSocketConnect( NetworkContext_t * pxNetworkContext );

/**
 * @brief Disconnect a TCP connection.
 *
 * @param[in] pxNetworkContext Network context.
 *
 * @return `pdPASS` if disconnect succeeds, else `pdFAIL`.
 */
static BaseType_t prvSocketDisconnect( NetworkContext_t * pxNetworkContext );

/**
 * @brief Callback executed when there is activity on the TCP socket that is
 * connected to the MQTT broker.  If there are no messages in the MQTT agent's
 * command queue then the callback send a message to ensure the MQTT agent
 * task unblocks and can therefore process whatever is necessary on the socket
 * (if anything) as quickly as possible.
 *
 * @param[in] pxSocket Socket with data, unused.
 */
static void prvMQTTClientSocketWakeupCallback( Socket_t pxSocket );

/**
 * @brief Fan out the incoming publishes to the callbacks registered by different
 * tasks. If there are no callbacks registered for the incoming publish, it will be
 * passed to the unsolicited publish handler.
 *
 * @param[in] pMqttAgentContext Agent context.
 * @param[in] packetId Packet ID of publish.
 * @param[in] pxPublishInfo Info of incoming publish.
 */
static void prvIncomingPublishCallback( MQTTAgentContext_t * pMqttAgentContext,
                                        uint16_t packetId,
                                        MQTTPublishInfo_t * pxPublishInfo );

/**
 * @brief Function to attempt to resubscribe to the topics already present in the
 * subscription list.
 *
 * This function will be invoked when this demo requests the broker to
 * reestablish the session and the broker cannot do so. This function will
 * enqueue commands to the MQTT Agent queue and will be processed once the
 * command loop starts.
 *
 * @return `MQTTSuccess` if adding subscribes to the command queue succeeds, else
 * appropriate error code from MQTTAgent_Subscribe.
 * */
static MQTTStatus_t prvHandleResubscribe( void );

/**
 * @brief Passed into MQTTAgent_Subscribe() as the callback to execute when the
 * broker ACKs the SUBSCRIBE message. This callback implementation is used for
 * handling the completion of resubscribes. Any topic filter failed to resubscribe
 * will be removed from the subscription list.
 *
 * See https://freertos.org/mqtt/mqtt-agent-demo.html#example_mqtt_api_call
 *
 * @param[in] pxCommandContext Context of the initial command.
 * @param[in] pxReturnInfo The result of the command.
 */
//static void prvSubscriptionCommandCallback( void * pxCommandContext, MQTTAgentReturnInfo_t * pxReturnInfo );
/**
 * @brief Passed into MQTTAgent_Publish() as the callback to execute when the
 * broker ACKs the PUBLISH message.  Its implementation sends a notification
 * to the task that called MQTTAgent_Publish() to let the task know the
 * PUBLISH operation completed.  It also sets the xReturnStatus of the
 * structure passed in as the command's context to the value of the
 * xReturnStatus parameter - which enables the task to check the status of the
 * operation.
 *
 * See https://freertos.org/mqtt/mqtt-agent-demo.html#example_mqtt_api_call
 *
 * @param[in] pxCommandContext Context of the initial command.
 * @param[in].xReturnStatus The result of the command.
 */
static void prvPublishCommandCallback( MQTTAgentCommandContext_t * pxCommandContext,
                                       MQTTAgentReturnInfo_t * pxReturnInfo );

/**
 * @brief The timer query function provided to the MQTT context.
 *
 * @return Time in milliseconds.
 */
static uint32_t prvGetTimeMs( void );

/* USER CODE END FunctionPrototypes */

void StartDefaultTask(void *argument);
void StartMQTTConn(void *argument);
void StartPingWD(void *argument);
void StartTelemetry(void *argument);
void StartLogging(void *argument);

extern void MX_USB_DEVICE_Init(void);
void MX_FREERTOS_Init(void); /* (MISRA C 2004 rule 8.1) */

void parseTime(const uint8_t *data, int len);

/* Hook prototypes */
void vApplicationStackOverflowHook(xTaskHandle xTask, signed char *pcTaskName);

/* USER CODE BEGIN 4 */
void vApplicationStackOverflowHook(xTaskHandle xTask, signed char *pcTaskName)
{
   /* Run time stack overflow checking is performed if
   configCHECK_FOR_STACK_OVERFLOW is defined to 1 or 2. This hook function is
   called if a stack overflow is detected. */
	printf("Stack Overflow: %s\n", pcTaskName);
}
/* USER CODE END 4 */

/**
  * @brief  FreeRTOS initialization
  * @param  None
  * @retval None
  */
void MX_FREERTOS_Init(void) {
  /* USER CODE BEGIN Init */


  /* USER CODE END Init */

  /* USER CODE BEGIN RTOS_MUTEX */
		  /* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* USER CODE BEGIN RTOS_SEMAPHORES */
		  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
		  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* USER CODE BEGIN RTOS_QUEUES */
		  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* creation of defaultTask */
  defaultTaskHandle = osThreadNew(StartDefaultTask, NULL, &defaultTask_attributes);

  /* creation of ManageMQTTConn */
  //ManageMQTTConnHandle = osThreadNew(StartMQTTConn, NULL, &ManageMQTTConn_attributes);

  /* creation of PingWatchdog */
  PingWatchdogHandle = osThreadNew(StartPingWD, NULL, &PingWatchdog_attributes);

  /* creation of Telemetry */
  TelemetryHandle = osThreadNew(StartTelemetry, NULL, &Telemetry_attributes);

  /* USER CODE BEGIN RTOS_THREADS */
  LoggingHandle = osThreadNew(StartLogging, NULL, &Logging_attributes);

  /* USER CODE END RTOS_THREADS */

  /* USER CODE BEGIN RTOS_EVENTS */

  /* creation of I2CSendComplete */
  I2CSendCompleteHandle = osEventFlagsNew(&I2CSendComplete_attributes);

  /* creation of I2CReceiveComplete */
  I2CReceiveCompleteHandle = osEventFlagsNew(&I2CReceiveComplete_attributes);

  /* creation of SendTelemetry */
  SendTelemetryHandle = osEventFlagsNew(&SendTelemetry_attributes);

  /* USER CODE END RTOS_EVENTS */

}

/* USER CODE BEGIN Header_StartDefaultTask */
/**
  * @brief  Function implementing the defaultTask thread.
  * @param  argument: Not used
  * @retval None
  */
/* USER CODE END Header_StartDefaultTask */
void StartDefaultTask(void *argument)
{
  /* init code for USB_DEVICE */
  MX_USB_DEVICE_Init();
  /* USER CODE BEGIN StartDefaultTask */

  HAL_IWDG_Refresh(&hiwdg); // Pat the watchdog

  // Calculate a quasi-unique 32-bit value to use as MAC and create BFC name
  bfc_uid = HAL_CRC_Calculate(&hcrc, uid, 3);
  uint8_t idByte1 = (bfc_uid >> 16) & 0xFF;
  uint8_t idByte2 = (bfc_uid >> 8) & 0xFF;
  uint8_t idByte3 = bfc_uid & 0xFF;
  ucMACAddress[0] = 0x00;
  ucMACAddress[1] = 0x80;
  ucMACAddress[2] = 0xE1; // ST Micro MAC Prefix
  ucMACAddress[3] = idByte1;
  ucMACAddress[4] = idByte2;
  ucMACAddress[5] = idByte3;
  ucIPAddress[3] = idByte3; // Use last byte of ID for last byte of IP Address and hope for no conflicts
  snprintf(&bfc_name[strlen(bfc_name)], 7, "%.2X%.2X%.2X", idByte1, idByte2, idByte3);
  printf("BFC Name: %s\n", bfc_name);

  strcat(cmdTopic, bfc_name);
  strcat(cmdTopic, "/#");

  // Initialise the beamformer data structure
  int i, j;
  for (i=0;i<=7;i++) {
	  for (j=0;j<=15;j++) {
		  beamformer[i].lastXDelays[j] = 0;
		  beamformer[i].lastYDelays[j] = 0;
		  beamformer[i].nextXDelays[j] = 0;
		  beamformer[i].lastYDelays[j] = 0;
	  }
	  beamformer[i].lastTemp = -256.0;
	  beamformer[i].lastFlags = -1;
	  beamformer[i].lastPointTime = 0;
	  beamformer[i].nextPointTime = LONG_TIME_AWAY;
  }

  // Set unixTime from RTC
  RTC_DateTypeDef rtc_date;
  RTC_TimeTypeDef rtc_time;
  struct tm dateTime;
  HAL_RTC_GetTime(&hrtc, &rtc_time, RTC_FORMAT_BIN);
  HAL_RTC_GetDate(&hrtc, &rtc_date, RTC_FORMAT_BIN);
  printf("Startup Time: %04d-%02d-%02d - %02d:%02d:%02d\n", rtc_date.Year+2000, rtc_date.Month, rtc_date.Date, rtc_time.Hours, rtc_time.Minutes, rtc_time.Seconds);
  dateTime.tm_sec = rtc_time.Seconds;
  dateTime.tm_min = rtc_time.Minutes;
  dateTime.tm_hour = rtc_time.Hours;
  dateTime.tm_mday = rtc_date.Date;
  dateTime.tm_mon = rtc_date.Month -1;
  dateTime.tm_year = rtc_date.Year + 100;
  unixTime = mktime(&dateTime);
  printf("unixTime: %.lu\n", unixTime);

  FreeRTOS_IPInit( ucIPAddress, ucNetMask, ucGatewayAddress, ucDNSServerAddress, ucMACAddress );

  /* Infinite loop */
  for(;;)
  {
	  HAL_GPIO_TogglePin(LED1_GPIO_Port, LED1_Pin);
	  //printf("T: %.lu\n", unixTime);
	  if (unixTime > 10 && unixTime % 10 == 0) {
//		  RTC_DateTypeDef rtc_date;
//		  RTC_TimeTypeDef rtc_time;
//		  HAL_RTC_GetTime(&hrtc, &rtc_time, RTC_FORMAT_BIN);
//		  HAL_RTC_GetDate(&hrtc, &rtc_date, RTC_FORMAT_BIN);
//		  printf("Time: %.lu: %04d-%02d-%02d - %02d:%02d:%02d\n", unixTime, rtc_date.Year+2000, rtc_date.Month, rtc_date.Date, rtc_time.Hours, rtc_time.Minutes, rtc_time.Seconds);
	//	  doBFTest();
	  }
	  if (unixTime >= nextPointTime) {
		  printf("%.lu: Time to send next pointing\n",unixTime);
		  sendBFPointing();
		  nextPointTime = LONG_TIME_AWAY;
		  for (int i=0; i<=7; i++) {
			  if (beamformer[i].nextPointTime < nextPointTime) nextPointTime = beamformer[i].nextPointTime;
		  }
		  if (nextPointTime <= unixTime) nextPointTime = LONG_TIME_AWAY;
	  }
	  if(xGlobalMqttAgentContext.mqttContext.connectStatus == MQTTConnected) {
		  if(taskMonitor == mask_TaskFlags) {
			  HAL_IWDG_Refresh(&hiwdg); // Pat the watchdog
			  taskMonitor = 0;
		  }
	  }
	  osDelay(1000);
  }
  /* USER CODE END StartDefaultTask */
}

/* USER CODE BEGIN Header_StartMQTTConn */
/**
* @brief Function implementing the ManageMQTTConn thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartMQTTConn */
void StartMQTTConn(void *argument)
{
  /* USER CODE BEGIN StartMQTTConn */
	ulGlobalEntryTimeMs = prvGetTimeMs();

	xNetworkContext.pParams = &xPlaintextTransportParams;

	/* Wait for Networking */
	if( FreeRTOS_IsNetworkUp() == pdFALSE )
	{
		LogInfo( ( "Waiting for the network link up event..." ) );

		while( FreeRTOS_IsNetworkUp() == pdFALSE )
		{
			vTaskDelay( pdMS_TO_TICKS( 1000U ) );
		}
	}

	/* Create the TCP connection to the broker, then the MQTT connection to the
	 * same. */
	BaseType_t xNetworkStatus = pdFAIL;
	MQTTStatus_t xMQTTStatus;
	MQTTContext_t * pMqttContext = &( xGlobalMqttAgentContext.mqttContext );

	/* Connect a TCP socket to the broker. */
	xNetworkStatus = prvSocketConnect( &xNetworkContext );
	configASSERT( xNetworkStatus == pdPASS );

	/* Initialize the MQTT context with the buffer and transport interface. */
	xMQTTStatus = prvMQTTInit();
	configASSERT( xMQTTStatus == MQTTSuccess );

	/* Form an MQTT connection without a persistent session. */
	xMQTTStatus = prvMQTTConnect( true );
	configASSERT( xMQTTStatus == MQTTSuccess );


  /* Infinite loop */
  do
  {
	/* MQTTAgent_CommandLoop() is effectively the agent implementation.  It
	* will manage the MQTT protocol until such time that an error occurs,
	* which could be a disconnect.  If an error occurs the MQTT context on
	* which the error happened is returned so there can be an attempt to
	* clean up and reconnect however the application writer prefers. */
	xMQTTStatus = MQTTAgent_CommandLoop( &xGlobalMqttAgentContext );

	taskMonitor &= taskflag_MQTT; // Set the task flag to indicate task is still running

	/* Success is returned for disconnect or termination. The socket should
	* be disconnected. */
	if( ( xMQTTStatus == MQTTSuccess ) && ( xGlobalMqttAgentContext.mqttContext.connectStatus == MQTTNotConnected ) )
	{
		/* MQTT Disconnect. Disconnect the socket. */
		xNetworkStatus = prvSocketDisconnect( &xNetworkContext );
		configASSERT( xNetworkStatus == pdPASS );
	}
	else if( xMQTTStatus == MQTTSuccess )
	{
	  /* MQTTAgent_Terminate() was called, but MQTT was not disconnected. */
	  xMQTTStatus = MQTT_Disconnect( &( xGlobalMqttAgentContext.mqttContext ) );
	  configASSERT( xMQTTStatus == MQTTSuccess );
	  xNetworkStatus = prvSocketDisconnect( &xNetworkContext );
	  configASSERT( xNetworkStatus == pdPASS );
	}
	/* Handle Error. */
	else
	{
	  /* Reconnect TCP. */
		xNetworkStatus = prvSocketDisconnect( &xNetworkContext );
		configASSERT( xNetworkStatus == pdPASS );
		xNetworkStatus = prvSocketConnect( &xNetworkContext );
		configASSERT( xNetworkStatus == pdPASS );
		pMqttContext->connectStatus = MQTTNotConnected;
		/* MQTT Connect with a persistent session. */
		xMQTTStatus = prvMQTTConnect( false );
		configASSERT( xMQTTStatus == MQTTSuccess );
		xMQTTStatus = 1; // Restart the command loop
	}
  } while (xMQTTStatus != MQTTSuccess);

  /* Should not get here.  Force an assert if the task exits from loop */
  configASSERT( 0 );

  /* USER CODE END StartMQTTConn */
}

/* USER CODE BEGIN Header_StartPingWD */
/**
* @brief Function implementing the PingWatchdog thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartPingWD */
void StartPingWD(void *argument)
{
  /* USER CODE BEGIN StartPingWD */
  /* Infinite loop */
  for(;;)
  {
    osDelay(103);
  }
  /* USER CODE END StartPingWD */
}

/* USER CODE BEGIN Header_StartTelemetry */
/**
* @brief Function implementing the Telemetry thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartTelemetry */
void StartTelemetry(void *argument)
{
  /* USER CODE BEGIN StartTelemetry */
	BaseType_t xReturn;
	int i;
	osDelay(12000); // short delay to let services connect

  /* Infinite loop */
  for(;;)
  {
	  for (i=0; i<=7; i++)
	  {
		  readI2Csensor(i, &beamformer[i].volts, &beamformer[i].amps, &beamformer[i].fault);
		  beamformer[i].last_read = unixTime;
	  }
	  bfController.temp = readTempSensor();
	  if(xGlobalMqttAgentContext.mqttContext.connectStatus == MQTTConnected) {
		  // Send new telemetry data - build JSON packet, publish to MQTT broker
		  cJSON *telemetryPacketJSON = cJSON_CreateObject();
		  cJSON_AddNumberToObject(telemetryPacketJSON, "time", unixTime);
		  char float_val[10];
		  snprintf(float_val, 9, "%.3f", bfController.temp);
		  cJSON_AddRawToObject(telemetryPacketJSON, "bfc_temp", float_val);
		  if (checkPowerAlarm()) {
			  cJSON_AddTrueToObject(telemetryPacketJSON, "pwr_gd");
		  } else {
			  cJSON_AddFalseToObject(telemetryPacketJSON, "pwr_gd");
		  }
		  cJSON *beamformerArrayJSON = cJSON_AddArrayToObject(telemetryPacketJSON, "bfs");
		  for (i=0; i<=7; i++) {
			  cJSON *bf_item = cJSON_CreateObject();
			  cJSON *doc_bool_item = NULL;
			  if (beamformer[i].doc_pwr) doc_bool_item = cJSON_CreateTrue();
			  else doc_bool_item = cJSON_CreateFalse();
			  cJSON_AddItemToObject(bf_item, "doc_pwr", doc_bool_item);
			  cJSON *bf_bool_item = NULL;
			  if (beamformer[i].bf_pwr) bf_bool_item = cJSON_CreateTrue();
			  else bf_bool_item = cJSON_CreateFalse();
			  cJSON_AddItemToObject(bf_item, "bf_pwr", bf_bool_item);
			  char float_val[10];
			  snprintf(float_val, 9, "%.2f", beamformer[i].volts);
			  cJSON *raw_item = cJSON_CreateRaw(float_val);
			  cJSON_AddItemToObject(bf_item, "bf_V", raw_item);
			  cJSON *int_item = cJSON_CreateNumber(beamformer[i].amps);
			  cJSON_AddItemToObject(bf_item, "bf_I", int_item);
			  cJSON *fault_bool_item = NULL;
			  if (beamformer[i].fault) fault_bool_item = cJSON_CreateTrue();
			  else fault_bool_item = cJSON_CreateFalse();
			  cJSON_AddItemToObject(bf_item, "bf_fault", fault_bool_item);

			  cJSON_AddNumberToObject(bf_item, "last_time", beamformer[i].lastPointTime);
			  snprintf(float_val, 9, "%.1f", beamformer[i].lastTemp);
			  cJSON_AddRawToObject(bf_item, "last_temp", float_val);
			  cJSON_AddNumberToObject(bf_item, "last_flags", beamformer[i].lastFlags);
			  cJSON_AddItemToArray(beamformerArrayJSON, bf_item);
		  }
		  char *payload = NULL;
		  payload = cJSON_PrintUnformatted(telemetryPacketJSON);
		  configASSERT(payload != NULL);

		  printf("Telemetry payload: %s\n", payload);

		  MQTTPublishInfo_t xPublishInfo;
		  /* The context for the completion callback must stay in scope until
		   * the completion callback is invoked. In this case, using a stack variable
		   * is safe because the task waits for the callback to execute before continuing. */
		  MQTTAgentCommandContext_t xCommandContext;
		  MQTTAgentCommandInfo_t xCommandInformation;
		  char topic[25] = "telemetry/";
		  strcat(topic, bfc_name);

		  memset( (void *) &xPublishInfo, 0x00, sizeof( xPublishInfo ) );
		  xPublishInfo.qos = MQTTQoS0;
		  xPublishInfo.pTopicName = topic;
		  xPublishInfo.topicNameLength = (uint16_t) strlen(topic);
		  xPublishInfo.pPayload = payload;
		  xPublishInfo.payloadLength = strlen(payload);

		  memset( (void *) &xCommandContext, 0x00, sizeof( xCommandContext ) );
		  xCommandContext.xTaskToNotify = xTaskGetCurrentTaskHandle();
		  xCommandContext.ulCmdCounter = cmdCounter++;

		  xCommandInformation.cmdCompleteCallback = prvPublishCommandCallback;
		  xCommandInformation.pCmdCompleteCallbackContext = &xCommandContext;
		  xCommandInformation.blockTimeMs = MAX_COMMAND_SEND_BLOCK_TIME_MS;

		  MQTTStatus_t xCommandAdded = MQTTAgent_Publish(&xGlobalMqttAgentContext, &xPublishInfo, &xCommandInformation );

		  if( xCommandAdded == MQTTSuccess ) {
			  //printf("%s", "MQTT Publish command sent to agent thread successfully\n");
			  /* The command was successfully sent to the agent.  Note the data
			  pointed to by xPublishInfo.pTopicName and xPublishInfo.pPayload must
			  remain valid (not be lost from a stack frame or overwritten in a buffer)
			  until the PUBLISH is acknowledged. */
			  xReturn = xTaskNotifyWait( 0, 0, NULL, portMAX_DELAY ); /* Wait indefinitely. */

			  if( xReturn != pdFAIL )
			  {
				  /* The message was acknowledged and
				  xCommandContext.xReturnStatus holds the result of the operation. */
				  //printf("%s", "MQTT Publish command successfully acknowledged by broker\n");
			  }
		  }

		  cJSON_Delete(telemetryPacketJSON);
		  cJSON_free(payload);

	  }
	  // Wait until we're flagged to send telemetry again, or after the set period
	  // We need to wake up every 10s or so to set the task flag, so the watchdog gets pats
	  int msCounter = 0;
	  while (msCounter < TELEMETRY_PERIOD) {
		  if (msCounter + 10000 > TELEMETRY_PERIOD) {
			  taskMonitor &= taskflag_Telemetry; // Set the task flag to indicate task is still running
			  xTaskNotifyWait( 0, 0xFFFFFFFF, NULL, pdMS_TO_TICKS(10000));
			  msCounter += 10000;
		  } else {
			  taskMonitor &= taskflag_Telemetry; // Set the task flag to indicate task is still running
			  xTaskNotifyWait( 0, 0xFFFFFFFF, NULL, pdMS_TO_TICKS(TELEMETRY_PERIOD - msCounter));
		  }
	  }
  }
  /* USER CODE END StartTelemetry */
}

void StartLogging(void *argument) {
	BaseType_t xStatus;

	for (;;) {
		osDelay(5107);
		taskMonitor &= taskflag_Logging; // Set the task flag to indicate task is still running
	}
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

int checkPowerAlarm() {
	return HAL_GPIO_ReadPin(PRAlarm_GPIO_Port, PRAlarm_Pin);
}

void readI2Csensor(int sensorNum, float* bf_volts, int* bf_amps, int* bf_fault)
// Read the registers from one LTC4151 sensor and calculate the resulting values
{
	const uint16_t sensorAddr[8] = {BF1_SENSOR_ADDR, BF2_SENSOR_ADDR, BF3_SENSOR_ADDR, BF4_SENSOR_ADDR, BF5_SENSOR_ADDR, BF6_SENSOR_ADDR, BF7_SENSOR_ADDR, BF8_SENSOR_ADDR};
	uint8_t sData[8] = {0};
	uint32_t flag_ret;

	HAL_StatusTypeDef result = HAL_I2C_Master_Seq_Transmit_IT(&hi2c1, sensorAddr[sensorNum], (uint8_t*) sData, 1, I2C_FIRST_FRAME);
	if (result == HAL_OK) {
		// wait for transmission to finish, signalled by flag from callback function
		flag_ret = osEventFlagsWait(I2CSendCompleteHandle, EVENT_FLAG, osFlagsWaitAny, I2C_TIMEOUT);
		if (flag_ret != EVENT_FLAG) {
			// I2C error
			printf("I2C Timeout Error waiting for Addr Transmit: %d\n", (int)HAL_I2C_GetError(&hi2c1));
			I2C_Reset(&hi2c1);
		}
		else {
			result = HAL_I2C_Master_Seq_Receive_IT(&hi2c1, sensorAddr[sensorNum], (uint8_t*) sData, 6, I2C_LAST_FRAME);
			if (result == HAL_OK) {
				// wait for receive to finish, signalled by flag from callback function
				flag_ret = osEventFlagsWait(I2CReceiveCompleteHandle, EVENT_FLAG, osFlagsWaitAny, I2C_TIMEOUT);
				if (flag_ret != EVENT_FLAG) {
					// I2C error
					printf("I2C Timeout Error waiting for Data Receive: %d\n", (int)HAL_I2C_GetError(&hi2c1));
					I2C_Reset(&hi2c1);
				}
				else {
					// Successful read of 6 bytes of I2C sensor data - process into V & I
					uint16_t amps_int = ((uint16_t)sData[0] << 4) + ((sData[1] & 0xF0) >> 4);
					*bf_amps = (int) amps_int;
					uint16_t volts_int = ((uint16_t)sData[2] << 4) + ((sData[3] & 0xF0) >> 4);
					*bf_volts = (float) volts_int * 25e-3;
					uint16_t adc_int = (sData[4] << 4) + ((sData[5] & 0xF0) >> 4);
					if (volts_int > 1640 && adc_int > 810) *bf_fault = 0; else *bf_fault = 1;
				}
			}
			else {
				// Error on I2C receive
				printf("I2C HAL Error Receiving Data: %d\n", result);
				I2C_Reset(&hi2c1);
			}
		}
	}
	else {
		// Error on I2C transmit
		printf("I2C HAL Error Sending Register Addr: %d\n", result);
		I2C_Reset(&hi2c1);
	}

}

float readTempSensor() {
	uint32_t flag_ret;
	uint8_t sData[3] = {0};

	if (HAL_I2C_Master_Seq_Transmit_IT(&hi2c1, TEMP_SENSOR_ADDR, (uint8_t*) sData, 1, I2C_FIRST_FRAME) == HAL_OK) {
		// wait for transmission to finish, signalled by flag from callback function
		flag_ret = osEventFlagsWait(I2CSendCompleteHandle, EVENT_FLAG, osFlagsWaitAny, I2C_TIMEOUT);
		if (flag_ret != EVENT_FLAG) {
			// I2C error
			printf("%s", "readTempSensor: I2C Timeout Error waiting for Addr Transmit\n");
		}
		else {
			if (HAL_I2C_Master_Seq_Receive_IT(&hi2c1, TEMP_SENSOR_ADDR, (uint8_t*) sData, 2, I2C_LAST_FRAME) == HAL_OK) {
				// wait for receive to finish, signalled by flag from callback function
				flag_ret = osEventFlagsWait(I2CReceiveCompleteHandle, EVENT_FLAG, osFlagsWaitAny, I2C_TIMEOUT);
				if (flag_ret != EVENT_FLAG) {
					// I2C error
					printf("%s", "readTempSensor: I2C Timeout Error waiting for Data Receive\n");
				}
				else {
					// Successful read of 2 bytes of temp sensor data - process into float temp
					//printf("Read Temp Sensor, 0:%#.2x, 1:%#.2x\n",sData[0],sData[1]);
					int temp = sData[1] >> 5;
					temp += sData[0] << 3;
					return (float) temp * 0.125;
				}
			}
			else {
				// Error on I2C receive
				printf("%s", "readTempSensor: I2C HAL Error Receiving Data\n");
				I2C_Reset(&hi2c1);
			}
		}
	}
	else {
		// Error on I2C transmit
		printf("%s", "readTempSensor: I2C HAL Error Sending Register Addr\n");
		I2C_Reset(&hi2c1);
	}

	return -256.0;
}

void HAL_I2C_MasterTxCpltCallback (I2C_HandleTypeDef * hi2c) {
	uint32_t ret = osEventFlagsSet(I2CSendCompleteHandle, EVENT_FLAG);
	if (ret != EVENT_FLAG) {
		printf("%s", "HAL_I2C_MasterTxCpltCallback: Error setting flag\n");
	}
}

void HAL_I2C_MasterRxCpltCallback (I2C_HandleTypeDef * hi2c) {
	uint32_t ret = osEventFlagsSet(I2CReceiveCompleteHandle, EVENT_FLAG);
	if (ret != EVENT_FLAG) {
		printf("%s", "HAL_I2C_MasterRxCpltCallback: Error setting flag\n");
	}
}

void HAL_I2C_ErrorCallback(I2C_HandleTypeDef * hi2c) {
	printf("I2C Error, aborting...\n");
}

void I2C_Reset(I2C_HandleTypeDef *hi2c) {
	printf("I2C1 Reset\n");
	//HAL_I2C_DeInit(&hi2c);
	//osDelay(100);
	//HAL_I2C_Init(&hi2c);
	//osDelay(100);
}

void sendPointingResult() {
	if(xGlobalMqttAgentContext.mqttContext.connectStatus == MQTTConnected) {
		// Send result message - build JSON packet, publish to MQTT broker
		cJSON *resultPacketJSON = cJSON_CreateObject();
		cJSON_AddNumberToObject(resultPacketJSON, "time", bfController.lastPointTime);
		cJSON *bf_tempsJSON = cJSON_AddArrayToObject(resultPacketJSON, "bf_temps");
		int i;
		for (i=0; i<=7; i++) {
		  char float_val[10];
		  if (bfController.lastBFEnable & (0x01 << i)) snprintf(float_val, 9, "%.1f", bfController.lastBFTemps[i]);
		  else snprintf(float_val, 9, "%.1f", -256.0);
		  cJSON *raw_item = cJSON_CreateRaw(float_val);
		  cJSON_AddItemToArray(bf_tempsJSON, raw_item);
		}
		cJSON *bf_flagsJSON = cJSON_AddArrayToObject(resultPacketJSON, "bf_flags");
		for (i=0; i<=7; i++) {
			cJSON *int_item = NULL;
			if (bfController.lastBFEnable & (0x01 << i)) int_item = cJSON_CreateNumber(bfController.lastBFFlags[i]);
			else int_item = cJSON_CreateNumber(-1);
		  cJSON_AddItemToArray(bf_flagsJSON, int_item);
		}
		char *payload = NULL;
		payload = cJSON_PrintUnformatted(resultPacketJSON);

		printf("Payload: %s\n", payload);

		MQTTPublishInfo_t xPublishInfo;
		/* The context for the completion callback must stay in scope until
		* the completion callback is invoked. In this case, using a stack variable
		* is safe because the task waits for the callback to execute before continuing. */
		MQTTAgentCommandContext_t xCommandContext;
		MQTTAgentCommandInfo_t xCommandInformation;
		BaseType_t xReturn;
		char topic[25] = "result/";
		strcat(topic, bfc_name);
		strcat(topic, "/point");

		memset( (void *) &xPublishInfo, 0x00, sizeof( xPublishInfo ) );
		xPublishInfo.qos = MQTTQoS0;
		xPublishInfo.pTopicName = topic;
		xPublishInfo.topicNameLength = (uint16_t) strlen(topic);
		xPublishInfo.pPayload = payload;
		xPublishInfo.payloadLength = strlen(payload);

		memset( (void *) &xCommandContext, 0x00, sizeof( xCommandContext ) );
		xCommandContext.xTaskToNotify = xTaskGetCurrentTaskHandle();
		xCommandContext.ulCmdCounter = cmdCounter++;

		xCommandInformation.cmdCompleteCallback = prvPublishCommandCallback;
		xCommandInformation.pCmdCompleteCallbackContext = &xCommandContext;
		xCommandInformation.blockTimeMs = MAX_COMMAND_SEND_BLOCK_TIME_MS;

		MQTTStatus_t xCommandAdded = MQTTAgent_Publish(&xGlobalMqttAgentContext, &xPublishInfo, &xCommandInformation );

		if( xCommandAdded == MQTTSuccess ) {
		  //printf("%s", "MQTT Publish command sent to agent thread successfully\n");
		  /* The command was successfully sent to the agent.  Note the data
		  pointed to by xPublishInfo.pTopicName and xPublishInfo.pPayload must
		  remain valid (not be lost from a stack frame or overwritten in a buffer)
		  until the PUBLISH is acknowledged. */
		  xReturn = xTaskNotifyWait( 0, 0, NULL, portMAX_DELAY ); /* Wait indefinitely. */

		  if( xReturn != pdFAIL )
		  {
			  /* The message was acknowledged and
			  xCommandContext.xReturnStatus holds the result of the operation. */
			  //printf("%s", "MQTT Publish command successfully acknowledged by broker\n");
		  }
		} else {
			printf("%s","Failed to enqueue message for MQTT Agent Publish - Pointing Result\n");
		}

		cJSON_Delete(resultPacketJSON);
		cJSON_free(payload);
	}
}

void sendLog(char *logLevel, char *logMsg) {
	// Publish a character string log message to log/bfc#
	if(xGlobalMqttAgentContext.mqttContext.connectStatus == MQTTConnected) {
		cJSON *logPacketJSON = cJSON_CreateObject();
		cJSON_AddStringToObject(logPacketJSON, logLevel, logMsg);
		char *payload = cJSON_PrintUnformatted(logPacketJSON);

		printf("Payload: %s\n", payload);

		MQTTPublishInfo_t xPublishInfo;
		/* The context for the completion callback must stay in scope until
		* the completion callback is invoked. In this case, using a stack variable
		* is safe because the task waits for the callback to execute before continuing. */
		MQTTAgentCommandContext_t xCommandContext;
		MQTTAgentCommandInfo_t xCommandInformation;
		BaseType_t xReturn;
		char topic[20] = "log/";
		strcat(topic, bfc_name);

		memset( (void *) &xPublishInfo, 0x00, sizeof( xPublishInfo ) );
		xPublishInfo.qos = MQTTQoS0;
		xPublishInfo.pTopicName = topic;
		xPublishInfo.topicNameLength = (uint16_t) strlen(topic);
		xPublishInfo.pPayload = payload;
		xPublishInfo.payloadLength = strlen(payload);

		memset( (void *) &xCommandContext, 0x00, sizeof( xCommandContext ) );
		xCommandContext.xTaskToNotify = xTaskGetCurrentTaskHandle();
		xCommandContext.ulCmdCounter = cmdCounter++;

		xCommandInformation.cmdCompleteCallback = prvPublishCommandCallback;
		xCommandInformation.pCmdCompleteCallbackContext = &xCommandContext;
		xCommandInformation.blockTimeMs = MAX_COMMAND_SEND_BLOCK_TIME_MS;

		MQTTStatus_t xCommandAdded = MQTTAgent_Publish(&xGlobalMqttAgentContext, &xPublishInfo, &xCommandInformation );

		if( xCommandAdded == MQTTSuccess ) {
		  //printf("%s", "MQTT Publish command sent to agent thread successfully\n");
		  /* The command was successfully sent to the agent.  Note the data
		  pointed to by xPublishInfo.pTopicName and xPublishInfo.pPayload must
		  remain valid (not be lost from a stack frame or overwritten in a buffer)
		  until the PUBLISH is acknowledged. */
		  xReturn = xTaskNotifyWait( 0, 0, NULL, portMAX_DELAY ); /* Wait indefinitely. */

		  if( xReturn != pdFAIL )
		  {
			  /* The message was acknowledged and
			  xCommandContext.xReturnStatus holds the result of the operation. */
			  //printf("%s", "MQTT Publish command successfully acknowledged by broker\n");
		  }
		} else {
			printf("%s","Failed to enqueue message for MQTT Agent Publish - Log Message\n");
		}

		cJSON_Delete(logPacketJSON);
		cJSON_free(payload);
	}
}

/*-----------------------------------------------------------*/

static void prvPublishCommandCallback( MQTTAgentCommandContext_t * pxCommandContext,
                                       MQTTAgentReturnInfo_t * pxReturnInfo )
{
    /* Store the result in the application defined context so the task that
     * initiated the publish can check the operation's status. */
    pxCommandContext->xReturnStatus = pxReturnInfo->returnCode;

    if( pxCommandContext->xTaskToNotify != NULL )
    {
        /* Send the context's ulNotificationValue as the notification value so
         * the receiving task can check the value it set in the context matches
         * the value it receives in the notification. */
        xTaskNotify( pxCommandContext->xTaskToNotify,
                     pxCommandContext->ulCmdCounter,
                     eSetValueWithOverwrite );
    }
}

void vApplicationIPNetworkEventHook( eIPCallbackEvent_t eNetworkEvent ) {
static BaseType_t xTasksAlreadyCreated = pdFALSE;

    /* Both eNetworkUp and eNetworkDown events can be processed here. */
    if( eNetworkEvent == eNetworkUp )
    {
        /* Create the tasks that use the TCP/IP stack if they have not already
        been created. */
        if( xTasksAlreadyCreated == pdFALSE )
        {
            /*
             * For convenience, tasks that use FreeRTOS-Plus-TCP can be created here
             * to ensure they are not created before the network is usable.
             */
        	/* creation of ManageMQTTConn */
        	ManageMQTTConnHandle = osThreadNew(StartMQTTConn, NULL, &ManageMQTTConn_attributes);

            xTasksAlreadyCreated = pdTRUE;
        }
    }
}

BaseType_t xApplicationGetRandomNumber( uint32_t * pulNumber ) {
	if (HAL_RNG_GenerateRandomNumber(&hrng, pulNumber) == HAL_OK) return pdTRUE; else return pdFALSE;
}

uint32_t ulApplicationGetNextSequenceNumber( uint32_t ulSourceAddress, uint16_t usSourcePort, uint32_t ulDestinationAddress, uint16_t usDestinationPort ) {
	uint32_t random = 0;
	if (HAL_RNG_GenerateRandomNumber(&hrng, &random) == HAL_OK) return random; else return 0U;
}

uint32_t uxRand() {
	uint32_t random = 0;
	if (HAL_RNG_GenerateRandomNumber(&hrng, &random) == HAL_OK) return random; else return 0U;
}

/*-----------------------------------------------------------*/

static void prvIncomingPublishCallback( MQTTAgentContext_t * pMqttAgentContext,
                                        uint16_t packetId,
                                        MQTTPublishInfo_t * pxPublishInfo ) {

	HAL_GPIO_TogglePin(LED3_GPIO_Port, LED3_Pin); //toggle pin when new message arrived
	printf("Incoming Publish: %.*s - %.*s\n",pxPublishInfo->topicNameLength, pxPublishInfo->pTopicName, pxPublishInfo->payloadLength, (char*)pxPublishInfo->pPayload);

	int incomingTopicID = CMD_TOPICERROR;
	char* topic = pxPublishInfo->pTopicName;
	// Decode topic string into an ID
	if (pxPublishInfo->topicNameLength > 6) {
		if(strncmp(topic, "command/", 8) == 0) {
			char* substr = topic + 8;
			if(strncmp(substr, bfc_name, strlen(bfc_name)) == 0) {
				substr += strlen(bfc_name) + 1;
				if(strncmp(substr, "bfs", 3) == 0) incomingTopicID = CMD_BFPWR;
				else if(strncmp(substr, "docs", 4) == 0) incomingTopicID = CMD_DOCPWR;
				else if(strncmp(substr, "time", 4) == 0) incomingTopicID = CMD_SETTIME;
				else if(strncmp(substr, "point", 5) == 0) incomingTopicID = CMD_POINT;
				else if(strncmp(substr, "bftest", 6) == 0) incomingTopicID = CMD_BFTEST;
				else if(strncmp(substr, "status", 6) == 0) incomingTopicID = CMD_STATUS;
				else incomingTopicID = CMD_TOPICERROR;
			} else incomingTopicID = CMD_TOPICERROR;
		} else incomingTopicID = CMD_TOPICERROR;
	}

	printf("Command Decoded to: %d\n", incomingTopicID);

	uint8_t* data = (uint8_t*)pxPublishInfo->pPayload;
	int len = (int)pxPublishInfo->payloadLength;

	switch(incomingTopicID) {
		case CMD_POINT:
			parsePointing(data, len);
			break;
		case CMD_BFTEST:
			// bftest has no payload
			doBFTest();
			break;
		case CMD_DOCPWR:
			parseDoCPower(data, len);
			vTaskDelay(200);
			xTaskNotify(TelemetryHandle, EVENT_FLAG, eSetValueWithOverwrite);
			break;
		case CMD_BFPWR:
			parseBFPower(data, len);
			vTaskDelay(200);
			xTaskNotify(TelemetryHandle, EVENT_FLAG, eSetValueWithOverwrite);
			break;
		case CMD_STATUS:
			// status has no payload, ignored
			xTaskNotify(TelemetryHandle, EVENT_FLAG, eSetValueWithOverwrite);
			break;
		case CMD_TOPICERROR:
			// Ignore the payload for invalid topics
			break;
		case CMD_SETTIME:
			parseTime(data, len);
			break;
		}

}
/*-----------------------------------------------------------*/
void parseTime(const uint8_t *data, int len) {
	RTC_TimeTypeDef rtc_time;
	RTC_DateTypeDef rtc_date;
	struct tm dateTime;

	if (len > 0) {
		cJSON *command = cJSON_ParseWithLength((const char*) data, len);
		cJSON *time_data = cJSON_GetObjectItem(command, "time");
		if (time_data->valuedouble > 1677036957UL) {
			unixTime = time_data->valuedouble;
			uint32_t sec = unixTime;// + SNTP_UNIX_TIMESTAMP_DIFF;

			rtc_time.Hours = (sec/3600) % 24;
			rtc_time.Minutes = (sec/60) % 60;
			rtc_time.Seconds = sec % 60;
			rtc_time.SubSeconds = 107;
			rtc_time.DayLightSaving = RTC_DAYLIGHTSAVING_NONE;
			rtc_time.StoreOperation = RTC_STOREOPERATION_RESET;

			localtime_r((time_t*) &sec, &dateTime);
			rtc_date.Date = dateTime.tm_mday;
			rtc_date.Month = dateTime.tm_mon +1;
			rtc_date.Year = dateTime.tm_year % 100;

			HAL_RTC_SetDate(&hrtc, &rtc_date, RTC_FORMAT_BIN);
			HAL_RTC_SetTime(&hrtc, &rtc_time, RTC_FORMAT_BIN);
			printf("Time set %.lu - %04d-%02d-%02d - %02d:%02d:%02d\n", unixTime, rtc_date.Year+2000, rtc_date.Month, rtc_date.Date, rtc_time.Hours, rtc_time.Minutes, rtc_time.Seconds);
		}
	}
}


/*-----------------------------------------------------------*/

static MQTTStatus_t prvMQTTInit( void )
{
    TransportInterface_t xTransport;
    MQTTStatus_t xReturn;
    MQTTFixedBuffer_t xFixedBuffer = { .pBuffer = xNetworkBuffer, .size = MQTT_AGENT_NETWORK_BUFFER_SIZE };
    //static uint8_t staticQueueStorageArea[ MQTT_AGENT_COMMAND_QUEUE_LENGTH * sizeof( MQTTAgentCommand_t * ) ];
    //static StaticQueue_t staticQueueStructure;
    MQTTAgentMessageInterface_t messageInterface =
    {
        .pMsgCtx        = NULL,
        .send           = Agent_MessageSend,
        .recv           = Agent_MessageReceive,
        .getCommand     = Agent_GetCommand,
        .releaseCommand = Agent_ReleaseCommand
    };

    LogDebug( ( "Creating command queue." ) );
    xCommandQueue.queue = xQueueCreate( MQTT_AGENT_COMMAND_QUEUE_LENGTH,
                                        sizeof( MQTTAgentCommand_t * ) );
    configASSERT( xCommandQueue.queue );
    messageInterface.pMsgCtx = &xCommandQueue;

    /* Initialize the task pool. */
    Agent_InitializePool();

    /* Fill in Transport Interface send and receive function pointers. */
    xTransport.pNetworkContext = &xNetworkContext;
	xTransport.send = Plaintext_FreeRTOS_send;
	xTransport.recv = Plaintext_FreeRTOS_recv;
	xTransport.writev = NULL;

    /* Initialize MQTT library. */
    xReturn = MQTTAgent_Init( &xGlobalMqttAgentContext,
                              &messageInterface,
                              &xFixedBuffer,
                              &xTransport,
                              prvGetTimeMs,
                              prvIncomingPublishCallback,
                              // Context to pass into the callback.
                              NULL );

    return xReturn;
}

/*-----------------------------------------------------------*/

static void prvMQTTClientSocketWakeupCallback( Socket_t pxSocket )
{
    MQTTAgentCommandInfo_t xCommandParams = { 0 };

    /* Just to avoid compiler warnings.  The socket is not used but the function
     * prototype cannot be changed because this is a callback function. */
    ( void ) pxSocket;

    /* A socket used by the MQTT task may need attention.  Send an event
     * to the MQTT task to make sure the task is not blocked on xCommandQueue. */
    if( ( uxQueueMessagesWaiting( xCommandQueue.queue ) == 0U ) && ( FreeRTOS_recvcount( pxSocket ) > 0 ) )
    {
        /* Don't block as this is called from the context of the IP task. */
        xCommandParams.blockTimeMs = 0U;
        MQTTAgent_ProcessLoop( &xGlobalMqttAgentContext, &xCommandParams );
    }
}

/*-----------------------------------------------------------*/

//static void prvSubscriptionCommandCallback( void * pxCommandContext,
//                                            MQTTAgentReturnInfo_t * pxReturnInfo )
//{
//    size_t lIndex = 0;
//    MQTTAgentSubscribeArgs_t * pxSubscribeArgs = ( MQTTAgentSubscribeArgs_t * ) pxCommandContext;
//
//    /* If the return code is success, no further action is required as all the topic filters
//     * are already part of the subscription list. */
//    if( pxReturnInfo->returnCode != MQTTSuccess )
//    {
//        /* Check through each of the suback codes and determine if there are any failures. */
//        for( lIndex = 0; lIndex < pxSubscribeArgs->numSubscriptions; lIndex++ )
//        {
//            /* This demo doesn't attempt to resubscribe in the event that a SUBACK failed. */
//            if( pxReturnInfo->pSubackCodes[ lIndex ] == MQTTSubAckFailure )
//            {
//                LogError( ( "Failed to resubscribe to topic %.*s.",
//                            pxSubscribeArgs->pSubscribeInfo[ lIndex ].topicFilterLength,
//                            pxSubscribeArgs->pSubscribeInfo[ lIndex ].pTopicFilter ) );
//            }
//        }
//
//        /* Hit an assert as some of the tasks won't be able to proceed correctly without
//         * the subscriptions. This logic will be updated with exponential backoff and retry.  */
//        configASSERT( pdTRUE );
//    }
//}

/*-----------------------------------------------------------*/

static MQTTStatus_t prvHandleResubscribe( void )
{
    MQTTStatus_t xResult = MQTTBadParameter;
    // Subscribe to Command topic
	MQTTSubscribeInfo_t xMQTTSubscription;
	uint16_t usSubscribePacketIdentifier = MQTT_GetPacketId( &(xGlobalMqttAgentContext.mqttContext) );

	( void ) memset( ( void * ) &xMQTTSubscription, 0x00, sizeof( xMQTTSubscription ) );
	xMQTTSubscription.qos = MQTTQoS0;
	xMQTTSubscription.pTopicFilter = cmdTopic;
	xMQTTSubscription.topicFilterLength = strlen( cmdTopic );

	xResult = MQTT_Subscribe( &(xGlobalMqttAgentContext.mqttContext), &xMQTTSubscription, 1, usSubscribePacketIdentifier );
	configASSERT(xResult == MQTTSuccess);

    return xResult;
}

/*-----------------------------------------------------------*/

static MQTTStatus_t prvMQTTConnect( bool xCleanSession )
{
    MQTTStatus_t xResult;
    MQTTConnectInfo_t xConnectInfo;
    bool xSessionPresent = false;
    MQTTPublishInfo_t willInfo = { 0 };

    /* Many fields are not used in this demo so start with everything at 0. */
    memset( &xConnectInfo, 0x00, sizeof( xConnectInfo ) );

    /* Start with a clean session i.e. direct the MQTT broker to discard any
     * previous session data. Also, establishing a connection with clean session
     * will ensure that the broker does not store any data when this client
     * gets disconnected. */
    xConnectInfo.cleanSession = xCleanSession;

    /* The client identifier is used to uniquely identify this MQTT client to
     * the MQTT broker. In a production device the identifier can be something
     * unique, such as a device serial number. */
    xConnectInfo.pClientIdentifier = bfc_name;
    xConnectInfo.clientIdentifierLength = ( uint16_t ) strlen( bfc_name );

    /* Set MQTT keep-alive period. It is the responsibility of the application
     * to ensure that the interval between Control Packets being sent does not
     * exceed the Keep Alive value. In the absence of sending any other Control
     * Packets, the Client MUST send a PINGREQ Packet.  This responsibility will
     * be moved inside the agent. */
    xConnectInfo.keepAliveSeconds = KEEP_ALIVE_INTERVAL_SECONDS;

    // The last will and testament is optional, it will be published by the broker
    // should this client disconnect without sending a DISCONNECT packet.
    char willTopic[20] = "status/";
    strcat(willTopic, bfc_name);
    willInfo.qos = MQTTQoS0;
    willInfo.pTopicName = willTopic;
    willInfo.topicNameLength = strlen( willInfo.pTopicName );
    willInfo.pPayload = "OFFLINE";
    willInfo.payloadLength = strlen("OFFLINE");
    willInfo.retain = true;

    // Send MQTT CONNECT packet to broker.
    xResult = MQTT_Connect( &( xGlobalMqttAgentContext.mqttContext ),
                            &xConnectInfo,
                            &willInfo,
                            CONNACK_RECV_TIMEOUT_MS,
                            &xSessionPresent );

    LogInfo( ( "Session present: %d\n", xSessionPresent ) );

    /* Resume a session if desired. */
    if( ( xResult == MQTTSuccess ) && ( xCleanSession == false ) )
    {
        xResult = MQTTAgent_ResumeSession( &xGlobalMqttAgentContext, xSessionPresent );

        /* Resubscribe to all the subscribed topics. */
        if( ( xResult == MQTTSuccess ) && ( xSessionPresent == false ) )
        {
            xResult = prvHandleResubscribe();
        }
    }

    // Publish Online Message
    char payload[30] = "Online ";
	RTC_DateTypeDef rtc_date;
	RTC_TimeTypeDef rtc_time;
	HAL_RTC_GetTime(&hrtc, &rtc_time, RTC_FORMAT_BIN);
	HAL_RTC_GetDate(&hrtc, &rtc_date, RTC_FORMAT_BIN);
	char date[20];
	char time[20];
	sprintf(date, "%04d-%02d-%02d - ", rtc_date.Year+2000, rtc_date.Month, rtc_date.Date);
	sprintf(time, "%02d:%02d:%02d", rtc_time.Hours, rtc_time.Minutes, rtc_time.Seconds);
	strcat(payload, date);
	strcat(payload, time);

	char topic[20] = "status/";
	strcat(topic, bfc_name);

	MQTTPublishInfo_t xPublishInfo;
	memset( (void *) &xPublishInfo, 0x00, sizeof( xPublishInfo ) );
	xPublishInfo.qos = MQTTQoS0;
	xPublishInfo.pTopicName = topic;
	xPublishInfo.topicNameLength = (uint16_t) strlen(topic);
	xPublishInfo.pPayload = payload;
	xPublishInfo.payloadLength = strlen(payload);
	xPublishInfo.retain = true;

	xResult = MQTT_Publish(&(xGlobalMqttAgentContext.mqttContext), &xPublishInfo, 0);

	// Subscribe to Command topic
	MQTTSubscribeInfo_t xMQTTSubscription;
	uint16_t usSubscribePacketIdentifier = MQTT_GetPacketId( &(xGlobalMqttAgentContext.mqttContext) );

	( void ) memset( ( void * ) &xMQTTSubscription, 0x00, sizeof( xMQTTSubscription ) );
	xMQTTSubscription.qos = MQTTQoS0;
	xMQTTSubscription.pTopicFilter = cmdTopic;
	xMQTTSubscription.topicFilterLength = strlen( cmdTopic );

	xResult = MQTT_Subscribe( &(xGlobalMqttAgentContext.mqttContext), &xMQTTSubscription, 1, usSubscribePacketIdentifier );
	configASSERT(xResult == MQTTSuccess);

    return xResult;
}


/*-----------------------------------------------------------*/

static BaseType_t prvSocketConnect( NetworkContext_t * pxNetworkContext )
{
    BaseType_t xConnected = pdPASS;
    BackoffAlgorithmStatus_t xBackoffAlgStatus = BackoffAlgorithmSuccess;
    BackoffAlgorithmContext_t xReconnectParams = { 0 };
    uint16_t usNextRetryBackOff = 0U;
    const TickType_t xTransportTimeout = 0UL;

    PlaintextTransportStatus_t xNetworkStatus = PLAINTEXT_TRANSPORT_CONNECT_FAILURE;

    /* We will use a retry mechanism with an exponential backoff mechanism and
     * jitter.  That is done to prevent a fleet of IoT devices all trying to
     * reconnect at exactly the same time should they become disconnected at
     * the same time. We initialize reconnect attempts and interval here. */
    BackoffAlgorithm_InitializeParams( &xReconnectParams,
                                       RETRY_BACKOFF_BASE_MS,
                                       RETRY_MAX_BACKOFF_DELAY_MS,
                                       RETRY_MAX_ATTEMPTS );

    if( xConnected == pdPASS )
    {
        /* Attempt to connect to MQTT broker. If connection fails, retry after a
         * timeout. Timeout value will exponentially increase until the maximum
         * number of attempts are reached.
         */
        do
        {
			LogInfo( ( "Creating a TCP connection to %s:%d.",
					   MQTT_BROKER_ENDPOINT,
					   MQTT_BROKER_PORT ) );
			xNetworkStatus = Plaintext_FreeRTOS_Connect( pxNetworkContext,
														 MQTT_BROKER_ENDPOINT,
														 MQTT_BROKER_PORT,
														 TRANSPORT_SEND_RECV_TIMEOUT_MS,
														 TRANSPORT_SEND_RECV_TIMEOUT_MS );
			xConnected = ( xNetworkStatus == PLAINTEXT_TRANSPORT_SUCCESS ) ? pdPASS : pdFAIL;

            if( !xConnected )
            {
                /* Get back-off value (in milliseconds) for the next connection retry. */
                xBackoffAlgStatus = BackoffAlgorithm_GetNextBackoff( &xReconnectParams, uxRand(), &usNextRetryBackOff );

                if( xBackoffAlgStatus == BackoffAlgorithmSuccess )
                {
                    LogWarn( ( "Connection to the broker failed. "
                               "Retrying connection in %hu ms.",
                               usNextRetryBackOff ) );
                    vTaskDelay( pdMS_TO_TICKS( usNextRetryBackOff ) );
                }
            }

            if( xBackoffAlgStatus == BackoffAlgorithmRetriesExhausted )
            {
                LogError( ( "Connection to the broker failed, all attempts exhausted." ) );
            }
        } while( ( xConnected != pdPASS ) && ( xBackoffAlgStatus == BackoffAlgorithmSuccess ) );
    }

    /* Set the socket wakeup callback and ensure the read block time. */
    if( xConnected )
    {
        ( void ) FreeRTOS_setsockopt( pxNetworkContext->pParams->tcpSocket,
                                      0, /* Level - Unused. */
                                      FREERTOS_SO_WAKEUP_CALLBACK,
                                      ( void * ) prvMQTTClientSocketWakeupCallback,
                                      sizeof( &( prvMQTTClientSocketWakeupCallback ) ) );

        ( void ) FreeRTOS_setsockopt( pxNetworkContext->pParams->tcpSocket,
                                      0,
                                      FREERTOS_SO_RCVTIMEO,
                                      &xTransportTimeout,
                                      sizeof( TickType_t ) );
    }

    return xConnected;
}

/*-----------------------------------------------------------*/

static BaseType_t prvSocketDisconnect( NetworkContext_t * pxNetworkContext )
{
    BaseType_t xDisconnected = pdFAIL;

    /* Set the wakeup callback to NULL since the socket will disconnect. */
    ( void ) FreeRTOS_setsockopt( pxNetworkContext->pParams->tcpSocket,
                                  0, /* Level - Unused. */
                                  FREERTOS_SO_WAKEUP_CALLBACK,
                                  ( void * ) NULL,
                                  sizeof( void * ) );

        LogInfo( ( "Disconnecting TCP connection.\n" ) );
        PlaintextTransportStatus_t xNetworkStatus = PLAINTEXT_TRANSPORT_CONNECT_FAILURE;
        xNetworkStatus = Plaintext_FreeRTOS_Disconnect( pxNetworkContext );
        xDisconnected = ( xNetworkStatus == PLAINTEXT_TRANSPORT_SUCCESS ) ? pdPASS : pdFAIL;

    return xDisconnected;
}


/*-----------------------------------------------------------*/

static uint32_t prvGetTimeMs( void )
{
    TickType_t xTickCount = 0;
    uint32_t ulTimeMs = 0UL;

    /* Get the current tick count. */
    xTickCount = xTaskGetTickCount();

    /* Convert the ticks to milliseconds. */
    ulTimeMs = ( uint32_t ) xTickCount * MILLISECONDS_PER_TICK;

    /* Reduce ulGlobalEntryTimeMs from obtained time so as to always return the
     * elapsed time in the application. */
    ulTimeMs = ( uint32_t ) ( ulTimeMs - ulGlobalEntryTimeMs );

    return ulTimeMs;
}

/*-----------------------------------------------------------*/

/* USER CODE END Application */

