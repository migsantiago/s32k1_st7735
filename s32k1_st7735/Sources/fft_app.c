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

#include "Cpu.h"
#include "fft_app.h"
#include <math.h>
#include <stdio.h>
#include <string.h>
#include "tft_st7735/TFT_ST7735.h"

//////////////////////////////////////////////////////////////////////
/// Defines
//////////////////////////////////////////////////////////////////////

/* Every 102.4ms an FFT should be calculated */
#define FFT_SAMPLE_MAX                                  (2048U)
#define FFT_POWER_OF_TWO                                (11U)
#define FFT_SAMPLING_PERIOD_S                           (0.00005) /* LPIT set to 50us */
#define FFT_SAMPLING_FREQUENCY_HZ                       (1.0 / FFT_SAMPLING_PERIOD_S)

/* The frequency response is half the size of the time domain data + 1 (Nyquist) */
#define FFT_FREQUENCY_RESP_SIZE                         ((FFT_SAMPLE_MAX / 2) + 1)

#define FFT_PI                                          (3.141592653)
#define FFT_ADC_MAX                                     (4095U)
#define FFT_ADC_REFERENCE                               (5.0)

#define TFT_FONT_1_WIDTH                                (6)
#define TFT_FONT_1_HEIGHT                               (8)

//////////////////////////////////////////////////////////////////////
/// Variables
//////////////////////////////////////////////////////////////////////

/**
 * As this variables are huge, they may collide with the default stack from Eclipse:
 *                 0x00000400                HEAP_SIZE = DEFINED (__heap_size__)?__heap_size__:0x400
 *                 0x00000400                STACK_SIZE = DEFINED (__stack_size__)?__stack_size__:0x400
 *
 * Modify this in the corresponding linker section (*.ld files).
 *
 * In order to get stack usage reports use these arguments in gcc:
 * -fstack-usage
 *
 * Fix for S32K144 stack and bss allocation available here:
 * https://electrolinks.blogspot.com/2020/05/calcular-el-stack-en-s32-design-studio.html
 *
 */

static uint16_t FFT_AudioSamples[FFT_SAMPLE_MAX];
static float FFT_HammingWindow[FFT_SAMPLE_MAX];
static float FFT_Frequency_Bands[FFT_FREQ_BANDS];
static uint8_t FFT_elementsPerBand = 0;

static uint16_t FFT_currentSample = 0;
static uint8_t FFT_bufferReady = 0;

//////////////////////////////////////////////////////////////////////
/// Function prototypes
//////////////////////////////////////////////////////////////////////

/**
 * This computes an in-place complex-to-complex FFT
 * re and im are the real and imaginary arrays of 2^m points.
 * dir =  1 gives forward transform
 * dir = -1 gives reverse transform
 * @param dir
 * @param m
 * @param re
 * @param im
 */
static void FFT(int dir, long m, float* re, float* im);

/**
 * Initialize a Hamming window array
 */
static void FFT_Initialize_Hamming(void);

/**
 * Initialize a small array of bands showing the center frequency
 * for each one
 */
static void FFT_Initialize_Frequency_Bands(void);

//////////////////////////////////////////////////////////////////////
/// Function definitions
//////////////////////////////////////////////////////////////////////

void FFT_Initialize(void)
{
    (void)memset(&FFT_AudioSamples[0], 0, sizeof(FFT_AudioSamples));

    FFT_currentSample = 0;
    FFT_bufferReady = 0;
    FFT_elementsPerBand = 0;

    FFT_Initialize_Hamming();
    FFT_Initialize_Frequency_Bands();
}

void FFT_GetSample(uint16_t sample)
{
    if (0 == FFT_bufferReady)
    {
        FFT_AudioSamples[FFT_currentSample] = sample;

        FFT_currentSample++;

        if (FFT_SAMPLE_MAX == FFT_currentSample)
        {
            FFT_bufferReady = 1;
            FFT_currentSample = 0;

            /* Disable the ADC so that we do not get a new sample out of order */
            INT_SYS_DisableIRQ(ADC0_IRQn);
        }
    }
}

