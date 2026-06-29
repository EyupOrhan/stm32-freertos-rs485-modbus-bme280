/* USER CODE BEGIN Header */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "cmsis_os.h"
/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "bme280.h"
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

I2C_HandleTypeDef hi2c1;
UART_HandleTypeDef huart1;

/* Definitions for ModbusTask */
osThreadId_t ModbusTaskHandle;
const osThreadAttr_t ModbusTask_attributes = {
  .name = "ModbusTask",
  .stack_size = 512 * 4,
  .priority = (osPriority_t) osPriorityAboveNormal,
};
/* Definitions for SensorTask */
osThreadId_t SensorTaskHandle;
const osThreadAttr_t SensorTask_attributes = {
  .name = "SensorTask",
  .stack_size = 256 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for StatusTask */
osThreadId_t StatusTaskHandle;
const osThreadAttr_t StatusTask_attributes = {
  .name = "StatusTask",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityLow,
};
/* Definitions for registerMutex */
osMutexId_t registerMutexHandle;
const osMutexAttr_t registerMutex_attributes = {
  .name = "registerMutex"
};


/* USER CODE BEGIN PV */

struct SensorData { //sensor data struct
	float temperature;
	float pressure;
	float humidity;
	float pressure_hPa;
}data;
#define REG 11
uint16_t holdingRegister[REG];

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/

void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_I2C1_Init(void);
static void MX_USART1_UART_Init(void);
void StartModbusTask(void *argument);
void StartSensorTask(void *argument);
void StartStatusTask(void *argument);

/* USER CODE BEGIN PFP */
/* USER CODE END PFP */
/* Private user code ---------------------------------------------------------*/

/* USER CODE BEGIN 0 */

//function prototypes
void RS485_R(void){
	HAL_GPIO_WritePin(DE_RE_485_GPIO_Port, DE_RE_485_Pin, GPIO_PIN_RESET);
}
void RS485_T(void){
	HAL_GPIO_WritePin(DE_RE_485_GPIO_Port, DE_RE_485_Pin, GPIO_PIN_SET);
}
void LED_T(void){
	HAL_GPIO_TogglePin(LED_GPIO_Port, LED_Pin);
}

uint16_t crc16(uint8_t *buffer, uint16_t length)
{
	uint16_t crc = 0xFFFF;
			  	  for (int i=0;i<length;i++)
			  	  {
			  		  crc = crc ^ buffer[i];

			  		  for (int j=0;j<8;j++)
			  		  {
			  		  	  if ((crc) & (1))
			  		  	  	  {
			  		  		  crc = crc >> 1;
			  		  		  crc = crc ^ 0xA001;
			  		  	  	  }
			  		  	  else
			  		  	  	  {
			  		  		  crc= crc >> 1;
			  		  	  	  }
			  		  }

			  	  }
			  	  return crc;
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
  MX_I2C1_Init();
  MX_USART1_UART_Init();
  /* USER CODE BEGIN 2 */
  int result = bme280_init(&hi2c1,
                              BME280_OVERSAMPLE_X1,    // humidity
                              BME280_OVERSAMPLE_X1,    // temperature
                              BME280_OVERSAMPLE_X1,    // pressure
                              BME280_NORMAL_MODE,      // operating mode
                              BME280_STANDBY_1000MS,   // standby time
                              BME280_FILTER_OFF);      // filter coefficient

      if (result != 0) {
          // Initialization failed
          Error_Handler();
      }
  /* USER CODE END 2 */

  /* Init scheduler */
  osKernelInitialize();
  /* Create the mutex(es) */
  /* creation of registerMutex */
  registerMutexHandle = osMutexNew(&registerMutex_attributes);

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
  /* creation of ModbusTask */
  ModbusTaskHandle = osThreadNew(StartModbusTask, NULL, &ModbusTask_attributes);

  /* creation of SensorTask */
  SensorTaskHandle = osThreadNew(StartSensorTask, NULL, &SensorTask_attributes);

  /* creation of StatusTask */
  StatusTaskHandle = osThreadNew(StartStatusTask, NULL, &StatusTask_attributes);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  /* USER CODE END RTOS_THREADS */

  /* USER CODE BEGIN RTOS_EVENTS */
  /* add events, ... */
  /* USER CODE END RTOS_EVENTS */

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

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief I2C1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_I2C1_Init(void)
{

  /* USER CODE BEGIN I2C1_Init 0 */

  /* USER CODE END I2C1_Init 0 */

  /* USER CODE BEGIN I2C1_Init 1 */

  /* USER CODE END I2C1_Init 1 */
  hi2c1.Instance = I2C1;
  hi2c1.Init.ClockSpeed = 100000;
  hi2c1.Init.DutyCycle = I2C_DUTYCYCLE_2;
  hi2c1.Init.OwnAddress1 = 0;
  hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c1.Init.OwnAddress2 = 0;
  hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2C1_Init 2 */

  /* USER CODE END I2C1_Init 2 */

}

/**
  * @brief USART1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART1_UART_Init(void)
{

  /* USER CODE BEGIN USART1_Init 0 */

  /* USER CODE END USART1_Init 0 */

  /* USER CODE BEGIN USART1_Init 1 */

  /* USER CODE END USART1_Init 1 */
  huart1.Instance = USART1;
  huart1.Init.BaudRate = 9600;
  huart1.Init.WordLength = UART_WORDLENGTH_8B;
  huart1.Init.StopBits = UART_STOPBITS_1;
  huart1.Init.Parity = UART_PARITY_NONE;
  huart1.Init.Mode = UART_MODE_TX_RX;
  huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart1.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART1_Init 2 */

  /* USER CODE END USART1_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  /* USER CODE BEGIN MX_GPIO_Init_1 */

  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_SET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(DE_RE_485_GPIO_Port, DE_RE_485_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin : LED_Pin */
  GPIO_InitStruct.Pin = LED_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(LED_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : DE_RE_485_Pin */
  GPIO_InitStruct.Pin = DE_RE_485_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(DE_RE_485_GPIO_Port, &GPIO_InitStruct);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/* USER CODE BEGIN Header_StartModbusTask */
/**
  * @brief  Function implementing the ModbusTask thread.
  * @param  argument: Not used
  * @retval None
  */
/* USER CODE END Header_StartModbusTask */


// Communication Task
/*MODBUS HOLDING REGISTER MAP
 * 0  Temperature x100
 * 1  Pressure hPa x10
 * 2  Humidity x100
 * 3  Device Status
 * 4  Error Code
 * 5  Firmware Version
 * 6  Uptime Seconds
 * 7  CRC Error Count
 * 8  Invalid Request Count
 * 9  Modbus Request Count
 * 10 Sensor Read Count
  */
void StartModbusTask(void *argument)
{
	/* USER CODE BEGIN StartModbusTask */
	/* Infinite loop */
	 RS485_R();
	 HAL_StatusTypeDef uart_r; // uart receive status

	 uint8_t rxbuffer[8];           // receive and transmit data arrays
	 uint8_t txbuffer[3+ REG*2 + 2];

	 uint8_t crcholdrx[2]; // crc holders
	 uint8_t crcholdtx[2];

	 uint16_t request_c = 0,crc_c = 0, irequest_c = 0; // error code counts

	 uint16_t start_address; // address 3-> quantity (register amount)
	 uint16_t quantity;


	 uint8_t exception; //error code definitions
	 uint16_t error;

  for(;;)
  {
	  uart_r=HAL_UART_Receive(&huart1,rxbuffer,8,100);

	  if (uart_r==HAL_OK)
		{

		  	  uint16_t crc_rx = crc16(rxbuffer,6);

		  	  crcholdrx[0]= crc_rx & 0xFF;
		  	  crcholdrx[1]= crc_rx >> 8;

		  	  start_address = (rxbuffer[2] << 8 | rxbuffer[3]); // starting address
		  	  quantity = (rxbuffer[4]<< 8 | rxbuffer[5]); // total reg amount
		  	  uint16_t responselength= 3+quantity*2+2;

		if ((rxbuffer[6]!=crcholdrx[0]) || (rxbuffer[7]!=crcholdrx[1]))   //crc checker
		  	  {
		  		  crc_c++;
		  		  osMutexAcquire(registerMutexHandle,osWaitForever);
		  		  holdingRegister[7]=crc_c;
		  		  osMutexRelease(registerMutexHandle);

		  	  }


		else if ( (rxbuffer[0]==1) &&      // correct configurations response generator
				  (rxbuffer[1]==3) &&
				  (quantity > 0) &&
				  (quantity <= REG) &&
				  (start_address < REG) &&
				  (start_address + quantity <= REG) &&
				  (rxbuffer[6]==crcholdrx[0]) &&
				  (rxbuffer[7]==crcholdrx[1])
			    )

		  {

			  txbuffer[0]=rxbuffer[0];
			  txbuffer[1]=rxbuffer[1];
			  txbuffer[2]=quantity*2 ;

			  osMutexAcquire(registerMutexHandle,osWaitForever);
			  request_c++;
			  holdingRegister[9]=request_c;

			  for (int i=0; i < quantity; i++)

			  {
				  uint16_t value = holdingRegister[start_address+i];
				  txbuffer[3+i*2]= value >> 8;
				  txbuffer[3+i*2 + 1] = value & 0xFF;

			  }
			  osMutexRelease(registerMutexHandle);

			  uint16_t crc_tx = crc16(txbuffer, responselength-2);

			  crcholdtx[0]= crc_tx & 0xFF;
			  crcholdtx[1]= crc_tx >> 8;

			  txbuffer[responselength-2]  = crcholdtx[0];
			  txbuffer[responselength-1] = crcholdtx[1];


			  RS485_T();
			  HAL_UART_Transmit(&huart1,txbuffer,responselength,1000); // register response
			  RS485_R();
		  }

	 else {                                                                // error response
			  if (rxbuffer[0]==1){

				  if(rxbuffer[1]!=3)
				  {
					  exception=0x01; // illegal function
					  error=2; // invalid
				  }
				  else if ((start_address >=REG)|| (start_address+ quantity> REG))
				  {
					  exception=0x02; // illegal address
					  error= 3; // invalid
				  }
				  else
				  {
					  exception=0x03; // illegal data
					  error = 4; //invalid
				  }

				  irequest_c++;
				  			  osMutexAcquire(registerMutexHandle,osWaitForever);
				  			  holdingRegister[8]=irequest_c;
				  			  holdingRegister[4]= error ;
				  			  osMutexRelease(registerMutexHandle);

				  			  txbuffer[0]=rxbuffer[0];
				  			  txbuffer[1]=rxbuffer[1]|0x80;
				  			  txbuffer[2]=exception;
				  			  uint16_t errorcrc=crc16(txbuffer,3);

				  			  txbuffer[3]=errorcrc & 0xFF;
				  			  txbuffer[4]=errorcrc >> 8;

				  			  RS485_T();
				  			  HAL_UART_Transmit(&huart1,txbuffer,5,1000); //5 byte response
				  			  RS485_R();
			  }

		  }

	  }
	  osDelay(10);
	  }
  /* USER CODE END StartModbusTask */
  }



/* USER CODE BEGIN Header_StartSensorTask */
/**
* @brief Function implementing the SensorTask thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartSensorTask */
void StartSensorTask(void *argument)     //sensor reading task
{
	/* USER CODE BEGIN StartSensorTask */
	/* Infinite loop */
	uint16_t sensor_c=0;
	for(;;)
	{
	  sensor_c++;
	  data.temperature = bme280_get_temperature();
	  data.pressure = bme280_get_pressure();
	  data.humidity = bme280_get_humidity();
	  data.pressure_hPa = data.pressure/100.0f;

	  osMutexAcquire(registerMutexHandle,osWaitForever);
	  holdingRegister[0]=data.temperature*100;
	  holdingRegister[1]=data.pressure_hPa*10;
	  holdingRegister[2]=data.humidity*100;
	  holdingRegister[10]=sensor_c;
	  osMutexRelease(registerMutexHandle);

	  osDelay(2000);
  }
	/* USER CODE END StartSensorTask */
}

/* USER CODE BEGIN Header_StartStatusTask */
/**
* @brief Function implementing the StatusTask thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartStatusTask */
void StartStatusTask(void *argument)   // current status task
{
	uint16_t uptime=0;
  /* USER CODE BEGIN StartStatusTask */
  /* Infinite loop */
  for(;;)
  {
	  uptime++;
	  osMutexAcquire(registerMutexHandle,osWaitForever);
	  holdingRegister[3]=1; //device status
	  holdingRegister[5]=100; //firmware 1.0.0
	  holdingRegister[6]=uptime; //uptime
	  osMutexRelease(registerMutexHandle);
	  LED_T();
	  osDelay(1000);
  }
  /* USER CODE END StartStatusTask */
}

/**
  * @brief  Period elapsed callback in non blocking mode
  * @note   This function is called  when TIM4 interrupt took place, inside
  * HAL_TIM_IRQHandler(). It makes a direct call to HAL_IncTick() to increment
  * a global variable "uwTick" used as application time base.
  * @param  htim : TIM handle
  * @retval None
  */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  /* USER CODE BEGIN Callback 0 */

  /* USER CODE END Callback 0 */
  if (htim->Instance == TIM4)
  {
    HAL_IncTick();
  }
  /* USER CODE BEGIN Callback 1 */

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
