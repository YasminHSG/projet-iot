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
#include "i2c.h"
#include "spi.h"
#include "tim.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "cayenne_lpp.h"
//#include "debug.h"
//#include "hal.h"
#include "lmic.h"
#include "lorabase.h"
#include "oslmic.h"
#include <bme680/bme68x_necessary_functions.h>
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
/* Private variables ---------------------------------------------------------*/
// application router ID (LSBF) < ------- IMPORTANT
static const u1_t APPEUI[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };
// unique device ID (LSBF) < ------- IMPORTANT
static const u1_t DEVEUI[8] = { 0x4C, 0x47, 0x07, 0xD0, 0x7E, 0xD5, 0xB3, 0x70};
// device-specific AES key (derived from device EUI (MSBF))
static const u1_t DEVKEY[16] = { 0x18, 0xD1, 0x68, 0xF1, 0xDC, 0xEC, 0x2C, 0xF4, 0xEA, 0x5D, 0xD4, 0x19, 0x9A, 0xAA, 0xC1, 0x52 };

volatile int entree_adc_15 = 0;
volatile float temp = 0;

cayenne_lpp_t cayenne_packet;

// BME680 data
struct bme68x_data data;
volatile int err_code = 0;


//APPEUI,DEVEUI must be copied from thethingsnetwork application datas in LSB format
//-----------------------------------------------------------------------------------------
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
// provide application router ID (8 bytes, LSBF)
void os_getArtEui (u1_t* buf) {
	memcpy(buf, APPEUI, 8);
}
// provide device ID (8 bytes, LSBF)
void os_getDevEui (u1_t* buf) {
	memcpy(buf, DEVEUI, 8);
}
// provide device key (16 bytes)
void os_getDevKey (u1_t* buf) {
	memcpy(buf, DEVKEY, 16);
}
void initsensor(){
// Here you init your sensors
	HAL_GPIO_WritePin(Alim_temp_GPIO_Port, Alim_temp_Pin, GPIO_PIN_SET); //alimente le capteur de temperature
	HAL_ADCEx_Calibration_Start(&hadc1, ADC_SINGLE_ENDED);
	/*HAL_ADCEx_Calibration_Start(&hadc1, ADC_SINGLE_ENDED);
	HAL_GPIO_WritePin(GPIOA, GPIO_PIN_7, GPIO_PIN_SET); // on met la broche qui alimente le capteur à 1
	HAL_ADCEx_Calibration_Start(&hadc1, ADC_SINGLE_ENDED);
	HAL_Delay(10); // délai pour que le capteur se stabilise */
}

void initfunc (osjob_t* j) {
	// intialize sensor hardware
	initsensor();
	// reset MAC state
	LMIC_reset();

	//LMIC_setClockError(MAX_CLOCK_ERROR * 1 / 100);
	// start joining
	LMIC_startJoining();
	// init done - onEvent() callback will be invoked...
}
u2_t readsensor(){
	u2_t value = 0xDF; /// read from evrything ...make your own sensor
	// entier non signé de 16 bits
	// On met ici ce qu'on transmet
	return value;
}

u2_t readsensor_temp(){
		return (188686 - 147 * entree_adc_15);
}


static osjob_t reportjob;

