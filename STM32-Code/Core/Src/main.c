#include "main.h"

I2C_HandleTypeDef hi2c1;
TIM_HandleTypeDef htim1;

UART_HandleTypeDef huart1;
UART_HandleTypeDef huart5;
UART_HandleTypeDef huart3;

void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_UART5_Init(void);
static void MX_USART1_UART_Init(void);
static void MX_USART3_UART_Init(void);
static void MX_TIM1_Init(void);
static void MX_I2C1_Init(void);
void UState_Machine(void);
void CState_Machine(void);

volatile unsigned char SM1_Clk;

unsigned char request[8] = {0x01, 0x04, 0x00, 0x00, 0x00, 0x0A, 0x70, 0x0D};
volatile unsigned char rx_data[25];
volatile unsigned char rx_done = 0;
unsigned short voltage;
unsigned long current;
unsigned long watts;
unsigned long watthours;
unsigned char pzem = 1;
unsigned char pzem_active = 1;
unsigned char timeout;

enum {UInit, UWait, UTransmit, URecieve} UARTState;

enum {CInit, CWait, CCompute} CalcState;

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    if (htim->Instance == TIM1)
    {
        SM1_Clk = 1;
    }
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART1 || huart->Instance == UART5 || huart->Instance == USART3)
    {
        rx_done = 1;
    }
}

int main(void)
{
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();
    MX_UART5_Init();
    MX_USART1_UART_Init();
    MX_USART3_UART_Init();
    MX_TIM1_Init();
    MX_I2C1_Init();

    HAL_TIM_Base_Start_IT(&htim1);

    UARTState = UInit;
    CalcState = CInit;

    while(1)
    {
        if(SM1_Clk){
            SM1_Clk = 0;
            UState_Machine();
        }
        CState_Machine();
    }
}

void UState_Machine(void)
{
    switch (UARTState)
    {
        case UInit:
        	timeout = 0;
            rx_done = 0;
            UARTState = UWait;
        break;
        case UWait:
            UARTState = UTransmit;
        break;
        case UTransmit:
        	if (pzem == 1)
        	{
                if (HAL_UART_Transmit(&huart1, request, 8, 50) == HAL_OK)
                {
                	pzem_active = 1;
                    HAL_UART_Receive_IT(&huart1, (unsigned char*)rx_data, 25);
                    UARTState = URecieve;
                }
                else
                {
                    UARTState = UWait;
                }
            }
        	else if (pzem == 2)
        	{
                if (HAL_UART_Transmit(&huart5, request, 8, 50) == HAL_OK)
                {
                	pzem_active = 2;
                    HAL_UART_Receive_IT(&huart5, (unsigned char*)rx_data, 25);
                    UARTState = URecieve;
                }
                else
                {
                    UARTState = UWait;
                }
        	}
        	else if (pzem == 3)
        	{
                if (HAL_UART_Transmit(&huart3, request, 8, 50) == HAL_OK)
                {
                	pzem_active = 3;
                    HAL_UART_Receive_IT(&huart3, (unsigned char*)rx_data, 25);
                    UARTState = URecieve;
                }
                else
                {
                    UARTState = UWait;
                }
        	}
        break;
        case URecieve:
            if (rx_done)
            {
            	rx_done = 0;
            	pzem = pzem + 1;
                UARTState = UWait;
            }
            else
            {
            	timeout = timeout + 1;
            	if (timeout >= 2)
            	{
            		timeout = 0;
            		UARTState = UWait;
            		pzem = pzem + 1;
            	}
            }
        	if (pzem > 3)
        	{
        		pzem = 1;
        	}
        break;
    }
}

