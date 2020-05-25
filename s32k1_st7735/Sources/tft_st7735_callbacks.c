/***************************************************
  DESCRIPTION

  TFT graphics library for any uC or SoC with open
  implementations on SPI callouts.

  No specific HW implementations are available.

  The larger fonts are Run Length Encoded to reduce
  their FLASH footprint.

  New implementation was brought back to C for more
  compatibility on more environments.

  Code cleanup and reordering was also applied.

  Use at your own risk.

  AUTHOR (modifier)
  migsantiago.com

  SOURCES

  This library has been derived from the Adafruit_GFX
  library and the associated driver library. See text
  at the end of this file.

  This was also taken from Github:
  Bodmer / TFT_ST7735
  https://github.com/Bodmer/TFT_ST7735

 ****************************************************/

#include "Cpu.h"
#include "lpspiCom1.h"
#include "lpTmr1.h"
#include "pin_mux.h"
#include "tft_st7735/TFT_ST7735.h"

/**
 * Pinout for the display:
 * 160x128 TFT display ST7735 w/uSD card breakout
 *
 * Display - S32K144
 *
 * LITE    - PTD15
 * MOSI    - PTB4
 * SCK     - PTB2
 * TFT-CS  - PTB3
 * DC      - PTB5
 * RESET   - 10kOhm - PTB0
 * VDD     - 5V
 * GND     - GND
 */


#define TFT_ST7735_PIN_DATA_COMM                    (5)
#define TFT_ST7735_PIN_RESET                        (0)
#define TFT_ST7735_PIN_BACKLIGHT                    (15)
#define TFT_ST7735_PIN_CHIP_SELECT                  (3)

/**
 * Any specific initialization stuff that the uC environment may need
 * for SPI and GPIOs (DC, RESET, CS, BACKLIGHT) must be implemented here
 * SPI clock = 15MHz maximum
 */
void TFT_ST7735_Configure_SPI(void)
{
    /* LPSPI0 is initialized outside */

    /* Enable light */
    PINS_DRV_SetPins(PTD, (1 << TFT_ST7735_PIN_BACKLIGHT));

    /* Reset screen */
    PINS_DRV_ClearPins(PTB, (1 << TFT_ST7735_PIN_RESET));

    /* Disable CS */
    PINS_DRV_SetPins(PTB, (1 << TFT_ST7735_PIN_CHIP_SELECT));
}

/**
 * A blocking delay must be implemented in this call
 * Use HW or SW but this call must be blocking
 * @param ms - delay in milli seconds
 */
void TFT_ST7735_Delay(unsigned int ms)
{
    LPTMR_DRV_StopCounter(INST_LPTMR1);
    LPTMR_DRV_ClearCompareFlag(INST_LPTMR1);
    LPTMR_DRV_StartCounter(INST_LPTMR1);

    while(ms != 0)
    {
        while(!LPTMR_DRV_GetCompareFlag(INST_LPTMR1));
        LPTMR_DRV_ClearCompareFlag(INST_LPTMR1);
        ms--;
    }
}

/**
 * Set the physical voltage on the chip select line
 * @param status - refer to TFT_ST7735_CS_T
 */
void TFT_ST7735_Set_Chip_Select(TFT_ST7735_CS_T status)
{
    if(CHIP_SELECT_HIGH == status)
    {
        PINS_DRV_SetPins(PTB, 1 << TFT_ST7735_PIN_CHIP_SELECT);
    }
    else
    {
        PINS_DRV_ClearPins(PTB, 1 << TFT_ST7735_PIN_CHIP_SELECT);
    }
}

/**
 * Set the mode of the request that will be sent
 * @param request - refer to TFT_ST7735_Data_Command_T
 */
void TFT_ST7735_Set_Data_Command(TFT_ST7735_Data_Command_T request)
{
    if(REQUEST_COMMAND == request)
    {
        PINS_DRV_ClearPins(PTB, 1 << TFT_ST7735_PIN_DATA_COMM);
    }
    else
    {
        PINS_DRV_SetPins(PTB, 1 << TFT_ST7735_PIN_DATA_COMM);
    }
}

/**
 * Set the reset pin status
 * @param status -  refer to TFT_ST7735_Data_Command_T
 */
void TFT_ST7735_Set_Reset(TFT_ST7735_Reset_T status)
{
    if(RESET_PIN_LOW == status)
    {
        PINS_DRV_ClearPins(PTB, 1 << TFT_ST7735_PIN_RESET);
    }
    else
    {
        PINS_DRV_SetPins(PTB, 1 << TFT_ST7735_PIN_RESET);
    }
}

/**
 * Write size bytes to the display
 * @attention Make sure to send data waiting for bytes to be sent
 * one after the other (wait until previous data has been sent).
 * CS, or DC must not be affected in this call.
 * @param data - pointer to an array of data to send
 * @param size - how many bytes to send
 */
void TFT_ST7735_Write_SPI(unsigned char *data, uint32_t size)
{
    uint32_t remainingBytes = 0;

    /* Wait until any previous activity is over */
    do
    {
        LPSPI_DRV_MasterGetTransferStatus(LPSPICOM1, &remainingBytes);
    }
    while (remainingBytes != 0);

    LPSPI_DRV_MasterTransfer(LPSPICOM1, (const uint8_t *)data, (uint8_t*)0, size);
}