void FFT_SetBufferAvailable(void)
{
    /* Critical section */
    INT_SYS_DisableIRQ(ADC0_IRQn);
    FFT_bufferReady = 0;
    INT_SYS_EnableIRQ(ADC0_IRQn);
}

uint8_t FFT_GetBufferReady(void)
{
    return FFT_bufferReady;
}

void FFT_GetFrequencyResponse(float* freqResponsePerBand)
{
    if (1 == FFT_bufferReady)
    {
        float audioReal[FFT_SAMPLE_MAX];
        float audioImag[FFT_SAMPLE_MAX];
        float freqResp[FFT_FREQUENCY_RESP_SIZE];

        /* Copy samples as fast as possible */
        for(uint32_t i = 0; i < FFT_SAMPLE_MAX; i++)
        {
            audioReal[i] = (float)FFT_AudioSamples[i];
        }

        /* At this point the ADC sampling can be restarted as FFT_AudioSamples
         * is no longer needed. */
        FFT_SetBufferAvailable();
        INT_SYS_EnableIRQ(ADC0_IRQn);

        (void)memset(&audioImag[0], 0, sizeof(audioImag));
        (void)memset(&freqResp[0], 0, sizeof(freqResp));

        /* Convert samples to voltages */
        for(uint32_t i = 0; i < FFT_SAMPLE_MAX; i++)
        {
            audioReal[i] = (audioReal[i] / FFT_ADC_MAX) * FFT_ADC_REFERENCE;

            /* Apply Hamming window */
            audioReal[i] *= FFT_HammingWindow[i];
        }

        /* Calculate FFT */
        FFT(1, FFT_POWER_OF_TWO, &audioReal[0], &audioImag[0]);

        for (uint32_t i = 0; i < FFT_FREQUENCY_RESP_SIZE; ++i)
        {
            freqResp[i] = sqrt((audioReal[i] * audioReal[i]) + (audioImag[i] * audioImag[i]));
            freqResp[i] /= FFT_SAMPLE_MAX;
        }

        for (uint32_t i = 1; i < (FFT_FREQUENCY_RESP_SIZE - 1); ++i)
        {
            freqResp[i] *= 2;
        }

        /* Accumulate various elements per band */
        for (uint32_t currentBand = 0; currentBand < FFT_FREQ_BANDS; currentBand++)
        {
            freqResponsePerBand[currentBand] = 0;

            for (uint32_t currentElement = 0; currentElement < FFT_elementsPerBand; currentElement++)
            {
                freqResponsePerBand[currentBand] +=
                    freqResp[currentElement + (currentBand * FFT_elementsPerBand)];
            }
        }
    }
}

void FFT_PlotFrequencyResponse(float* freqResponsePerBand)
{
    static uint8_t initializedScreen = 0;

    if (0 == initializedScreen)
    {
        uint8_t x = 0;
        uint8_t y = TFT_ST7735_height() - TFT_FONT_1_HEIGHT;
        char str[4] = {0, '.', 0, 0};

        TFT_ST7735_setTextColor(ST7735_YELLOW);

        TFT_ST7735_fillRect(x, 0, TFT_ST7735_width(), 10, ST7735_BLACK);
        TFT_ST7735_drawCentreString("MigSantiago.com", TFT_ST7735_width() / 2, 0, 1);
        TFT_ST7735_fillRect(x, y - 1, TFT_ST7735_width(), 10, ST7735_BLACK);

        for (uint8_t i = 0; i < FFT_FREQ_BANDS; i++)
        {
            str[0] = '0' + (int)(FFT_Frequency_Bands[i] / 1000.0);
            str[2] = '0' + ((int)(FFT_Frequency_Bands[i] / 100.0)) % 10;

            TFT_ST7735_drawChar(x, y, str[0], ST7735_YELLOW, ST7735_BLACK, 1);
            TFT_ST7735_drawChar(x + 5, y, str[1], ST7735_YELLOW, ST7735_BLACK, 1);
            TFT_ST7735_drawChar(x + 10, y, str[2], ST7735_YELLOW, ST7735_BLACK, 1);

            /* Move to the right */
            x += TFT_ST7735_width() / FFT_FREQ_BANDS;
        }

        initializedScreen = 1;
    }

    {
        /* Bar height = 128 - 10 - 10 */
        const int32_t barTotalHeight = 108;
        const float maximumVoltage = 0.5;
        int32_t barHeight;

        /* Workaround, remove DC offset */
        freqResponsePerBand[0] -= 1.983;

        /* Draw bars */
        uint8_t x = 0;
        uint8_t y = 10;
        for (uint8_t i = 0; i < FFT_FREQ_BANDS; i++)
        {
            /* barHeight can exceed the maximumVoltage, so make it int32_t */
            barHeight = (freqResponsePerBand[i] / maximumVoltage) * barTotalHeight;

            if (barHeight > barTotalHeight)
            {
                barHeight = barTotalHeight;
            }
            else if (barHeight < 0)
            {
                barHeight = 0;
            }

            /* Draw Height */
            TFT_ST7735_fillRect(
                x + 6,
                y + (barTotalHeight - barHeight),
                (TFT_ST7735_width() / FFT_FREQ_BANDS) - 15, /* shorter is faster to draw */
                barHeight,
                ST7735_GREEN);

            /* Draw empty space */
            TFT_ST7735_fillRect(
                x + 6,
                y,
                (TFT_ST7735_width() / FFT_FREQ_BANDS) - 15, /* shorter is faster to draw */
                barTotalHeight - barHeight,
                ST7735_WHITE);

            x += TFT_ST7735_width() / FFT_FREQ_BANDS;
        }
    }
}