void CState_Machine(void)
{
    unsigned long raw_voltage;
    unsigned long raw_current;
    unsigned long raw_watts;
    unsigned long raw_watthours;
    switch (CalcState){
        case CInit:

        CalcState = CWait;
        break;
        case CWait:
            if (rx_done)
            {
                CalcState = CCompute;
            }
        break;
        case CCompute:
        	if (rx_data[0] == 0x01 && rx_data[1] == 0x04 && rx_data[2] == 0x14)
        	{
        		raw_voltage = ((rx_data[3] << 8) | rx_data[4]);
        		raw_current = ((rx_data[7] << 8 | rx_data[8]) << 16) | ((rx_data[5] << 8) | rx_data[6]);
        		raw_watts = ((rx_data[11] << 8 | rx_data[12]) << 16) | ((rx_data[9] << 8) | rx_data[10]);
        		raw_watthours = ((rx_data[15] << 8 | rx_data[16]) << 16) | ((rx_data[13] << 8) | rx_data[14]);

        		if (pzem_active == 1)
        		{
        			voltage = raw_voltage | 0x00000000;
        			current = raw_current | 0x00000000;
        			watts = raw_watts | 0x00000000;
        			watthours = raw_watthours | 0x00000000;
        		}
        		else if (pzem_active == 2)
        		{
        			voltage = raw_voltage | 0x40000000;
        			current = raw_current | 0x40000000;
        			watts = raw_watts | 0x40000000;
        			watthours = raw_watthours | 0x40000000;
        		}
        		else if (pzem_active == 3)
        		{
        			voltage = raw_voltage | 0x80000000;
        			current = raw_current | 0x80000000;
        			watts = raw_watts | 0x80000000;
        			watthours = raw_watthours | 0x80000000;
        		}
        	}
        	CalcState = CWait;
        break;
    }
}



//defult Configuration for uart, ports, and timers
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE3);

  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 16;
  RCC_OscInitStruct.PLL.PLLN = 336;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV4;
  RCC_OscInitStruct.PLL.PLLQ = 2;
  RCC_OscInitStruct.PLL.PLLR = 2;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}

static void MX_I2C1_Init(void)
{
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
}

static void MX_TIM1_Init(void)
{

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};

  htim1.Instance = TIM1;
  htim1.Init.Prescaler = 8399;
  htim1.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim1.Init.Period = 9999;
  htim1.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim1.Init.RepetitionCounter = 0;
  htim1.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim1) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim1, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim1, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }

}

static void MX_USART1_UART_Init(void)
{

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

}

static void MX_UART5_Init(void)
{
  huart5.Instance = UART5;
  huart5.Init.BaudRate = 9600;
  huart5.Init.WordLength = UART_WORDLENGTH_8B;
  huart5.Init.StopBits = UART_STOPBITS_1;
  huart5.Init.Parity = UART_PARITY_NONE;
  huart5.Init.Mode = UART_MODE_TX_RX;
  huart5.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart5.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart5) != HAL_OK)
  {
    Error_Handler();
  }
}
static void MX_USART3_UART_Init(void)
{
  huart3.Instance = USART3;
  huart3.Init.BaudRate = 9600;
  huart3.Init.WordLength = UART_WORDLENGTH_8B;
  huart3.Init.StopBits = UART_STOPBITS_1;
  huart3.Init.Parity = UART_PARITY_NONE;
  huart3.Init.Mode = UART_MODE_TX_RX;
  huart3.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart3.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart3) != HAL_OK)
  {
    Error_Handler();
  }

}

static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  HAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin, GPIO_PIN_RESET);

  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_10|GPIO_PIN_11, GPIO_PIN_SET);

  GPIO_InitStruct.Pin = B1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(B1_GPIO_Port, &GPIO_InitStruct);

  GPIO_InitStruct.Pin = USART_TX_Pin|USART_RX_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  GPIO_InitStruct.Alternate = GPIO_AF7_USART2;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  GPIO_InitStruct.Pin = LD2_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(LD2_GPIO_Port, &GPIO_InitStruct);

  GPIO_InitStruct.Pin = GPIO_PIN_10|GPIO_PIN_11;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

}
void Error_Handler(void)
{
  __disable_irq();
  while (1)
  {
  }
}
#ifdef USE_FULL_ASSERT
void assert_failed(uint8_t *file, uint32_t line)
{
}
#endif
