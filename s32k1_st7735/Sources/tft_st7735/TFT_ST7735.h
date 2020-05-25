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

#ifndef TFT_ST7735_H
#define TFT_ST7735_H

#define INITR_GREENTAB  (0x0)
#define INITR_REDTAB    (0x1)
#define INITR_BLACKTAB  (0x2)
#define INITR_GREENTAB2 (0x3) // Use if you get random pixels on two edges of green tab display
#define INITB  (0xB)

// Include header file that defines the fonts loaded and the pins to be used
#include "TFT_ST7735_cfg.h"

// Only load the fonts defined in User_Setup.h (to save space)
// Set flag so RLE rendering code is optionally compiled
#ifdef LOAD_GLCD
  #include "fonts/glcdfont.h"
#endif

#ifdef LOAD_FONT2
  #include "fonts/Font16.h"
#endif

#ifdef LOAD_FONT4
  #include "fonts/Font32rle.h"
  #define LOAD_RLE
#endif

#ifdef LOAD_FONT6
  #include "fonts/Font64rle.h"
  #ifndef LOAD_RLE
    #define LOAD_RLE
  #endif
#endif

#ifdef LOAD_FONT7
  #include "fonts/Font7srle.h"
  #ifndef LOAD_RLE
    #define LOAD_RLE
  #endif
#endif

#ifdef LOAD_FONT8
  #include "fonts/Font72rle.h"
  #ifndef LOAD_RLE
    #define LOAD_RLE
  #endif
#endif

//These enumerate the text plotting alignment (reference datum point)
#define TL_DATUM (0) // Top left (default)
#define TC_DATUM (1) // Top centre
#define TR_DATUM (2) // Top right
#define ML_DATUM (3) // Middle left
#define CL_DATUM (3) // Centre left, same as above
#define MC_DATUM (4) // Middle centre
#define CC_DATUM (4) // Centre centre, same as above
#define MR_DATUM (5) // Middle right
#define CR_DATUM (5) // Centre right, same as above
#define BL_DATUM (6) // Bottom left
#define BC_DATUM (7) // Bottom centre
#define BR_DATUM (8) // Bottom right


// Change the width and height if required (defined in portrait mode)
// or use the constructor to over-ride defaults
#define ST7735_TFTWIDTH  (128)
#define ST7735_TFTHEIGHT (160)

#define ST7735_INIT_DELAY (0x80)

// These are the ST7735 control registers
// some flags for initR() :(

#define ST7735_TFTWIDTH  (128)
#define ST7735_TFTHEIGHT (160)

#define ST7735_NOP     (0x00)
#define ST7735_SWRESET (0x01)
#define ST7735_RDDID   (0x04)
#define ST7735_RDDST   (0x09)

#define ST7735_SLPIN   (0x10)
#define ST7735_SLPOUT  (0x11)
#define ST7735_PTLON   (0x12)
#define ST7735_NORON   (0x13)

#define ST7735_INVOFF  (0x20)
#define ST7735_INVON   (0x21)
#define ST7735_DISPOFF (0x28)
#define ST7735_DISPON  (0x29)
#define ST7735_CASET   (0x2A)
#define ST7735_RASET   (0x2B)
#define ST7735_RAMWR   (0x2C)
#define ST7735_RAMRD   (0x2E)

#define ST7735_PTLAR   (0x30)
#define ST7735_COLMOD  (0x3A)
#define ST7735_MADCTL  (0x36)

#define ST7735_FRMCTR1 (0xB1)
#define ST7735_FRMCTR2 (0xB2)
#define ST7735_FRMCTR3 (0xB3)
#define ST7735_INVCTR  (0xB4)
#define ST7735_DISSET5 (0xB6)

#define ST7735_PWCTR1  (0xC0)
#define ST7735_PWCTR2  (0xC1)
#define ST7735_PWCTR3  (0xC2)
#define ST7735_PWCTR4  (0xC3)
#define ST7735_PWCTR5  (0xC4)
#define ST7735_VMCTR1  (0xC5)

#define ST7735_RDID1   (0xDA)
#define ST7735_RDID2   (0xDB)
#define ST7735_RDID3   (0xDC)
#define ST7735_RDID4   (0xDD)

