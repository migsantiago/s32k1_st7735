/***************************************************
  DESCRIPTION

  TFT graphics library for any uC or SoC with open
  implementations on SPI callbacks.

  No specific HW implementations are available.

  This means: no dependency on Arduino, yay!

  New implementation was brought back from C++ to C
  for more compatibility on more environments.

  Code cleanup and reordering was also applied.

  Use at your own risk.

  AUTHOR (modifier)
  migsantiago.com / May 2020

  SOURCES

  This library has been derived from the Adafruit_GFX
  library and the associated driver library. See text
  at the end of this file.

  This was also taken and refactored from Github:
  Bodmer / TFT_ST7735
  https://github.com/Bodmer/TFT_ST7735

 ****************************************************/

//////////////////////////////////////////////////////////////////////
/// Include files
//////////////////////////////////////////////////////////////////////

#include "Cpu.h"
#include "test.h"
#include "./tft_st7735/TFT_ST7735.h"

#ifdef ENABLE_TEST_CODE
#if (ENABLE_TEST_CODE == 1)

/* Flag used to store if an ADC IRQ was executed */
volatile uint8_t adcConvDone = 0;
/* Variable to store value from ADC conversion */
volatile uint16_t adcRawValue = 0;

//////////////////////////////////////////////////////////////////////
/// Function definitions
//////////////////////////////////////////////////////////////////////

void testBlinkingRGB(void)
{
    TFT_ST7735_Configure_SPI();

    LPTMR_DRV_StartCounter(INST_LPTMR1);
    while(1)
    {
        if(LPTMR_DRV_GetCompareFlag(INST_LPTMR1))
        {
            LPTMR_DRV_ClearCompareFlag(INST_LPTMR1);
            PINS_DRV_TogglePins(PTD, (1 << 15) | (1 << 16));
        }
    }
}

void testDummySPI(void)
{
    volatile status_t stat = STATUS_ERROR;
    uint32_t remainingBytes = 0;
    const uint8_t dummy_data[] =
    {
        0xFF, 0x55, 0x00, 0xFF, 0x55, 0x00, 0xFF, 0x55
    };

    while (1)
    {
        stat = LPSPI_DRV_MasterGetTransferStatus(LPSPICOM1, &remainingBytes);

        if (0 == remainingBytes)
        {
            stat = LPSPI_DRV_MasterTransfer(LPSPICOM1, &dummy_data[0], (uint8_t*)0, sizeof(dummy_data));
        }
    }
}

void testDummySPIIncreasing(void)
{
    volatile status_t stat = STATUS_ERROR;
    uint32_t remainingBytes = 0;
    uint8_t dummy_data[256];

    for(remainingBytes = 0; remainingBytes < sizeof(dummy_data); remainingBytes++)
    {
        dummy_data[remainingBytes] = remainingBytes;
    }

    while (1)
    {
        stat = LPSPI_DRV_MasterGetTransferStatus(LPSPICOM1, &remainingBytes);

        if (0 == remainingBytes)
        {
            stat = LPSPI_DRV_MasterTransfer(LPSPICOM1, &dummy_data[0], (uint8_t*)0, sizeof(dummy_data));
        }
    }
}