static void FFT(int dir, long m, float* re, float* im)
{
    long n,i,i1,j,k,i2,l,l1,l2;
    double c1,c2,tx,ty,t1,t2,u1,u2,z;

    /* Calculate the number of points */
    n = 1;
    for (i=0;i<m;i++)
        n *= 2;

    /* Do the bit reversal */
    i2 = n >> 1;
    j = 0;
    for (i=0;i<n-1;i++) {
        if (i < j) {
            tx = re[i];
            ty = im[i];
            re[i] = re[j];
            im[i] = im[j];
            re[j] = tx;
            im[j] = ty;
        }
        k = i2;
        while (k <= j) {
            j -= k;
            k >>= 1;
        }
        j += k;
    }

    /* Compute the FFT */
    c1 = -1.0;
    c2 = 0.0;
    l2 = 1;
    for (l=0;l<m;l++) {
        l1 = l2;
        l2 <<= 1;
        u1 = 1.0;
        u2 = 0.0;
        for (j=0;j<l1;j++) {
            for (i=j;i<n;i+=l2) {
                i1 = i + l1;
                t1 = u1 * re[i1] - u2 * im[i1];
                t2 = u1 * im[i1] + u2 * re[i1];
                re[i1] = re[i] - t1;
                im[i1] = im[i] - t2;
                re[i] += t1;
                im[i] += t2;
            }
            z =  u1 * c1 - u2 * c2;
            u2 = u1 * c2 + u2 * c1;
            u1 = z;
        }
        c2 = sqrt((1.0 - c1) / 2.0);
        if (dir == 1)
            c2 = -c2;
        c1 = sqrt((1.0 + c1) / 2.0);
    }
}

static void FFT_Initialize_Hamming(void)
{
    float v;
    int N = sizeof(FFT_HammingWindow) / sizeof(FFT_HammingWindow[0]);

    for (int n = 0; n < N; ++n)
    {
        v = 0.54 - (0.46 * cos((2.0 * FFT_PI * (float)n) / ((float)N - 1.0)));
        FFT_HammingWindow[n] = v;
    }
}

static void FFT_Initialize_Frequency_Bands(void)
{
    FFT_elementsPerBand = FFT_FREQUENCY_RESP_SIZE / FFT_FREQ_BANDS;

    /* For each band, get its center by dividing the elementsPerBand by 2 */
    for (uint32_t currentBand = 0; currentBand < FFT_FREQ_BANDS; currentBand++)
    {
        FFT_Frequency_Bands[currentBand] =
            (FFT_SAMPLING_FREQUENCY_HZ / FFT_SAMPLE_MAX)
            * (((float)FFT_elementsPerBand / 2) + ((float)currentBand * FFT_elementsPerBand));
    }
}