#define ST7735_PWCTR6  (0xFC)

#define ST7735_GMCTRP1 (0xE0)
#define ST7735_GMCTRN1 (0xE1)

#define MADCTL_MY  (0x80)
#define MADCTL_MX  (0x40)
#define MADCTL_MV  (0x20)
#define MADCTL_ML  (0x10)
#define MADCTL_RGB (0x00)
#define MADCTL_BGR (0x08)
#define MADCTL_MH  (0x04)

// New color definitions use for all my libraries
#define TFT_BLACK       (0x0000)      /*   0,   0,   0 */
#define TFT_NAVY        (0x000F)      /*   0,   0, 128 */
#define TFT_DARKGREEN   (0x03E0)      /*   0, 128,   0 */
#define TFT_DARKCYAN    (0x03EF)      /*   0, 128, 128 */
#define TFT_MAROON      (0x7800)      /* 128,   0,   0 */
#define TFT_PURPLE      (0x780F)      /* 128,   0, 128 */
#define TFT_OLIVE       (0x7BE0)      /* 128, 128,   0 */
#define TFT_LIGHTGREY   (0xC618)      /* 192, 192, 192 */
#define TFT_DARKGREY    (0x7BEF)      /* 128, 128, 128 */
#define TFT_BLUE        (0x001F)      /*   0,   0, 255 */
#define TFT_GREEN       (0x07E0)      /*   0, 255,   0 */
#define TFT_CYAN        (0x07FF)      /*   0, 255, 255 */
#define TFT_RED         (0xF800)      /* 255,   0,   0 */
#define TFT_MAGENTA     (0xF81F)      /* 255,   0, 255 */
#define TFT_YELLOW      (0xFFE0)      /* 255, 255,   0 */
#define TFT_WHITE       (0xFFFF)      /* 255, 255, 255 */
#define TFT_ORANGE      (0xFD20)      /* 255, 165,   0 */
#define TFT_GREENYELLOW (0xAFE5)      /* 173, 255,  47 */
#define TFT_PINK        (0xF81F)

// Color definitions for backwards compatibility
#define ST7735_BLACK       (0x0000)      /*   0,   0,   0 */
#define ST7735_NAVY        (0x000F)      /*   0,   0, 128 */
#define ST7735_DARKGREEN   (0x03E0)      /*   0, 128,   0 */
#define ST7735_DARKCYAN    (0x03EF)      /*   0, 128, 128 */
#define ST7735_MAROON      (0x7800)      /* 128,   0,   0 */
#define ST7735_PURPLE      (0x780F)      /* 128,   0, 128 */
#define ST7735_OLIVE       (0x7BE0)      /* 128, 128,   0 */
#define ST7735_LIGHTGREY   (0xC618)      /* 192, 192, 192 */
#define ST7735_DARKGREY    (0x7BEF)      /* 128, 128, 128 */
#define ST7735_BLUE        (0x001F)      /*   0,   0, 255 */
#define ST7735_GREEN       (0x07E0)      /*   0, 255,   0 */
#define ST7735_CYAN        (0x07FF)      /*   0, 255, 255 */
#define ST7735_RED         (0xF800)      /* 255,   0,   0 */
#define ST7735_MAGENTA     (0xF81F)      /* 255,   0, 255 */
#define ST7735_YELLOW      (0xFFE0)      /* 255, 255,   0 */
#define ST7735_WHITE       (0xFFFF)      /* 255, 255, 255 */
#define ST7735_ORANGE      (0xFD20)      /* 255, 165,   0 */
#define ST7735_GREENYELLOW (0xAFE5)      /* 173, 255,  47 */
#define ST7735_PINK        (0xF81F)

/**
 * It turns out that all writes are done with only 1 byte
 */
#define TFT_ST7735_WRITE_BYTE_SPI(byte) \
    { \
        unsigned char _c_temp = (byte); \
        TFT_ST7735_Write_SPI(&_c_temp, 1); \
    }

typedef struct {
    const unsigned char *chartbl;
    const unsigned char *widthtbl;
    unsigned       char height;
} fontinfo;