void testDrawRainbow(void)
{
    static uint8_t red = 31;
    static uint8_t green = 0;
    static uint8_t blue = 0;
    static uint8_t state = 0;
    static unsigned int colour;
    static uint8_t initialized = 0;

    if(0 == initialized)
    {
        colour = red << 11;

        TFT_ST7735_init();
        TFT_ST7735_setRotation(1);
        TFT_ST7735_fillScreen(ST7735_BLACK);

        initialized = 1;
    }

    for (int i = 0; i<160; i++) {
      TFT_ST7735_drawFastVLine(i, 0, TFT_ST7735_height(), colour);
      switch (state) {
      case 0:
        green +=2;
        if (green == 64) {
          green=63;
          state = 1;
        }
        break;
      case 1:
        red--;
        if (red == 255) {
          red = 0;
          state = 2;
        }
        break;
      case 2:
        blue ++;
        if (blue == 32) {
          blue=31;
          state = 3;
        }
        break;
      case 3:
        green -=2;
        if (green ==255) {
          green=0;
          state = 4;
        }
        break;
      case 4:
        red ++;
        if (red == 32) {
          red = 31;
          state = 5;
        }
        break;
      case 5:
        blue --;
        if (blue == 255) {
          blue = 0;
          state = 0;
        }
        break;
      }
      colour = red<<11 | green<<5 | blue;
    }

//    // The standard ADAFruit font still works as before
//    TFT_ST7735_setTextColor_bgcolor(ST7735_BLACK, ST7735_BLACK); // Note these fonts do not plot the background colour
//    TFT_ST7735_drawString("Original ADAfruit font!", 12, 5, 2);
//
//    // The new larger fonts do not use the .setCursor call, coords are embedded
//    TFT_ST7735_setTextColor_bgcolor(ST7735_BLACK, ST7735_BLACK); // Do not plot the background colour
//    // Overlay the black text on top of the rainbow plot (the advantage of not drawing the backgorund colour!)
//    TFT_ST7735_drawCentreString("Font size 2",80,14,2); // Draw text centre at position 80, 12 using font 2
//    //TFT_ST7735_drawCentreString("Font size 2",81,12,2); // Draw text centre at position 80, 12 using font 2
//    TFT_ST7735_drawCentreString("Font size 4",80,30,4); // Draw text centre at position 80, 24 using font 4
//    TFT_ST7735_drawCentreString("12.34",80,54,6); // Draw text centre at position 80, 24 using font 6
//    TFT_ST7735_drawCentreString("12.34 is in font size 6",80,92,2); // Draw text centre at position 80, 90 using font 2
//    // Note the x position is the top of the font!
//
//    // draw a floating point number
//    float pi = 3.14159; // Value to print
//    int precision = 3;  // Number of digits after decimal point
//    int xpos = 50;      // x position
//    int ypos = 110;     // y position
//    int font = 2;       // font number only 2,4,6,7 valid. Font 6 only contains characters [space] 0 1 2 3 4 5 6 7 8 9 0 : a p m
//    xpos += TFT_ST7735_drawFloat(pi,precision,xpos,ypos,font); // Draw rounded number and return new xpos delta for next print position
//    TFT_ST7735_drawString(" is pi",xpos,ypos,font); // Continue printing from new x position
}

void testDelay(void)
{
    TFT_ST7735_Configure_SPI();

    while(1)
    {
        TFT_ST7735_Delay(5);
        PINS_DRV_TogglePins(PTD, (1 << 15));
    }
}

#define ADC_MAX (4095)
void testADC(void)
{
    static uint8_t initialized = 0;

    if(0 == initialized)
    {
        TFT_ST7735_init();
        TFT_ST7735_setRotation(1);
        TFT_ST7735_fillScreen(ST7735_WHITE);
        TFT_ST7735_setTextColor_bgcolor(ST7735_YELLOW, ST7735_BLACK);
        initialized = 1;
    }

    if (adcConvDone)
    {
        uint16_t copyadcRawValue;
        float voltage = 0;

        INT_SYS_DisableIRQ(ADC0_IRQn);
        copyadcRawValue = adcRawValue;
        INT_SYS_EnableIRQ(ADC0_IRQn);

        voltage = ((float)copyadcRawValue / ADC_MAX) * 5.0F;

        /* Draw voltage, 3 decimals, at 0,0 with font 2 */
        TFT_ST7735_drawFloat(voltage, 3, 0, 0, 2);

        adcConvDone = 0;
    }
}

#endif /* #ifdef (ENABLE_TEST_CODE) */
#endif /* #if (ENABLE_TEST_CODE == 1) */
