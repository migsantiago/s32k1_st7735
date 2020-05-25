/***************************************************
  DESCRIPTION

  TFT ST7735 example to draw an FFT from audio on an S32K144.

  Use at your own risk.

  AUTHOR (modifier)
  migsantiago.com / May 2020

 ****************************************************/

#ifndef FFT_APP_H
#define FFT_APP_H

//////////////////////////////////////////////////////////////////////
/// Includes
//////////////////////////////////////////////////////////////////////

#include <stdint.h>

//////////////////////////////////////////////////////////////////////
/// Exported defines
//////////////////////////////////////////////////////////////////////

/* How many bands will be shown on screen */
#define FFT_FREQ_BANDS                                  (8U)

//////////////////////////////////////////////////////////////////////
/// Exported variables
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
/// Exported functions
//////////////////////////////////////////////////////////////////////

/**
 * Initialize variables for the FFT
 */
void FFT_Initialize(void);

/**
 * Get a sample every FFT_SAMPLING_PERIOD_US us
 * This is meant to be called from an interrupt
 * @param sample - current sample (12 bits sample with DC offset)
 */
void FFT_GetSample(uint16_t sample);

/**
 * Notify that the buffer is now available for writing new audio samples
 */
void FFT_SetBufferAvailable(void);

/**
 * Get a flag reporting if all samples are ready to be processed
 * @return 0  if not ready, 1 if ready
 */
uint8_t FFT_GetBufferReady(void);

/**
 * Get the calculated frequency responses in 10 bands
 * @param freqResponsePerBand - a pointer to an array of FFT_FREQ_BANDS float variables
 */
void FFT_GetFrequencyResponse(float* freqResponsePerBand);

/**
 * Plot on TFT ST7735 the frequency response
 * @param freqResponsePerBand - a pointer to an array of FFT_FREQ_BANDS float variables
 */
void FFT_PlotFrequencyResponse(float* freqResponsePerBand);

#endif /* FFT_APP_H */
