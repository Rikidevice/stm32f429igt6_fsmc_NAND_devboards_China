/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"

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
NAND_HandleTypeDef hnand1;
SDRAM_HandleTypeDef hsdram1;

/* USER CODE BEGIN PV */
NAND_IDTypeDef g_nand_id;

volatile HAL_StatusTypeDef g_erase_status = HAL_ERROR;
volatile HAL_StatusTypeDef g_read_status  = HAL_ERROR;
volatile HAL_StatusTypeDef g_write_status = HAL_ERROR;

NAND_AddressTypeDef g_erase_addr;

uint8_t g_write_buf[2048];
uint8_t g_read_buf[2048];

/* јдреса областей FMC NAND Bank3 */
const uint32_t g_data_area = 0x80000000u;
const uint32_t g_cmd_area  = 0x80000000u | 0x00010000u;  /* 0x80010000 */
const uint32_t g_addr_area = 0x80000000u | 0x00020000u;  /* 0x80020000 */
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_FMC_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* ќжидание готовности NAND по линии R/B# (PB13). */
static HAL_StatusTypeDef NAND_WaitReady(uint32_t timeout_ms)
{
  uint32_t start = HAL_GetTick();
  while (HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_13) == GPIO_PIN_RESET)
  {
    if ((HAL_GetTick() - start) > timeout_ms)
    {
      return HAL_TIMEOUT;
    }
  }
  return HAL_OK;
}