// report sensor value every minute
static void reportfunc (osjob_t* j) {

	/* // read sensor
		u2_t val = readsensor_temp();
		debug_val("val = ", val);
		// prepare and schedule data for transmission
		LMIC.frame[0] = 0x0; // Channel 0
		LMIC.frame[1] = 0x67; // Temperature sensor

		val *= 10; // Normalisation pour LPP Cayenne
		LMIC.frame[2] = (val >> 8) & 0xFF;
		LMIC.frame[3] = val & 0xFF;

	//	LMIC.frame[0] = val << 8;
	//	LMIC.frame[1] = val;
		LMIC_setTxData2(1, LMIC.frame, 4, 0); // (port 1, 2 bytes, unconfirmed)
		// reschedule job in 60 seconds
		os_setTimedCallback(j, os_getTime() + sec2osticks(15), reportfunc);
	  */

	// BME680

	if (bme68x_single_measure(&data) == 0) {
		// Measurement is successful, so continue with IAQ
	        HAL_Delay(200);

	        data.iaq_score = bme68x_iaq(); // Calculate IAQ
	        if(data.temperature > 35 ){
				HAL_GPIO_WritePin(LED_TEMP_GPIO_Port, LED_TEMP_Pin, GPIO_PIN_SET);
			}
			else {HAL_GPIO_WritePin(LED_TEMP_GPIO_Port, LED_TEMP_Pin, GPIO_PIN_RESET);}
			if(data.humidity > 35){
				HAL_GPIO_WritePin(LED_HUM_GPIO_Port, LED_HUM_Pin, GPIO_PIN_SET);
			}

			else {HAL_GPIO_WritePin(LED_HUM_GPIO_Port, LED_HUM_Pin, GPIO_PIN_RESET);}
			if(data.iaq_score > 50){
				HAL_GPIO_WritePin(LED_IAQ_GPIO_Port, LED_IAQ_Pin, GPIO_PIN_SET);
			}
			else{ HAL_GPIO_WritePin(LED_IAQ_GPIO_Port, LED_IAQ_Pin, GPIO_PIN_RESET);}
	    }


		cayenne_lpp_reset(&cayenne_packet);

		// On met sur les différents canaux du buffer les données que l'on souhaite.

		cayenne_lpp_add_temperature(&cayenne_packet, 0, data.temperature);
		cayenne_lpp_add_relative_humidity(&cayenne_packet, 1, data.humidity);
		cayenne_lpp_add_barometric_pressure(&cayenne_packet, 2, data.pressure/100);
		cayenne_lpp_add_analog_output(&cayenne_packet, 3, data.iaq_score);
		cayenne_lpp_add_analog_output(&cayenne_packet, 4, (float)(data.gas_index));
		cayenne_lpp_add_analog_output(&cayenne_packet, 5, data.gas_resistance);

		//LMIC_setTxData2(1, cayenne_packet.buffer, sizeof(cayenne_packet.buffer), 0);

		//  On envoioe les data du buffer à TTN grâce à la librairie cayenne_lpp.
		LMIC_setTxData2(1, cayenne_packet.buffer, cayenne_packet.cursor, 0);



		// On fait un envoi toutes les 30 secondes
		os_setTimedCallback(j, os_getTime()+sec2osticks(30), reportfunc);

	}



//////////////////////////////////////////////////
// LMIC EVENT CALLBACK
//////////////////////////////////////////////////
void onEvent (ev_t ev) {
	switch(ev) {
	// network joined, session established
		case EV_JOINING:
			break;
		case EV_JOINED:
			// kick-off periodic sensor job
			HAL_GPIO_WritePin(JOIN_GPIO_Port, JOIN_Pin, GPIO_PIN_SET);
			reportfunc(&reportjob);
			break;
		case EV_JOIN_FAILED:
			HAL_GPIO_WritePin(JOIN_GPIO_Port, JOIN_Pin, GPIO_PIN_RESET);
			break;
		case EV_SCAN_TIMEOUT:
			break;
		case EV_BEACON_FOUND:
			break;
		case EV_BEACON_MISSED:
			break;
		case EV_BEACON_TRACKED:
			break;
		case EV_RFU1:
			break;
		case EV_REJOIN_FAILED:
			break;
		case EV_TXCOMPLETE:
			break;
		case EV_LOST_TSYNC:
			break;
		case EV_RESET:
			break;
		case EV_RXCOMPLETE:
			// data received in ping slot
			break;
		case EV_LINK_DEAD:
			break;
		case EV_LINK_ALIVE:
			break;
		default:
			break;
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
  MX_TIM7_Init();
  MX_SPI3_Init();
  MX_ADC1_Init();
  MX_I2C1_Init();
  MX_TIM6_Init();
  /* USER CODE BEGIN 2 */

  /* BME680 API forced mode test */
  bme68x_start(&data, &hi2c1);

  HAL_TIM_Base_Start_IT(&htim6); //demarrage du timer 6 en interruption toutes les secondes pour la mesure temperature
  HAL_TIM_Base_Start_IT(&htim7);   // <----------- change to your setup
  __HAL_SPI_ENABLE(&hspi3);        // <----------- change to your setup
  osjob_t initjob;
  // initialize runtime env
  os_init();
  // setup initial job
  os_setCallback(&initjob, initfunc); // Lance la radio
  // execute scheduled jobs and events
  //os_runloop();
  // (not reached)
  //return 0;


  // setup initial job dans main.c
  //os_setCallback(&hellojob, hellofunc);
 // execute scheduled jobs and events
  os_runloop();
 // (not reached)
  return 0;

  /* USER CODE END 2 */

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
  if (HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 1;
  RCC_OscInitStruct.PLL.PLLN = 10;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV7;
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

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_4) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

/*

void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef *hadc)
{
	entree_adc_15 = HAL_ADC_GetValue(&hadc1);
}
*/
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
