#include "stm32f4xx.h"
#include "stm32f4xx_gpio.h"
#include "stm32f4xx_syscfg.h"
#include "main.h"
#include "usbd_hid_core.h"
#include "usbd_usr.h"
#include "usbd_desc.h"

//Library config for this project!!!!!!!!!!!
#include "stm32f4xx_conf.h"
#include "stm32f4xx_adc.h"

#define TESTRESULT_ADDRESS         0x080FFFFC
#define ALLTEST_PASS               0x00000000
#define ALLTEST_FAIL               0x55555555
#define BUFFERSIZE 128

/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
#ifdef USB_OTG_HS_INTERNAL_DMA_ENABLED
#if defined ( __ICCARM__ ) /*!< IAR Compiler */
#pragma data_alignment = 4
#endif
#endif /* USB_OTG_HS_INTERNAL_DMA_ENABLED */
__ALIGN_BEGIN USB_OTG_CORE_HANDLE  USB_OTG_dev __ALIGN_END;

uint16_t PrescalerValue = 0;

__IO uint32_t TimingDelay;
__IO uint8_t DemoEnterCondition = 0x00;
__IO uint8_t UserButtonPressed = 0x00;
LIS302DL_InitTypeDef  LIS302DL_InitStruct;
LIS302DL_FilterConfigTypeDef LIS302DL_FilterStruct;
__IO int8_t X_Offset, Y_Offset, Z_Offset  = 0x00;
uint8_t Buffer[6];
int ConvertedValue = 0; //Converted value readed from ADC

/* Private function prototypes -----------------------------------------------*/
static uint32_t Demo_USBConfig(void);
//static void TIM4_Config(void);

/* Private functions ---------------------------------------------------------*/
void GPIO_PIN_INIT(void);
void Interrupts_Configuration(void);


int main(void)
{
    RCC_ClocksTypeDef RCC_Clocks;

    /* Initialize LEDs and User_Button on STM32F4-Discovery --------------------*/
    STM_EVAL_PBInit(BUTTON_USER, BUTTON_MODE_EXTI);

    STM_EVAL_LEDInit(LED4);

    /* SysTick end of count event each 10ms */
    RCC_GetClocksFreq(&RCC_Clocks);
    SysTick_Config(RCC_Clocks.HCLK_Frequency / 100);

    /* Turn on LEDs available on STM32F4-Discovery ---------------------------*/
    STM_EVAL_LEDOn(LED4);

    if (TimingDelay == 0x00)
    {
        /* Turn off LEDs available on STM32F4-Discovery ------------------------*/
        STM_EVAL_LEDOff(LED4);

        /* Write PASS code at last word in the flash memory */
        FLASH_ProgramWord(TESTRESULT_ADDRESS, ALLTEST_PASS);

        /* Try to test ADC.*/
        SystemInit();
        RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOG, ENABLE);
        GPIO_PIN_INIT();
        Interrupts_Configuration();

        while(1)
        {
            //GPIO_ToggleBits(GPIOG, GPIO_Pin_13);
            //GPIO_ToggleBits(GPIOG, GPIO_Pin_14);
            // GPIO read IDR and assign to ODR
            //GPIOG->ODR = GPIOA->IDR << 13;
            //Delay(100);
        }

        /* Try to test ADC.*/
    }
    else {}
}

void GPIO_PIN_INIT(void)
{
    // GPIO Configuration
    // LED3 (Green): GPIO_Pin_13, LED4 (Red): GPIO_Pin_14
    GPIO_InitTypeDef GPIO_InitStructure;
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_13;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
    GPIO_Init(GPIOG, &GPIO_InitStructure);

    GPIO_InitTypeDef GPIO_InitStructure_I;
    GPIO_InitStructure_I.GPIO_Pin = GPIO_Pin_9;
    GPIO_InitStructure_I.GPIO_Mode = GPIO_Mode_IN;
    GPIO_InitStructure_I.GPIO_PuPd = GPIO_PuPd_NOPULL;      // Floating
    GPIO_Init(GPIOG, &GPIO_InitStructure_I);
}


static uint32_t Demo_USBConfig(void)
{
    USBD_Init(&USB_OTG_dev,
              USB_OTG_FS_CORE_ID,
              &USR_desc,
              &USBD_HID_cb,
              &USR_cb);

    return 0;
}

void Delay(__IO uint32_t nTime)
{
    TimingDelay = nTime;

    while(TimingDelay != 0);
}

void TimingDelay_Decrement(void)
{
    if (TimingDelay != 0x00)
    {
        TimingDelay--;
    }
}

void Interrupts_Configuration(void)
{
    EXTI_InitTypeDef EXTI_InitStructure;
    NVIC_InitTypeDef NVIC_InitStructure;

    /* Connect EXTI Line 0 to the button GPIO Pin */
    SYSCFG_EXTILineConfig(EXTI_PortSourceGPIOA, GPIO_PinSource0);

    /* Configure the EXTI line to generate an interrupt when the button is
     * pressed. The button pin is high when pressed, so it needs to trigger
     * when rising from low to high. */
    EXTI_InitStructure.EXTI_Line = EXTI_Line1;
    EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
    EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Rising;
    EXTI_InitStructure.EXTI_LineCmd = ENABLE;
    EXTI_Init(&EXTI_InitStructure);

    /* Enable and set Button EXTI Interrupt to the lowest priority */
    NVIC_InitStructure.NVIC_IRQChannel = EXTI1_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0x0F;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0x0F;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);
}

void EXTI1_IRQHandler(void)
{
    if(EXTI_GetFlagStatus(EXTI_Line1) != RESET)
    {
        // GPIO read IDR and assign to ODR
        GPIOG->ODR = GPIOA->IDR << 13;
        EXTI_ClearITPendingBit(EXTI_Line1);
    }
}

void Fail_Handler(void)
{
    /* Erase last sector */
    FLASH_EraseSector(FLASH_Sector_11, VoltageRange_3);
    /* Write FAIL code at last word in the flash memory */
    FLASH_ProgramWord(TESTRESULT_ADDRESS, ALLTEST_FAIL);

    while(1)
    {
        /* Toggle Red LED */
        STM_EVAL_LEDToggle(LED5);
        Delay(5);
    }
}

/**
  * @brief  MEMS accelerometre management of the timeout situation.
  * @param  None.
  * @retval None.
  */
uint32_t LIS302DL_TIMEOUT_UserCallback(void)
{
    /* MEMS Accelerometer Timeout error occured during Test program execution */
    if (DemoEnterCondition == 0x00)
    {
        /* Timeout error occured for SPI TXE/RXNE flags waiting loops.*/
        Fail_Handler();
    }
    /* MEMS Accelerometer Timeout error occured during Demo execution */
    else
    {
        while (1)
        {
        }
    }
    return 0;
}

#ifdef  USE_FULL_ASSERT

/**
  * @brief  Reports the name of the source file and the source line number
  *   where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t* file, uint32_t line)
{
    /* User can add his own implementation to report the file name and line number,
       ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */

    /* Infinite loop */
    while (1)
    {
    }
}
#endif
