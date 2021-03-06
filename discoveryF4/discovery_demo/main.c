/* Reference to stm32 discovery demo example.*/

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "usbd_hid_core.h"
#include "usbd_usr.h"
#include "usbd_desc.h"

//Library config for this project!!!!!!!!!!!
#include "stm32f4xx_conf.h"
#include "stm32f4xx_adc.h"

/** @addtogroup STM32F4-Discovery_Demo
  * @{
  */

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/

#define TESTRESULT_ADDRESS         0x080FFFFC
#define ALLTEST_PASS               0x00000000
#define ALLTEST_FAIL               0x55555555
#define BUFFERSIZE 128

uint16_t ADCConvertedValues[BUFFERSIZE];

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
int adc_convert();
void ADC_Configure(void);
void GPIO_Configure_Out(void);
void GPIO_Configure_In(void);
void RCC_Configure(void);



inline int conv2temp(uint16_t value)
{
    return ( ( ( ( value * 2960 ) / 4096 ) - 760 ) / ( 25 / 10 ) ) + 25;
}

//static uint16_t testing=10;

/**
  * @brief  Main program.
  * @param  None
  * @retval None
  */
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
        RCC_Configure();
        GPIO_Configure_Out();
        GPIO_Configure_In();
        ADC_Configure();

        uint16_t USING_PIN[]= {GPIO_Pin_3, GPIO_Pin_4, GPIO_Pin_5, GPIO_Pin_6, GPIO_Pin_7, GPIO_Pin_8, GPIO_Pin_9, GPIO_Pin_10, GPIO_Pin_11, GPIO_Pin_12, GPIO_Pin_13, GPIO_Pin_14};
        while(1)
        {
            GPIO_ResetBits(GPIOG, GPIO_Pin_3|GPIO_Pin_4|GPIO_Pin_5|GPIO_Pin_6|GPIO_Pin_7|GPIO_Pin_8|GPIO_Pin_9|GPIO_Pin_10|GPIO_Pin_11|GPIO_Pin_12|GPIO_Pin_13|GPIO_Pin_14);

            ConvertedValue = adc_convert();   //Read the ADC converted value
            uint16_t sum = 0;

            register int i;
            for(i=0; i<12; ++i)
                sum|=(ConvertedValue & (1 << i)?USING_PIN[i]:0);

            GPIO_SetBits(GPIOG, sum);
            Delay(100);
        }

        /* Try to test ADC.*/
    }
    else {}
}

void RCC_Configure(void)
{
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOG, ENABLE);

    //Clock configuration
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1, ENABLE);//ADC1 is connected to APB2 peripheral bus
    RCC_AHB1PeriphClockCmd(RCC_AHB1ENR_GPIOCEN, ENABLE);//Clock for the ADC port!! Do not forget about this one ;)
}


void ADC_Configure()
{
    ADC_InitTypeDef ADC_init_structure; //Structure for adc confguration

    //ADC structure configuration
    ADC_DeInit();//reset all parameters to their default values
    ADC_init_structure.ADC_DataAlign = ADC_DataAlign_Right;//converted data will be shifted to right
    ADC_init_structure.ADC_Resolution = ADC_Resolution_10b;//Input voltage is converted into a 10-bit number whose maximum value is 4095
    ADC_init_structure.ADC_ContinuousConvMode = ENABLE; //the conversion is continuous, the input data is converted more than once
    ADC_init_structure.ADC_ExternalTrigConv = ADC_ExternalTrigConv_T1_CC1;//use timer 1 capture/compare channel 1 for external trigger
    ADC_init_structure.ADC_ExternalTrigConvEdge = ADC_ExternalTrigConvEdge_None;//no trigger for conversion
    ADC_init_structure.ADC_NbrOfConversion = 1;//Number of used ADC channels
    ADC_init_structure.ADC_ScanConvMode = DISABLE;//No scan (only one channel)
    ADC_Init(ADC1, &ADC_init_structure);
    ADC_TempSensorVrefintCmd(ENABLE);

    // use channel 10 from ADC1, with sample time 144 cycles
    //ADC_RegularChannelConfig(ADC1, ADC_Channel_10, 1, ADC_SampleTime_144Cycles);
    ADC_RegularChannelConfig(ADC1, ADC_Channel_TempSensor, 1, ADC_SampleTime_144Cycles);
    ADC_Cmd(ADC1, ENABLE);
}

int adc_convert()
{
    ADC_SoftwareStartConv(ADC1);//Start the conversion
    while(!ADC_GetFlagStatus(ADC1, ADC_FLAG_EOC));//Processing the conversion
    return ADC_GetConversionValue(ADC1); //Return the converted data
}

void GPIO_Configure_Out(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;

    GPIO_PinAFConfig(GPIOG, GPIO_PinSource3|GPIO_PinSource4|GPIO_PinSource5|GPIO_PinSource6|GPIO_PinSource7|GPIO_PinSource8|GPIO_PinSource9|GPIO_PinSource10|GPIO_PinSource11|GPIO_PinSource12|GPIO_PinSource13|GPIO_PinSource14, GPIO_AF_TIM3);

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3|GPIO_Pin_4|GPIO_Pin_5|GPIO_Pin_6|GPIO_Pin_7|GPIO_Pin_8|GPIO_Pin_9|GPIO_Pin_10|GPIO_Pin_11|GPIO_Pin_12|GPIO_Pin_13|GPIO_Pin_14;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;            // Alt Function - Push Pull
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;//100
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
    GPIO_Init(GPIOG, &GPIO_InitStructure );
}

void GPIO_Configure_In(void)
{
    GPIO_InitTypeDef GPIO_initStructre; //Structure for analog input pin

    //Analog input pin configuration
    GPIO_initStructre.GPIO_Pin = GPIO_Pin_0;//The channel 10 is connected to PC0
    GPIO_initStructre.GPIO_Mode = GPIO_Mode_AN; //The PC0 pin is configured in analog mode
    GPIO_initStructre.GPIO_PuPd = GPIO_PuPd_NOPULL; //We don't need any pull up or pull down
    GPIO_Init(GPIOC, &GPIO_initStructre);
}

/**
  * @brief  Initializes the USB for the demonstration application.
  * @param  None
  * @retval None
  */
static uint32_t Demo_USBConfig(void)
{
    USBD_Init(&USB_OTG_dev,
              USB_OTG_FS_CORE_ID,
              &USR_desc,
              &USBD_HID_cb,
              &USR_cb);

    return 0;
}

/**
  * @brief  Inserts a delay time.
  * @param  nTime: specifies the delay time length, in 10 ms.
  * @retval None
  */
void Delay(__IO uint32_t nTime)
{
    TimingDelay = nTime;

    while(TimingDelay != 0);
}

/**
  * @brief  Decrements the TimingDelay variable.
  * @param  None
  * @retval None
  */
void TimingDelay_Decrement(void)
{
    if (TimingDelay != 0x00)
    {
        TimingDelay--;
    }
}

/**
  * @brief  This function handles the test program fail.
  * @param  None
  * @retval None
  */
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

/**
  * @}
  */


/******************* (C) COPYRIGHT 2011 STMicroelectronics *****END OF FILE****/