typedef enum TFT_ST7735_Result_Tag
{
    RESULT_FAILURE,
    RESULT_SUCCESS,
    RESULT_MAX_ENUM
}TFT_ST7735_Result_T;

typedef enum TFT_ST7735_Chip_Select_Tag
{
    /** Set low voltage in the pin */
    CHIP_SELECT_LOW,
    /** Set a high voltage in the pin */
    CHIP_SELECT_HIGH,
    CHIP_SELECT_MAX_ENUM
}TFT_ST7735_CS_T;

typedef enum TFT_ST7735_Data_Command_Tag
{
    REQUEST_COMMAND,
    REQUEST_DATA,
    REQUEST_MAX_ENUM
}TFT_ST7735_Data_Command_T;

typedef enum TFT_ST7735_Reset_Tag
{
    /** Set low voltage in the pin */
    RESET_PIN_LOW,
    /** Set a high voltage in the pin */
    RESET_PIN_HIGH,
    RESET_MAX_ENUM
}TFT_ST7735_Reset_T;

extern const fontinfo fontdata [];

///////////////////////////////////////////////////////////////////////////////////////
/// CALLOUTS START HERE
///////////////////////////////////////////////////////////////////////////////////////

/* The following functions must be defined by the user in an independent C file
 * to match their uC environment */

/**
 * Any specific initialization stuff that the uC environment may need
 * for SPI and GPIOs (DC, RESET, CS, BACKLIGHT) must be implemented here
 * SPI clock = 15MHz maximum
 */
void TFT_ST7735_Configure_SPI(void);

/**
 * A blocking delay must be implemented in this call
 * Use HW or SW but this call must be blocking
 * @param ms - delay in micro seconds
 */
void TFT_ST7735_Delay(unsigned int ms);

/**
 * Set the physical voltage on the chip select line
 * @param status - refer to TFT_ST7735_CS_T
 */
void TFT_ST7735_Set_Chip_Select(TFT_ST7735_CS_T status);

/**
 * Set the mode of the request that will be sent
 * @param request - refer to TFT_ST7735_Data_Command_T
 */
void TFT_ST7735_Set_Data_Command(TFT_ST7735_Data_Command_T request);

/**
 * Set the reset pin status
 * @param status -  refer to TFT_ST7735_Data_Command_T
 */
void TFT_ST7735_Set_Reset(TFT_ST7735_Reset_T status);

/**
 * Write size bytes to the display
 * @attention Make sure to send data waiting for bytes to be sent
 * one after the other (wait until previous data has been sent).
 * CS, or DC must not be affected in this call.
 * @param data - pointer to an array of data to send
 * @param size - how many bytes to send
 */
void TFT_ST7735_Write_SPI(unsigned char *data, uint32_t size);

///////////////////////////////////////////////////////////////////////////////////////
/// CALLOUTS END HERE
///////////////////////////////////////////////////////////////////////////////////////

void TFT_ST7735_init(void);

void TFT_ST7735_begin(void); // Same - begin included for backwards compatibility

void TFT_ST7735_drawPixel(uint16_t x, uint16_t y, uint16_t color);

void TFT_ST7735_drawChar(int16_t x, int16_t y, unsigned char c, uint16_t color, uint16_t bg, uint8_t font);

void TFT_ST7735_setAddrWindow(int16_t x0, int16_t y0, int16_t x1, int16_t y1);

void TFT_ST7735_pushColor(uint16_t color);

void TFT_ST7735_pushColor_len(uint16_t color, uint16_t len);

void TFT_ST7735_pushColors(uint16_t *data, uint8_t len);

void TFT_ST7735_pushColors_8(uint8_t  *data, uint16_t len);

void TFT_ST7735_fillScreen(uint16_t color);

void TFT_ST7735_writeEnd(void);

void TFT_ST7735_drawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color);

void TFT_ST7735_drawFastVLine(int16_t x, int16_t y, int16_t h, uint16_t color);

void TFT_ST7735_drawFastHLine(int16_t x, int16_t y, int16_t w, uint16_t color);