/* „тение байта статуса NAND (команда 0x70) */
static uint8_t NAND_ReadStatusByte(void)
{
  *(volatile uint8_t *)g_cmd_area = 0x70;
  return *(volatile uint8_t *)g_data_area;
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
  MX_FMC_Init();
  /* USER CODE BEGIN 2 */

  /* ================= SDRAM INITIALIZATION SEQUENCE ================= */
  FMC_SDRAM_CommandTypeDef Command;
  uint32_t tmpmrd = 0;

  Command.CommandMode            = FMC_SDRAM_CMD_CLK_ENABLE;
  Command.CommandTarget          = FMC_SDRAM_CMD_TARGET_BANK1_2;
  Command.AutoRefreshNumber      = 1;
  Command.ModeRegisterDefinition = 0;
  (void)HAL_SDRAM_SendCommand(&hsdram1, &Command, 0xFFFF);

  HAL_Delay(1);

  Command.CommandMode       = FMC_SDRAM_CMD_PALL;
  Command.CommandTarget     = FMC_SDRAM_CMD_TARGET_BANK2;
  Command.AutoRefreshNumber = 1;
  (void)HAL_SDRAM_SendCommand(&hsdram1, &Command, 0xFFFF);

  Command.CommandMode       = FMC_SDRAM_CMD_AUTOREFRESH_MODE;
  Command.CommandTarget     = FMC_SDRAM_CMD_TARGET_BANK2;
  Command.AutoRefreshNumber = 8;
  (void)HAL_SDRAM_SendCommand(&hsdram1, &Command, 0xFFFF);

  tmpmrd = (uint32_t)(0u << 0) | (uint32_t)(0u << 3) |
           (uint32_t)(3u << 4) | (uint32_t)(1u << 9);

  Command.CommandMode            = FMC_SDRAM_CMD_LOAD_MODE;
  Command.CommandTarget          = FMC_SDRAM_CMD_TARGET_BANK2;
  Command.AutoRefreshNumber      = 1;
  Command.ModeRegisterDefinition = tmpmrd;
  (void)HAL_SDRAM_SendCommand(&hsdram1, &Command, 0xFFFF);

  (void)HAL_SDRAM_ProgramRefreshRate(&hsdram1, 1385);
  HAL_Delay(10);

  /* ================= SDRAM TEST ================= */
  {
    volatile uint16_t *sdram_ptr = (volatile uint16_t *)0xD0000000u;
    *sdram_ptr = 0xAA55u;
    volatile uint16_t sdram_rd = *sdram_ptr;
    (void)sdram_rd;
  }

  /* ================= NAND: инициализаци€ адреса ================= */
  g_erase_addr.Plane = 0;
  g_erase_addr.Block = 0;
  g_erase_addr.Page  = 0;

  for (int i = 0; i < 2048; i++)
  {
    g_write_buf[i] = (uint8_t)(i & 0xFF);
  }

  /* ================= NAND TEST ================= */
  HAL_NAND_Reset(&hnand1);
  HAL_Delay(10);

  HAL_StatusTypeDef nand_status = HAL_NAND_Read_ID(&hnand1, &g_nand_id);

  if (nand_status == HAL_OK)
  {
    volatile uint32_t maker  = g_nand_id.Maker_Id;
    volatile uint32_t device = g_nand_id.Device_Id;
    volatile uint32_t third  = g_nand_id.Third_Id;
    volatile uint32_t fourth = g_nand_id.Fourth_Id;
    (void)maker; (void)device; (void)third; (void)fourth;
  }
  else
  {
    volatile uint32_t err = 1;
    (void)err;
  }

  /* ================= NAND: стирание блока 0 ================= */

  *(volatile uint8_t *)g_cmd_area = 0x60;

  *(volatile uint8_t *)g_addr_area = (uint8_t)((g_erase_addr.Block >> 8) & 0xFF);
  *(volatile uint8_t *)g_addr_area = (uint8_t)( g_erase_addr.Block       & 0xFF);

  *(volatile uint8_t *)g_cmd_area = 0xD0;

  /* tWB: NAND должен успеть ЂдЄрнутьї R/B# вниз */
  for (volatile int d = 0; d < 1000; d++);

  HAL_StatusTypeDef erase_wait = NAND_WaitReady(100);
  if (erase_wait != HAL_OK)
  {
    g_erase_status = HAL_TIMEOUT;
  }
  else
  {
    uint8_t st = NAND_ReadStatusByte();
    g_erase_status = ((st & 0x01u) == 0u) ? HAL_OK : HAL_ERROR;
  }

  /* ================= NAND: запись страницы 0 блока 0 ================= */

  *(volatile uint8_t *)g_cmd_area = 0x80;

  *(volatile uint8_t *)g_addr_area = 0x00;
  *(volatile uint8_t *)g_addr_area = 0x00;
  *(volatile uint8_t *)g_addr_area = 0x00;
  *(volatile uint8_t *)g_addr_area = 0x00;

  for (int i = 0; i < 2048; i++)
  {
    *(volatile uint8_t *)g_data_area = g_write_buf[i];
  }

  *(volatile uint8_t *)g_cmd_area = 0x10;

  for (volatile int d = 0; d < 1000; d++);

  HAL_StatusTypeDef prog_wait = NAND_WaitReady(50);
  if (prog_wait != HAL_OK)
  {
    g_write_status = HAL_TIMEOUT;
  }
  else
  {
    uint8_t st = NAND_ReadStatusByte();
    g_write_status = ((st & 0x01u) == 0u) ? HAL_OK : HAL_ERROR;
  }

  /* ================= NAND: чтение страницы 0 блока 0 ================= */

  *(volatile uint8_t *)g_cmd_area = 0x00;

  *(volatile uint8_t *)g_addr_area = 0x00;
  *(volatile uint8_t *)g_addr_area = 0x00;
  *(volatile uint8_t *)g_addr_area = 0x00;
  *(volatile uint8_t *)g_addr_area = 0x00;

  *(volatile uint8_t *)g_cmd_area = 0x30;

  /* tWB: критически важно, иначе NAND не успеет перевести R/B# в низкий уровень */
  for (volatile int d = 0; d < 5000; d++);

  /* tR: по даташиту W29N01HV максимум 25 мкс, 1 мс с большим запасом */
  HAL_Delay(1);

  /* „тение данных: никакой команды 0x70 здесь быть не должно */
  for (int i = 0; i < 2048; i++)
  {
    g_read_buf[i] = *(volatile uint8_t *)g_data_area;
  }

  /* —равнение: только по содержимому, без статусного байта */
  uint32_t mismatch = 0;
  for (int i = 0; i < 2048; i++)
  {
    if (g_read_buf[i] != g_write_buf[i])
    {
      mismatch++;
    }
  }
  g_read_status = (mismatch == 0) ? HAL_OK : HAL_ERROR;

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
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 8;
  RCC_OscInitStruct.PLL.PLLN = 180;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 4;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Activate the Over-Drive mode
  */
  if (HAL_PWREx_EnableOverDrive() != HAL_OK)
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

/* FMC initialization function */
static void MX_FMC_Init(void)
{

  /* USER CODE BEGIN FMC_Init 0 */

  /* USER CODE END FMC_Init 0 */

  FMC_NAND_PCC_TimingTypeDef ComSpaceTiming = {0};
  FMC_NAND_PCC_TimingTypeDef AttSpaceTiming = {0};
  FMC_SDRAM_TimingTypeDef SdramTiming = {0};

  /* USER CODE BEGIN FMC_Init 1 */

  /* USER CODE END FMC_Init 1 */

  /** Perform the NAND1 memory initialization sequence
  */
  hnand1.Instance = FMC_NAND_DEVICE;
  /* hnand1.Init */
  hnand1.Init.NandBank = FMC_NAND_BANK3;
  hnand1.Init.Waitfeature = FMC_NAND_PCC_WAIT_FEATURE_DISABLE;
  hnand1.Init.MemoryDataWidth = FMC_NAND_PCC_MEM_BUS_WIDTH_8;
  hnand1.Init.EccComputation = FMC_NAND_ECC_DISABLE;
  hnand1.Init.ECCPageSize = FMC_NAND_ECC_PAGE_SIZE_256BYTE;
  hnand1.Init.TCLRSetupTime = 0;
  hnand1.Init.TARSetupTime = 0;
  /* hnand1.Config */
  hnand1.Config.PageSize = 2048;
  hnand1.Config.SpareAreaSize = 64;
  hnand1.Config.BlockSize = 64;
  hnand1.Config.BlockNbr = 1024;
  hnand1.Config.PlaneNbr = 1;
  hnand1.Config.PlaneSize = 1024;
  hnand1.Config.ExtraCommandEnable = DISABLE;
  /* ComSpaceTiming */
  ComSpaceTiming.SetupTime = 1;
  ComSpaceTiming.WaitSetupTime = 4;
  ComSpaceTiming.HoldSetupTime = 3;
  ComSpaceTiming.HiZSetupTime = 1;
  /* AttSpaceTiming */
  AttSpaceTiming.SetupTime = 1;
  AttSpaceTiming.WaitSetupTime = 7;
  AttSpaceTiming.HoldSetupTime = 3;
  AttSpaceTiming.HiZSetupTime = 1;

  if (HAL_NAND_Init(&hnand1, &ComSpaceTiming, &AttSpaceTiming) != HAL_OK)
  {
    Error_Handler( );
  }

  /** Perform the SDRAM1 memory initialization sequence
  */
  hsdram1.Instance = FMC_SDRAM_DEVICE;
  /* hsdram1.Init */
  hsdram1.Init.SDBank = FMC_SDRAM_BANK2;
  hsdram1.Init.ColumnBitsNumber = FMC_SDRAM_COLUMN_BITS_NUM_8;
  hsdram1.Init.RowBitsNumber = FMC_SDRAM_ROW_BITS_NUM_12;
  hsdram1.Init.MemoryDataWidth = FMC_SDRAM_MEM_BUS_WIDTH_16;
  hsdram1.Init.InternalBankNumber = FMC_SDRAM_INTERN_BANKS_NUM_4;
  hsdram1.Init.CASLatency = FMC_SDRAM_CAS_LATENCY_3;
  hsdram1.Init.WriteProtection = FMC_SDRAM_WRITE_PROTECTION_DISABLE;
  hsdram1.Init.SDClockPeriod = FMC_SDRAM_CLOCK_PERIOD_2;
  hsdram1.Init.ReadBurst = FMC_SDRAM_RBURST_ENABLE;
  hsdram1.Init.ReadPipeDelay = FMC_SDRAM_RPIPE_DELAY_0;
  /* SdramTiming */
  SdramTiming.LoadToActiveDelay = 2;
  SdramTiming.ExitSelfRefreshDelay = 7;
  SdramTiming.SelfRefreshTime = 4;
  SdramTiming.RowCycleDelay = 7;
  SdramTiming.WriteRecoveryTime = 6;
  SdramTiming.RPDelay = 2;
  SdramTiming.RCDDelay = 2;

  if (HAL_SDRAM_Init(&hsdram1, &SdramTiming) != HAL_OK)
  {
    Error_Handler( );
  }

  /* USER CODE BEGIN FMC_Init 2 */

  /* USER CODE END FMC_Init 2 */
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
  __HAL_RCC_GPIOF_CLK_ENABLE();
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOG_CLK_ENABLE();
  __HAL_RCC_GPIOE_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();

  /*Configure GPIO pin : PB13 */
  GPIO_InitStruct.Pin = GPIO_PIN_13;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

/* USER CODE BEGIN MX_GPIO_Init_2 */
/* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
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
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
