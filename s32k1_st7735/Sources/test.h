/***************************************************
  DESCRIPTION

  TFT ST7735 example to draw an FFT from audio on an S32K144.

  Use at your own risk.

  AUTHOR (modifier)
  migsantiago.com / May 2020

 ****************************************************/

#ifndef TEST_H
#define TEST_H

//////////////////////////////////////////////////////////////////////
/// Exported defines and macros
//////////////////////////////////////////////////////////////////////

/**
 * This define is only meant for testing hardware peripherals
 */
#define ENABLE_TEST_CODE                                (0)

#ifdef ENABLE_TEST_CODE
#if (ENABLE_TEST_CODE == 1)

//////////////////////////////////////////////////////////////////////
/// Exported function prototypes
//////////////////////////////////////////////////////////////////////

/* Flag used to store if an ADC IRQ was executed */
extern volatile uint8_t adcConvDone;
/* Variable to store value from ADC conversion */
extern volatile uint16_t adcRawValue;

void testBlinkingRGB(void);
void testDummySPI(void);
void testDummySPIIncreasing(void);
void testDrawRainbow(void);
void testDelay(void);
void testADC(void);

#endif /* #ifdef (ENABLE_TEST_CODE) */
#endif /* #if (ENABLE_TEST_CODE == 1) */

#endif /* TEST_H */
