
/* User includes (#include below this line is not maintained by Processor Expert) */

/***************************************************
  DESCRIPTION

  TFT ST7735 example to draw an FFT from audio on an S32K144.

  Use at your own risk.

  AUTHOR (modifier)
  migsantiago.com / May 2020

 ****************************************************/

//////////////////////////////////////////////////////////////////////
/// Include files
//////////////////////////////////////////////////////////////////////

/* Including necessary module. Cpu.h contains other modules needed for compiling.*/
#include "adConv1.h"
#include "clockMan1.h"
#include "Cpu.h"
#include "fft_app.h"
#include "lpit1.h"
#include "pwrMan1.h"
#include "test.h"
#include "./tft_st7735/TFT_ST7735.h"

//////////////////////////////////////////////////////////////////////
/// Defined macros and constants
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
/// Global variables
//////////////////////////////////////////////////////////////////////

volatile int exit_code = 0;

/* Variable to store value from ADC conversion */
volatile uint16_t adcRawValue;

//////////////////////////////////////////////////////////////////////
/// Function prototypes
//////////////////////////////////////////////////////////////////////

void ADC_IRQHandler(void);
void InitializeHardware(void);
int main(void);
void GetFFTPlot(void);

//////////////////////////////////////////////////////////////////////
/// Function definitions
//////////////////////////////////////////////////////////////////////

/* @brief: ADC Interrupt Service Routine.
 *        Read the conversion result, store it
 *        into a variable and set a specified flag.
 */
void ADC_IRQHandler(void)
{
    /* Get channel result from ADC channel */
    ADC_DRV_GetChanResult(INST_ADCONV1, 0U, (uint16_t *)&adcRawValue);

    FFT_GetSample(adcRawValue);
}

/**
 * Initialize all the peripherals
 */
void InitializeHardware(void)
{
    /* ADC */
    IRQn_Type adcIRQ;

    /* Clock initialization */
    CLOCK_SYS_Init(g_clockManConfigsArr, CLOCK_MANAGER_CONFIG_CNT, g_clockManCallbacksArr, CLOCK_MANAGER_CALLBACK_CNT);
    CLOCK_SYS_UpdateConfiguration(0U, CLOCK_MANAGER_POLICY_FORCIBLE);

    /* Power mode initialization */
    POWER_SYS_Init(&powerConfigsArr, POWER_MANAGER_CONFIG_CNT, &powerStaticCallbacksConfigsArr, POWER_MANAGER_CALLBACK_CNT);
    /* The first configuration enables HSRUN mode at 112MHz */
    POWER_SYS_SetMode(0, POWER_MANAGER_POLICY_FORCIBLE);

    /* Initialize pins */
    PINS_DRV_Init(NUM_OF_CONFIGURED_PINS, g_pin_mux_InitConfigArr);

    /* Initialize LPTMR to set its flag every 1.024ms (512us + 1 tick) */
    LPTMR_DRV_Init(INST_LPTMR1, &lpTmr1_config0, false);
    LPTMR_DRV_SetConfig(INST_LPTMR1, &lpTmr1_config0);
    LPTMR_DRV_ClearCompareFlag(INST_LPTMR1);

    /* Initialize LPSPI0 at ~16MHz for the ST7735 */
    LPSPI_DRV_MasterInit(LPSPICOM1, &lpspiCom1State, &lpspiCom1_MasterConfig0);

    /* Configure and calibrate the ADC converter */
    DEV_ASSERT(adConv1_ChnConfig0.channel == ADC_CHN);
    ADC_DRV_ConfigConverter(INST_ADCONV1, &adConv1_ConvConfig0);
    ADC_DRV_AutoCalibration(INST_ADCONV1);
    ADC_DRV_ConfigChan(INST_ADCONV1, 0UL, &adConv1_ChnConfig0);
    adcIRQ = ADC0_IRQn;

    INT_SYS_InstallHandler(adcIRQ, &ADC_IRQHandler, (isr_t*) 0);

    /* Use LPIT as timer for the trigger that starts an ADC measurement
     * It has been set to trigger every 50us for 20kSps on the ADC */
    LPIT_DRV_Init(INST_LPIT1, &lpit1_InitConfig);
    LPIT_DRV_InitChannel(INST_LPIT1, 0, &lpit1_ChnConfig0);

    /* Configure TRGMUX to route the trigger from LPIT0 to ADC */
    TRGMUX_DRV_Init(INST_TRGMUX1, &trgmux1_InitConfig0);

    /* Enable ADC interrupt */
    INT_SYS_EnableIRQ(adcIRQ);
}

void GetFFTPlot(void)
{
    if (FFT_GetBufferReady())
    {
        float parsedFreqResponseBands[FFT_FREQ_BANDS];

        PINS_DRV_SetPins(PTD, 1 << 0);

        {
            {
                /* Get FFT_FREQ_BANDS bands showing the accumulated elements per frequency */
                FFT_GetFrequencyResponse(&parsedFreqResponseBands[0]);
            }

            {
                /* Plot the results on screen! */
                FFT_PlotFrequencyResponse(&parsedFreqResponseBands[0]);
            }
        }

        PINS_DRV_ClearPins(PTD, 1 << 0);
    }
}

/*! 
  \brief The main function for the project.
  \details The startup initialization sequence is the following:
 * - startup asm routine
 * - main()
*/
int main(void)
{
  /* Write your local variable definition here */

  /*** Processor Expert internal initialization. DON'T REMOVE THIS CODE!!! ***/
  #ifdef PEX_RTOS_INIT
    PEX_RTOS_INIT();                   /* Initialization of the selected RTOS. Macro is defined by the RTOS component. */
  #endif
  /*** End of Processor Expert internal initialization.                    ***/

  /* Write your code here */

    /* Init HW */
    InitializeHardware();

    /* Init display */
    TFT_ST7735_init();
    TFT_ST7735_setRotation(1);
    TFT_ST7735_fillScreen(ST7735_WHITE);
    TFT_ST7735_setTextColor_bgcolor(ST7735_YELLOW, ST7735_BLACK);

    /* Init FFT */
    FFT_Initialize();

    /* Initialize the ADC sampling */
    LPIT_DRV_StartTimerChannels(INST_LPIT1, (1 << 0));

    while (1)
    {
        GetFFTPlot();
    }

#ifdef ENABLE_TEST_CODE
#if (ENABLE_TEST_CODE == 1)

    while(1)
    {
        TFT_ST7735_Delay(200);
        testADC();
    }

#endif /* #ifdef (ENABLE_TEST_CODE) */
#endif /* #if (ENABLE_TEST_CODE == 1) */

  /*** Don't write any code pass this line, or it will be deleted during code generation. ***/
  /*** RTOS startup code. Macro PEX_RTOS_START is defined by the RTOS component. DON'T MODIFY THIS CODE!!! ***/
  #ifdef PEX_RTOS_START
    PEX_RTOS_START();                  /* Startup of the selected RTOS. Macro is defined by the RTOS component. */
  #endif
  /*** End of RTOS startup code.  ***/
  /*** Processor Expert end of main routine. DON'T MODIFY THIS CODE!!! ***/
  for(;;) {
    if(exit_code != 0) {
      break;
    }
  }
  return exit_code;
  /*** Processor Expert end of main routine. DON'T WRITE CODE BELOW!!! ***/
} /*** End of main routine. DO NOT MODIFY THIS TEXT!!! ***/

/* END main */
/*!
** @}
*/
/*
** ###################################################################
**
**     This file was created by Processor Expert 10.1 [05.21]
**     for the NXP S32K series of microcontrollers.
**
** ###################################################################
*/