void TFT_ST7735_drawRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color);

void TFT_ST7735_fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color);

void TFT_ST7735_drawRoundRect(int16_t x0, int16_t y0, int16_t w, int16_t h, int16_t radius, uint16_t color);

void TFT_ST7735_fillRoundRect(int16_t x0, int16_t y0, int16_t w, int16_t h, int16_t radius, uint16_t color);

void TFT_ST7735_setRotation(uint8_t r);

void TFT_ST7735_invertDisplay(unsigned char i);

void TFT_ST7735_drawCircle(int16_t x0, int16_t y0, int16_t r, uint16_t color);

void TFT_ST7735_drawCircleHelper(int16_t x0, int16_t y0, int16_t r, uint8_t cornername, uint16_t color);

void TFT_ST7735_fillCircle(int16_t x0, int16_t y0, int16_t r, uint16_t color);

void TFT_ST7735_fillCircleHelper(int16_t x0, int16_t y0, int16_t r, uint8_t cornername, int16_t delta, uint16_t color);

void TFT_ST7735_drawEllipse(int16_t x0, int16_t y0, int16_t rx, int16_t ry, uint16_t color);

void TFT_ST7735_fillEllipse(int16_t x0, int16_t y0, int16_t rx, int16_t ry, uint16_t color);

void TFT_ST7735_drawTriangle(int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint16_t color);

void TFT_ST7735_fillTriangle(int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint16_t color);

void TFT_ST7735_drawBitmap(int16_t x, int16_t y, const uint8_t *bitmap, int16_t w, int16_t h, uint16_t color);

void TFT_ST7735_setCursor(int16_t x, int16_t y);

void TFT_ST7735_setCursor_font(int16_t x, int16_t y, uint8_t font);

void TFT_ST7735_setTextColor(uint16_t color);

void TFT_ST7735_setTextColor_bgcolor(uint16_t fgcolor, uint16_t bgcolor);

void TFT_ST7735_setTextSize(uint8_t size);

void TFT_ST7735_setTextFont(uint8_t font);

void TFT_ST7735_setTextWrap(unsigned char wrap);

void TFT_ST7735_setTextDatum(uint8_t datum);

void TFT_ST7735_setTextPadding(uint16_t x_width);

void TFT_ST7735_writecommand(uint8_t c);

void TFT_ST7735_writedata(uint8_t d);

void TFT_ST7735_commandList(const uint8_t *addr);

uint8_t TFT_ST7735_getRotation(void);

uint16_t TFT_ST7735_fontsLoaded(void);

uint16_t TFT_ST7735_color565(uint8_t r, uint8_t g, uint8_t b);

int TFT_ST7735_drawChar_uniCode(unsigned int uniCode, int x, int y, int font);

int TFT_ST7735_drawNumber(long long_num,int poX, int poY, int font);

int TFT_ST7735_drawFloat(float floatNumber,int decimal,int poX, int poY, int font);

int TFT_ST7735_drawString(char *string, int poX, int poY, int font);

int TFT_ST7735_drawCentreString(char *string, int dX, int poY, int font);

int TFT_ST7735_drawRightString(char *string, int dX, int poY, int font);

int16_t TFT_ST7735_height(void);

int16_t TFT_ST7735_width(void);

int16_t TFT_ST7735_textWidth(char *string, int font);

int16_t TFT_ST7735_fontHeight(int font);

/***************************************************

  ORIGINAL LIBRARY HEADER

  This is our library for the Adafruit  ST7735 Breakout and Shield
  ----> http://www.adafruit.com/products/1651

  Check out the links above for our tutorials and wiring diagrams
  These displays use SPI to communicate, 4 or 5 pins are required to
  interface (RST is optional)
  Adafruit invests time and resources providing this open source code,
  please support Adafruit and open-source hardware by purchasing
  products from Adafruit!

  Written by Limor Fried/Ladyada for Adafruit Industries.
  MIT license, all text above must be included in any redistribution

  Updated with new functions by Bodmer 14/4/15
  https://github.com/Bodmer/TFT_ST7735
 ****************************************************/

#endif /* #ifndef TFT_ST7735_H */
