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

#include <limits.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "TFT_ST7735.h"

/* ltoa */
#define BUFSIZE (sizeof(long) * 8 + 1)

// Initialization commands for ST7735 screens
#define DELAY (0x80)

/**
 * Read a byte from ROM (const unsigned char)
 * @attention Original from AVR-GCC http://download.savannah.gnu.org/releases/avr-libc/
 * @param addr - an address from ROM
 * @return byte from that location
 */
#define TFT_ST7735_PGM_READ_BYTE(addr) (*((const uint8_t*)(addr)))

static uint8_t tabcolor, colstart, rowstart; // some displays need this changed
static int16_t  _width, _height, // Display w/h as modified by current rotation
         cursor_x, cursor_y, padX;

static uint16_t textcolor, textbgcolor, fontsloaded;

static uint8_t  addr_row, addr_col;

static uint8_t  textfont,
         textsize,
         textdatum,
         rotation;

static uint8_t textwrap; // If set, 'wrap' text at right edge of display

static const uint8_t Bcmd[] = {                  // Initialization commands for 7735B screens
        18,                       // 18 commands in list:
        ST7735_SWRESET,   DELAY,  //  1: Software reset, no args, w/delay
        50,                     //     50 ms delay
        ST7735_SLPOUT ,   DELAY,  //  2: Out of sleep mode, no args, w/delay
        255,                    //     255 = 500 ms delay
        ST7735_COLMOD , 1+DELAY,  //  3: Set color mode, 1 arg + delay:
        0x05,                   //     16-bit color
        10,                     //     10 ms delay
        ST7735_FRMCTR1, 3+DELAY,  //  4: Frame rate control, 3 args + delay:
        0x00,                   //     fastest refresh
        0x06,                   //     6 lines front porch
        0x03,                   //     3 lines back porch
        10,                     //     10 ms delay
        ST7735_MADCTL , 1      ,  //  5: Memory access ctrl (directions), 1 arg:
        0x08,                   //     Row addr/col addr, bottom to top refresh
        ST7735_DISSET5, 2      ,  //  6: Display settings #5, 2 args, no delay:
        0x15,                   //     1 clk cycle nonoverlap, 2 cycle gate
        //     rise, 3 cycle osc equalize
        0x02,                   //     Fix on VTL
        ST7735_INVCTR , 1      ,  //  7: Display inversion control, 1 arg:
        0x0,                    //     Line inversion
        ST7735_PWCTR1 , 2+DELAY,  //  8: Power control, 2 args + delay:
        0x02,                   //     GVDD = 4.7V
        0x70,                   //     1.0uA
        10,                     //     10 ms delay
        ST7735_PWCTR2 , 1      ,  //  9: Power control, 1 arg, no delay:
        0x05,                   //     VGH = 14.7V, VGL = -7.35V
        ST7735_PWCTR3 , 2      ,  // 10: Power control, 2 args, no delay:
        0x01,                   //     Opamp current small
        0x02,                   //     Boost frequency
        ST7735_VMCTR1 , 2+DELAY,  // 11: Power control, 2 args + delay:
        0x3C,                   //     VCOMH = 4V
        0x38,                   //     VCOML = -1.1V
        10,                     //     10 ms delay
        ST7735_PWCTR6 , 2      ,  // 12: Power control, 2 args, no delay:
        0x11, 0x15,
        ST7735_GMCTRP1,16      ,  // 13: Magical unicorn dust, 16 args, no delay:
        0x09, 0x16, 0x09, 0x20, //     (seriously though, not sure what
        0x21, 0x1B, 0x13, 0x19, //      these config values represent)
        0x17, 0x15, 0x1E, 0x2B,
        0x04, 0x05, 0x02, 0x0E,
        ST7735_GMCTRN1,16+DELAY,  // 14: Sparkles and rainbows, 16 args + delay:
        0x0B, 0x14, 0x08, 0x1E, //     (ditto)
        0x22, 0x1D, 0x18, 0x1E,
        0x1B, 0x1A, 0x24, 0x2B,
        0x06, 0x06, 0x02, 0x0F,
        10,                     //     10 ms delay
        ST7735_CASET  , 4      ,  // 15: Column addr set, 4 args, no delay:
        0x00, 0x02,             //     XSTART = 2
        0x00, 0x81,             //     XEND = 129
        ST7735_RASET  , 4      ,  // 16: Row addr set, 4 args, no delay:
        0x00, 0x02,             //     XSTART = 1
        0x00, 0x81,             //     XEND = 160
        ST7735_NORON  ,   DELAY,  // 17: Normal display on, no args, w/delay
        10,                     //     10 ms delay
        ST7735_DISPON ,   DELAY,  // 18: Main screen turn on, no args, w/delay
        255 };                  //     255 = 500 ms delay

static const uint8_t Rcmd1[] = {                 // Init for 7735R, part 1 (red or green tab)
        15,                       // 15 commands in list:
        ST7735_SWRESET,   DELAY,  //  1: Software reset, 0 args, w/delay
        150,                    //     150 ms delay
        ST7735_SLPOUT ,   DELAY,  //  2: Out of sleep mode, 0 args, w/delay
        255,                    //     500 ms delay
        ST7735_FRMCTR1, 3      ,  //  3: Frame rate ctrl - normal mode, 3 args:
        0x01, 0x2C, 0x2D,       //     Rate = fosc/(1x2+40) * (LINE+2C+2D)
        ST7735_FRMCTR2, 3      ,  //  4: Frame rate control - idle mode, 3 args:
        0x01, 0x2C, 0x2D,       //     Rate = fosc/(1x2+40) * (LINE+2C+2D)
        ST7735_FRMCTR3, 6      ,  //  5: Frame rate ctrl - partial mode, 6 args:
        0x01, 0x2C, 0x2D,       //     Dot inversion mode
        0x01, 0x2C, 0x2D,       //     Line inversion mode
        ST7735_INVCTR , 1      ,  //  6: Display inversion ctrl, 1 arg, no delay:
        0x07,                   //     No inversion
        ST7735_PWCTR1 , 3      ,  //  7: Power control, 3 args, no delay:
        0xA2,
        0x02,                   //     -4.6V
        0x84,                   //     AUTO mode
        ST7735_PWCTR2 , 1      ,  //  8: Power control, 1 arg, no delay:
        0xC5,                   //     VGH25 = 2.4C VGSEL = -10 VGH = 3 * AVDD
        ST7735_PWCTR3 , 2      ,  //  9: Power control, 2 args, no delay:
        0x0A,                   //     Opamp current small
        0x00,                   //     Boost frequency
        ST7735_PWCTR4 , 2      ,  // 10: Power control, 2 args, no delay:
        0x8A,                   //     BCLK/2, Opamp current small & Medium low
        0x2A,
        ST7735_PWCTR5 , 2      ,  // 11: Power control, 2 args, no delay:
        0x8A, 0xEE,
        ST7735_VMCTR1 , 1      ,  // 12: Power control, 1 arg, no delay:
        0x0E,
        ST7735_INVOFF , 0      ,  // 13: Don't invert display, no args, no delay
        ST7735_MADCTL , 1      ,  // 14: Memory access control (directions), 1 arg:
        0xC8,                   //     row addr/col addr, bottom to top refresh
        ST7735_COLMOD , 1      ,  // 15: set color mode, 1 arg, no delay:
        0x05 };                 //     16-bit color

static const uint8_t Rcmd2green[] = {            // Init for 7735R, part 2 (green tab only)
        2,                        //  2 commands in list:
        ST7735_CASET  , 4      ,  //  1: Column addr set, 4 args, no delay:
        0x00, 0x02,             //     XSTART = 0
        0x00, 0x7F+0x02,        //     XEND = 127
        ST7735_RASET  , 4      ,  //  2: Row addr set, 4 args, no delay:
        0x00, 0x01,             //     XSTART = 0
        0x00, 0x9F+0x01 };      //     XEND = 159

static const uint8_t Rcmd2red[] = {              // Init for 7735R, part 2 (red tab only)
        2,                        //  2 commands in list:
        ST7735_CASET  , 4      ,  //  1: Column addr set, 4 args, no delay:
        0x00, 0x00,             //     XSTART = 0
        0x00, 0x7F,             //     XEND = 127
        ST7735_RASET  , 4      ,  //  2: Row addr set, 4 args, no delay:
        0x00, 0x00,             //     XSTART = 0
        0x00, 0x9F };           //     XEND = 159

static const uint8_t Rcmd3[] = {                 // Init for 7735R, part 3 (red or green tab)
        4,                        //  4 commands in list:
        ST7735_GMCTRP1, 16      , //  1: Magical unicorn dust, 16 args, no delay:
        0x02, 0x1c, 0x07, 0x12,
        0x37, 0x32, 0x29, 0x2d,
        0x29, 0x25, 0x2B, 0x39,
        0x00, 0x01, 0x03, 0x10,
        ST7735_GMCTRN1, 16      , //  2: Sparkles and rainbows, 16 args, no delay:
        0x03, 0x1d, 0x07, 0x06,
        0x2E, 0x2C, 0x29, 0x2D,
        0x2E, 0x2E, 0x37, 0x3F,
        0x00, 0x00, 0x02, 0x10,
        ST7735_NORON  ,    DELAY, //  3: Normal display on, no args, w/delay
        10,                     //     10 ms delay
        ST7735_DISPON ,    DELAY, //  4: Main screen turn on, no args w/delay
        100 };                  //     100 ms delay

static void TFT_ST7735_setWindow(int16_t x0, int16_t y0, int16_t x1, int16_t y1);

/*
**  LTOA.C
**
**  Converts a long integer to a string.
**
**  Copyright 1988-90 by Robert B. Stout dba MicroFirm
**
**  Released to public domain, 1991
**
**  Parameters: 1 - number to be converted
**              2 - buffer in which to build the converted string
**              3 - number base to use for conversion
**
**  Returns:  A character pointer to the converted string if
**            successful, a NULL pointer if the number base specified
**            is out of range.
*/
static char* TFT_ST7735_ltoa(long N, char *str, int base);

/**
 * Placeholder for the construction of the non-existent class
 * @param w screen width
 * @param h screen height
 */
static void TFT_ST7735_Construct(int16_t w, int16_t h);

/**
 * Swap integers
 * @param a - this will be allocated in b
 * @param b - this will be allocated in a
 */
static void tftswap(int16_t *a, int16_t *b);

static char* TFT_ST7735_ltoa(long N, char *str, int base)
{
      int i = 2;
      long uarg;
      char *tail, *head = str, buf[BUFSIZE];

      if (36 < base || 2 > base)
            base = 10;                    /* can only use 0-9, A-Z        */
      tail = &buf[BUFSIZE - 1];           /* last character position      */
      *tail-- = '\0';

      if (10 == base && N < 0L)
      {
            *head++ = '-';
            uarg    = -N;
      }
      else  uarg = N;

      if (uarg)
      {
            for (i = 1; uarg; ++i)
            {
                  register ldiv_t r;

                  r       = ldiv(uarg, base);
                  *tail-- = (char)(r.rem + ((9L < r.rem) ?
                                  ('A' - 10L) : '0'));
                  uarg    = r.quot;
            }
      }
      else  *tail-- = '0';

      memcpy(head, ++tail, i);
      return str;
}

static void tftswap(int16_t *a, int16_t *b)
{
    int16_t temp = *a;

    *a = *b;
    *b = temp;
}

/***************************************************************************************
** Function name:           TFT_ST7735
** Description:             Constructor
***************************************************************************************/
static void TFT_ST7735_Construct(int16_t w, int16_t h)
{
    /* Reset the display */
    (void)TFT_ST7735_Set_Reset(RESET_PIN_LOW);

    (void)TFT_ST7735_Set_Data_Command(REQUEST_DATA);

    (void)TFT_ST7735_Set_Chip_Select(CHIP_SELECT_HIGH);

    _width    = w;
    _height   = h;
    rotation  = 0;
    cursor_y  = cursor_x    = 0;
    textfont  = 1;
    textsize  = 1;
    textcolor   = 0xFFFF;
    textbgcolor = 0x0000;
    padX = 0;
    textwrap  = 1;
    textdatum = 0; // Left text alignment is default
    fontsloaded = 0;

    addr_row = 0xFF;
    addr_col = 0xFF;

    #ifdef LOAD_GLCD
    fontsloaded = 0x0002; // Bit 1 set
    #endif

    #ifdef LOAD_FONT2
    fontsloaded |= 0x0004; // Bit 2 set
    #endif

    #ifdef LOAD_FONT4
    fontsloaded |= 0x0010; // Bit 4 set
    #endif

    #ifdef LOAD_FONT6
    fontsloaded |= 0x0040; // Bit 6 set
    #endif

    #ifdef LOAD_FONT7
    fontsloaded |= 0x0080; // Bit 7 set
    #endif

    #ifdef LOAD_FONT8
    fontsloaded |= 0x0100; // Bit 8 set
    #endif
}

/***************************************************************************************
** Function name:           TFT_ST7735_writecommand
** Description:             Send an 8 bit command to the TFT
***************************************************************************************/
void TFT_ST7735_writecommand(uint8_t c)
{
  TFT_ST7735_Set_Data_Command(REQUEST_COMMAND);
  TFT_ST7735_Set_Chip_Select(CHIP_SELECT_LOW);
  TFT_ST7735_WRITE_BYTE_SPI(c);
  (void)TFT_ST7735_Set_Chip_Select(CHIP_SELECT_HIGH);
}

/***************************************************************************************
** Function name:           TFT_ST7735_writedata
** Description:             Send a 8 bit data value to the TFT
***************************************************************************************/
void TFT_ST7735_writedata(uint8_t c)
{
  (void)TFT_ST7735_Set_Data_Command(REQUEST_DATA);
  TFT_ST7735_Set_Chip_Select(CHIP_SELECT_LOW);
  TFT_ST7735_WRITE_BYTE_SPI(c);
  (void)TFT_ST7735_Set_Chip_Select(CHIP_SELECT_HIGH);
}

/***************************************************************************************
** Function name:           writeEnd
** Description:             Raise the Chip Select
***************************************************************************************/
void TFT_ST7735_writeEnd() {
  (void)TFT_ST7735_Set_Chip_Select(CHIP_SELECT_HIGH);
}

/***************************************************************************************
** Function name:           begin
** Description:             Included for backwards compatibility
***************************************************************************************/
void TFT_ST7735_begin(void)
{
    TFT_ST7735_init();
}

/***************************************************************************************
** Function name:           init
** Description:             Reset, then initialise the TFT display registers
***************************************************************************************/
void TFT_ST7735_init(void)
{
    TFT_ST7735_Configure_SPI();

    TFT_ST7735_Construct(ST7735_TFTWIDTH, ST7735_TFTHEIGHT);

    // toggle RST low to reset
    (void)TFT_ST7735_Set_Reset(RESET_PIN_HIGH);
    TFT_ST7735_Delay(TFT_ST7735_FIRST_RESET_HIGH_DELAY);
    (void)TFT_ST7735_Set_Reset(RESET_PIN_LOW);
    TFT_ST7735_Delay(TFT_ST7735_SECOND_RESET_LOW_DELAY);
    (void)TFT_ST7735_Set_Reset(RESET_PIN_HIGH);
    TFT_ST7735_Delay(TFT_ST7735_THIRD_RESET_HIGH_DELAY);

    tabcolor = TAB_COLOUR;

    if (tabcolor == INITB)
    {
        TFT_ST7735_commandList(&Bcmd[0]);
    }
    else
    {
        TFT_ST7735_commandList(&Rcmd1[0]);
        if (tabcolor == INITR_GREENTAB)
        {
            TFT_ST7735_commandList(&Rcmd2green[0]);
            colstart = 2;
            rowstart = 1;
        }
        else if (tabcolor == INITR_GREENTAB2)
        {
            TFT_ST7735_commandList(&Rcmd2green[0]);
            TFT_ST7735_writecommand(ST7735_MADCTL);
            TFT_ST7735_writedata(0xC0);
            colstart = 2;
            rowstart = 1;
        }
        else if (tabcolor == INITR_REDTAB)
        {
            TFT_ST7735_commandList(&Rcmd2red[0]);
        }
        else if (tabcolor == INITR_BLACKTAB)
        {
            TFT_ST7735_writecommand(ST7735_MADCTL);
            TFT_ST7735_writedata(0xC0);
        }
        TFT_ST7735_commandList(&Rcmd3[0]);
    }
}

/***************************************************************************************
** Function name:           TFT_ST7735_commandList
** Description:             Get initialisation commands from FLASH and send to TFT
***************************************************************************************/
void TFT_ST7735_commandList (const uint8_t *addr)
{
  uint8_t  numCommands, numArgs;
  uint8_t  ms;

  numCommands = TFT_ST7735_PGM_READ_BYTE(addr++); // Number of commands to follow
  while (numCommands--)                           // For each command...
  {
    TFT_ST7735_writecommand(TFT_ST7735_PGM_READ_BYTE(addr++));    // Read, issue command
    numArgs = TFT_ST7735_PGM_READ_BYTE(addr++);        // Number of args to follow
    ms = numArgs & ST7735_INIT_DELAY;      // If hibit set, delay follows args
    numArgs &= ~ST7735_INIT_DELAY;         // Mask out delay bit
    while (numArgs--)                       // For each argument...
    {
      TFT_ST7735_writedata(TFT_ST7735_PGM_READ_BYTE(addr++)); // Read, issue argument
    }

    if (ms)
    {
      ms = TFT_ST7735_PGM_READ_BYTE(addr++);     // Read post-command delay time (ms)
      TFT_ST7735_Delay( (ms==255 ? 500 : ms) );
    }
  }

}

/***************************************************************************************
** Function name:           drawCircle
** Description:             Draw a circle outline
***************************************************************************************/
void TFT_ST7735_drawCircle(int16_t x0, int16_t y0, int16_t r, uint16_t color)
{
  int16_t f = 1 - r;
  int16_t ddF_x = 1;
  int16_t ddF_y = - r - r;
  int16_t x = 0;

  TFT_ST7735_drawPixel(x0 + r, y0  , color);
  TFT_ST7735_drawPixel(x0 - r, y0  , color);
  TFT_ST7735_drawPixel(x0  , y0 - r, color);
  TFT_ST7735_drawPixel(x0  , y0 + r, color);

  while (x < r) {
    if (f >= 0) {
      r--;
      ddF_y += 2;
      f += ddF_y;
    }
    x++;
    ddF_x += 2;
    f += ddF_x;

    TFT_ST7735_drawPixel(x0 + x, y0 + r, color);
    TFT_ST7735_drawPixel(x0 - x, y0 + r, color);
    TFT_ST7735_drawPixel(x0 - x, y0 - r, color);
    TFT_ST7735_drawPixel(x0 + x, y0 - r, color);

    TFT_ST7735_drawPixel(x0 + r, y0 + x, color);
    TFT_ST7735_drawPixel(x0 - r, y0 + x, color);
    TFT_ST7735_drawPixel(x0 - r, y0 - x, color);
    TFT_ST7735_drawPixel(x0 + r, y0 - x, color);
  }
}

/***************************************************************************************
** Function name:           TFT_ST7735_drawCircleHelper
** Description:             Support function for circle drawing
***************************************************************************************/
void TFT_ST7735_drawCircleHelper( int16_t x0, int16_t y0, int16_t r, uint8_t cornername, uint16_t color)
{
  int16_t f     = 1 - r;
  int16_t ddF_x = 1;
  int16_t ddF_y = -2 * r;
  int16_t x     = 0;

  while (x < r) {
    if (f >= 0) {
      r--;
      ddF_y += 2;
      f     += ddF_y;
    }
    x++;
    ddF_x += 2;
    f     += ddF_x;
    if (cornername & 0x8) {
      TFT_ST7735_drawPixel(x0 - r, y0 + x, color);
      TFT_ST7735_drawPixel(x0 - x, y0 + r, color);
    }
    if (cornername & 0x4) {
      TFT_ST7735_drawPixel(x0 + x, y0 + r, color);
      TFT_ST7735_drawPixel(x0 + r, y0 + x, color);
    }
    if (cornername & 0x2) {
      TFT_ST7735_drawPixel(x0 + r, y0 - x, color);
      TFT_ST7735_drawPixel(x0 + x, y0 - r, color);
    }
    if (cornername & 0x1) {
      TFT_ST7735_drawPixel(x0 - x, y0 - r, color);
      TFT_ST7735_drawPixel(x0 - r, y0 - x, color);
    }

  }
}

/***************************************************************************************
** Function name:           fillCircle
** Description:             draw a filled circle
***************************************************************************************/
void TFT_ST7735_fillCircle(int16_t x0, int16_t y0, int16_t r, uint16_t color)
{
  TFT_ST7735_drawFastVLine(x0, y0 - r, r + r + 1, color);
  TFT_ST7735_fillCircleHelper(x0, y0, r, 3, 0, color);
}

/***************************************************************************************
** Function name:           TFT_ST7735_fillCircleHelper
** Description:             Support function for filled circle drawing
***************************************************************************************/
// Used to do circles and roundrects
void TFT_ST7735_fillCircleHelper(int16_t x0, int16_t y0, int16_t r, uint8_t cornername, int16_t delta, uint16_t color)
{
  int16_t f     = 1 - r;
  int16_t ddF_x = 1;
  int16_t ddF_y = -r - r;
  int16_t x     = 0;

  delta++;
  while (x < r) {
    if (f >= 0) {
      r--;
      ddF_y += 2;
      f     += ddF_y;
    }
    x++;
    ddF_x += 2;
    f     += ddF_x;

    if (cornername & 0x1) {
      TFT_ST7735_drawFastVLine(x0 + x, y0 - r, r + r + delta, color);
      TFT_ST7735_drawFastVLine(x0 + r, y0 - x, x + x + delta, color);
    }
    if (cornername & 0x2) {
      TFT_ST7735_drawFastVLine(x0 - x, y0 - r, r + r + delta, color);
      TFT_ST7735_drawFastVLine(x0 - r, y0 - x, x + x + delta, color);
    }
  }
}

/***************************************************************************************
** Function name:           drawEllipse
** Description:             Draw a ellipse outline
***************************************************************************************/
void TFT_ST7735_drawEllipse(int16_t x0, int16_t y0, int16_t rx, int16_t ry, uint16_t color)
{
  if (rx<2) return;
  if (ry<2) return;
  int16_t x, y;
  int32_t rx2 = rx * rx;
  int32_t ry2 = ry * ry;
  int32_t fx2 = 4 * rx2;
  int32_t fy2 = 4 * ry2;
  int32_t s;

  for (x = 0, y = ry, s = 2*ry2+rx2*(1-2*ry); ry2*x <= rx2*y; x++)
  {
    TFT_ST7735_drawPixel(x0 + x, y0 + y, color);
    TFT_ST7735_drawPixel(x0 - x, y0 + y, color);
    TFT_ST7735_drawPixel(x0 - x, y0 - y, color);
    TFT_ST7735_drawPixel(x0 + x, y0 - y, color);
    if (s >= 0)
    {
      s += fx2 * (1 - y);
      y--;
    }
    s += ry2 * ((4 * x) + 6);
  }

  for (x = rx, y = 0, s = 2*rx2+ry2*(1-2*rx); rx2*y <= ry2*x; y++)
  {
    TFT_ST7735_drawPixel(x0 + x, y0 + y, color);
    TFT_ST7735_drawPixel(x0 - x, y0 + y, color);
    TFT_ST7735_drawPixel(x0 - x, y0 - y, color);
    TFT_ST7735_drawPixel(x0 + x, y0 - y, color);
    if (s >= 0)
    {
      s += fy2 * (1 - x);
      x--;
    }
    s += rx2 * ((4 * y) + 6);
  }
}

/***************************************************************************************
** Function name:           fillEllipse
** Description:             draw a filled ellipse
***************************************************************************************/
void TFT_ST7735_fillEllipse(int16_t x0, int16_t y0, int16_t rx, int16_t ry, uint16_t color)
{
  if (rx<2) return;
  if (ry<2) return;
  int16_t x, y;
  int32_t rx2 = rx * rx;
  int32_t ry2 = ry * ry;
  int32_t fx2 = 4 * rx2;
  int32_t fy2 = 4 * ry2;
  int32_t s;

  for (x = 0, y = ry, s = 2*ry2+rx2*(1-2*ry); ry2*x <= rx2*y; x++)
  {
    TFT_ST7735_drawFastHLine(x0 - x, y0 - y, x + x + 1, color);
    TFT_ST7735_drawFastHLine(x0 - x, y0 + y, x + x + 1, color);

    if (s >= 0)
    {
      s += fx2 * (1 - y);
      y--;
    }
    s += ry2 * ((4 * x) + 6);
  }

  for (x = rx, y = 0, s = 2*rx2+ry2*(1-2*rx); rx2*y <= ry2*x; y++)
  {
    TFT_ST7735_drawFastHLine(x0 - x, y0 - y, x + x + 1, color);
    TFT_ST7735_drawFastHLine(x0 - x, y0 + y, x + x + 1, color);

    if (s >= 0)
    {
      s += fy2 * (1 - x);
      x--;
    }
    s += rx2 * ((4 * y) + 6);
  }

}

/***************************************************************************************
** Function name:           fillScreen
** Description:             Clear the screen to defined colour
***************************************************************************************/
void TFT_ST7735_fillScreen(uint16_t color)
{
  TFT_ST7735_fillRect(0, 0, _width, _height, color);
}

/***************************************************************************************
** Function name:           drawRect
** Description:             Draw a rectangle outline
***************************************************************************************/
// Draw a rectangle
void TFT_ST7735_drawRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color)
{
  TFT_ST7735_drawFastHLine(x, y, w, color);
  TFT_ST7735_drawFastHLine(x, y + h - 1, w, color);
  TFT_ST7735_drawFastVLine(x, y, h, color);
  TFT_ST7735_drawFastVLine(x + w - 1, y, h, color);
}

/***************************************************************************************
** Function name:           drawRoundRect
** Description:             Draw a rounded corner rectangle outline
***************************************************************************************/
// Draw a rounded rectangle
void TFT_ST7735_drawRoundRect(int16_t x, int16_t y, int16_t w, int16_t h, int16_t r, uint16_t color)
{
  // smarter version
  TFT_ST7735_drawFastHLine(x + r  , y    , w - r - r, color); // Top
  TFT_ST7735_drawFastHLine(x + r  , y + h - 1, w - r - r, color); // Bottom
  TFT_ST7735_drawFastVLine(x    , y + r  , h - r - r, color); // Left
  TFT_ST7735_drawFastVLine(x + w - 1, y + r  , h - r - r, color); // Right
  // draw four corners
  TFT_ST7735_drawCircleHelper(x + r    , y + r    , r, 1, color);
  TFT_ST7735_drawCircleHelper(x + r    , y + h - r - 1, r, 8, color);
  TFT_ST7735_drawCircleHelper(x + w - r - 1, y + r    , r, 2, color);
  TFT_ST7735_drawCircleHelper(x + w - r - 1, y + h - r - 1, r, 4, color);
}

/***************************************************************************************
** Function name:           fillRoundRect
** Description:             Draw a rounded corner filled rectangle
***************************************************************************************/
// Fill a rounded rectangle
void TFT_ST7735_fillRoundRect(int16_t x, int16_t y, int16_t w, int16_t h, int16_t r, uint16_t color)
{
  // smarter version
  TFT_ST7735_fillRect(x + r, y, w - r - r, h, color);

  // draw four corners
  TFT_ST7735_fillCircleHelper(x + w - r - 1, y + r, r, 1, h - r - r - 1, color);
  TFT_ST7735_fillCircleHelper(x + r    , y + r, r, 2, h - r - r - 1, color);
}

/***************************************************************************************
** Function name:           drawTriangle
** Description:             Draw a triangle outline using 3 arbitrary points
***************************************************************************************/
// Draw a triangle
void TFT_ST7735_drawTriangle(int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint16_t color)
{
  TFT_ST7735_drawLine(x0, y0, x1, y1, color);
  TFT_ST7735_drawLine(x1, y1, x2, y2, color);
  TFT_ST7735_drawLine(x2, y2, x0, y0, color);
}

/***************************************************************************************
** Function name:           fillTriangle 
** Description:             Draw a filled triangle using 3 arbitrary points
***************************************************************************************/
// Fill a triangle - original Adafruit function works well and code footprint is small
void TFT_ST7735_fillTriangle ( int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint16_t color)
{
  int16_t a, b, y, last;

  // Sort coordinates by Y order (y2 >= y1 >= y0)
  if (y0 > y1) {
    tftswap(&y0, &y1); tftswap(&x0, &x1);
  }
  if (y1 > y2) {
    tftswap(&y2, &y1); tftswap(&x2, &x1);
  }
  if (y0 > y1) {
    tftswap(&y0, &y1); tftswap(&x0, &x1);
  }

  if (y0 == y2) { // Handle awkward all-on-same-line case as its own thing
    a = b = x0;
    if (x1 < a)      a = x1;
    else if (x1 > b) b = x1;
    if (x2 < a)      a = x2;
    else if (x2 > b) b = x2;
    TFT_ST7735_drawFastHLine(a, y0, b - a + 1, color);
    return;
  }

  int16_t
  dx01 = x1 - x0,
  dy01 = y1 - y0,
  dx02 = x2 - x0,
  dy02 = y2 - y0,
  dx12 = x2 - x1,
  dy12 = y2 - y1,
  sa   = 0,
  sb   = 0;

  // For upper part of triangle, find scanline crossings for segments
  // 0-1 and 0-2.  If y1=y2 (flat-bottomed triangle), the scanline y1
  // is included here (and second loop will be skipped, avoiding a /0
  // error there), otherwise scanline y1 is skipped here and handled
  // in the second loop...which also avoids a /0 error here if y0=y1
  // (flat-topped triangle).
  if (y1 == y2) last = y1;  // Include y1 scanline
  else         last = y1 - 1; // Skip it

  for (y = y0; y <= last; y++) {
    a   = x0 + sa / dy01;
    b   = x0 + sb / dy02;
    sa += dx01;
    sb += dx02;

    if (a > b) tftswap(&a, &b);
    TFT_ST7735_drawFastHLine(a, y, b - a + 1, color);
  }

  // For lower part of triangle, find scanline crossings for segments
  // 0-2 and 1-2.  This loop is skipped if y1=y2.
  sa = dx12 * (y - y1);
  sb = dx02 * (y - y0);
  for (; y <= y2; y++) {
    a   = x1 + sa / dy12;
    b   = x0 + sb / dy02;
    sa += dx12;
    sb += dx02;

    if (a > b) tftswap(&a, &b);
    TFT_ST7735_drawFastHLine(a, y, b - a + 1, color);
  }
}

/***************************************************************************************
** Function name:           drawBitmap
** Description:             Draw an image stored in an array on the TFT
***************************************************************************************/
void TFT_ST7735_drawBitmap(int16_t x, int16_t y, const uint8_t *bitmap, int16_t w, int16_t h, uint16_t color)
{

  int16_t i, j, byteWidth = (w + 7) / 8;

  for (j = 0; j < h; j++) {
    for (i = 0; i < w; i++ ) {
      if (TFT_ST7735_PGM_READ_BYTE(bitmap + j * byteWidth + i / 8) & (128 >> (i & 7))) {
        TFT_ST7735_drawPixel(x + i, y + j, color);
      }
    }
  }
}

/***************************************************************************************
** Function name:           setCursor
** Description:             Set the text cursor x,y position
***************************************************************************************/
void TFT_ST7735_setCursor(int16_t x, int16_t y)
{
  cursor_x = x;
  cursor_y = y;
}

/***************************************************************************************
** Function name:           setCursor
** Description:             Set the text cursor x,y position and font
***************************************************************************************/
void TFT_ST7735_setCursor_font(int16_t x, int16_t y, uint8_t font)
{
  textfont = font;
  cursor_x = x;
  cursor_y = y;
}

/***************************************************************************************
** Function name:           setTextSize
** Description:             Set the text size multiplier
***************************************************************************************/
void TFT_ST7735_setTextSize(uint8_t s)
{
  if (s>7) s = 7; // Limit the maximum size multiplier so byte variables can be used for rendering
  textsize = (s > 0) ? s : 1; // Don't allow font size 0
}

/***************************************************************************************
** Function name:           setTextFont
** Description:             Set the font for the print stream
***************************************************************************************/
void TFT_ST7735_setTextFont(uint8_t f)
{
  textfont = (f > 0) ? f : 1; // Don't allow font 0
}

/***************************************************************************************
** Function name:           setTextColor
** Description:             Set the font foreground colour (background is transparent)
***************************************************************************************/
void TFT_ST7735_setTextColor(uint16_t c)
{
  // For 'transparent' background, we'll set the bg
  // to the same as fg instead of using a flag
  textcolor = textbgcolor = c;
}

/***************************************************************************************
** Function name:           setTextColor
** Description:             Set the font foreground and background colour
***************************************************************************************/
void TFT_ST7735_setTextColor_bgcolor(uint16_t c, uint16_t b)
{
  textcolor   = c;
  textbgcolor = b;
}

/***************************************************************************************
** Function name:           setTextWrap
** Description:             Define if text should wrap at end of line
***************************************************************************************/
void TFT_ST7735_setTextWrap(unsigned char w)
{
  textwrap = w;
}

/***************************************************************************************
** Function name:           setTextDatum
** Description:             Set the text position reference datum
***************************************************************************************/
void TFT_ST7735_setTextDatum(uint8_t d)
{
  textdatum = d;
}

/***************************************************************************************
** Function name:           setTextPadding
** Description:             Define padding width (aids erasing old text and numbers)
***************************************************************************************/
void TFT_ST7735_setTextPadding(uint16_t x_width)
{
  padX = x_width;
}

/***************************************************************************************
** Function name:           getRotation
** Description:             Return the rotation value (as used by setRotation())
***************************************************************************************/
uint8_t TFT_ST7735_getRotation(void)
{
  return rotation;
}

/***************************************************************************************
** Function name:           width
** Description:             Return the pixel width of display (per current rotation)
***************************************************************************************/
// Return the size of the display (per current rotation)
int16_t TFT_ST7735_width(void)
{
  return _width;
}

/***************************************************************************************
** Function name:           height
** Description:             Return the pixel height of display (per current rotation)
***************************************************************************************/
int16_t TFT_ST7735_height(void)
{
  return _height;
}

/***************************************************************************************
** Function name:           TFT_ST7735_textWidth
** Description:             Return the width in pixels of a string in a given font
***************************************************************************************/
int16_t TFT_ST7735_textWidth(char *string, int font)
{
  unsigned int str_width  = 0;
  char uniCode;
  char *widthtable;

  if (font <= 1 || font >= 9)
  {
      /* Default */
      font = 2;
  }

  widthtable = (char *)(fontdata[font].widthtbl - 32); //subtract the 32 outside the loop

  while (*string)
  {
    uniCode = *(string++);
#ifdef LOAD_GLCD
    if (font == 1)
    {
        str_width += 6;
    }
    else
#endif
    {
        str_width += TFT_ST7735_PGM_READ_BYTE(widthtable + uniCode); // Normally we need to substract 32 from uniCode
    }
  }
  return str_width * textsize;
}

/***************************************************************************************
** Function name:           fontsLoaded
** Description:             return an encoded 16 bit value showing the fonts loaded
***************************************************************************************/
// Returns a value showing which fonts are loaded (bit N set =  Font N loaded)

uint16_t TFT_ST7735_fontsLoaded(void)
{
  return fontsloaded;
}


/***************************************************************************************
** Function name:           fontHeight
** Description:             return the height of a font
***************************************************************************************/
int16_t TFT_ST7735_fontHeight(int font)
{
  return (fontdata[font].height * textsize);
}

/***************************************************************************************
** Function name:           TFT_ST7735_drawChar
** Description:             draw a single character in the Adafruit GLCD font
***************************************************************************************/
void TFT_ST7735_drawChar(int16_t x, int16_t y, unsigned char c, uint16_t color, uint16_t bg, uint8_t size)
{
#ifdef LOAD_GLCD
  if ((x >= _width)            || // Clip right
      (y >= _height)           || // Clip bottom
      ((x + 6 * size - 1) < 0) || // Clip left
      ((y + 8 * size - 1) < 0))   // Clip top
    return;
  uint8_t fillbg = (bg != color);

// This is about 5 times faster for textsize=1 with background (at 200us per character)
  if ((size==1) && fillbg)
  {
    uint8_t column[6];
    uint8_t mask = 0x1;
    TFT_ST7735_setWindow(x, y, x+5, y+8);
    for (int8_t i = 0; i < 5; i++ ) column[i] = TFT_ST7735_PGM_READ_BYTE(font + (c * 5) + i);
    column[5] = 0;

    for (int8_t j = 0; j < 8; j++) {
      for (int8_t k = 0; k < 5; k++ ) {
        if (column[k] & mask) {
          TFT_ST7735_WRITE_BYTE_SPI(color >> 8);
          TFT_ST7735_WRITE_BYTE_SPI(color);
        }
        else {
          TFT_ST7735_WRITE_BYTE_SPI(bg >> 8);
          TFT_ST7735_WRITE_BYTE_SPI(bg);
        }
      }

      mask <<= 1;
      TFT_ST7735_WRITE_BYTE_SPI(bg >> 8);
      TFT_ST7735_WRITE_BYTE_SPI(bg);
    }
  }
  else
  {
    if (size == 1) // default size
    {
      for (int8_t i = 0; i < 5; i++ ) {
        uint8_t line = TFT_ST7735_PGM_READ_BYTE(font + c*5 + i);
        if (line & 0x1)  TFT_ST7735_drawPixel(x + i, y, color);
        if (line & 0x2)  TFT_ST7735_drawPixel(x + i, y + 1, color);
        if (line & 0x4)  TFT_ST7735_drawPixel(x + i, y + 2, color);
        if (line & 0x8)  TFT_ST7735_drawPixel(x + i, y + 3, color);
        if (line & 0x10) TFT_ST7735_drawPixel(x + i, y + 4, color);
        if (line & 0x20) TFT_ST7735_drawPixel(x + i, y + 5, color);
        if (line & 0x40) TFT_ST7735_drawPixel(x + i, y + 6, color);
        if (line & 0x80) TFT_ST7735_drawPixel(x + i, y + 7, color);
      }
    }
    else {  // big size
      for (int8_t i = 0; i < 5; i++ ) {
        uint8_t line = TFT_ST7735_PGM_READ_BYTE(font + c*5 + i);
        for (int8_t j = 0; j < 8; j++) {
          if (line & 0x1) TFT_ST7735_fillRect(x + (i * size), y + (j * size), size, size, color);
          else if (fillbg) TFT_ST7735_fillRect(x + i * size, y + j * size, size, size, bg);
          line >>= 1;
        }
      }
    }
  }
#endif // LOAD_GLCD
}

/***************************************************************************************
** Function name:           setAddrWindow
** Description:             define an area to receive a stream of pixels
***************************************************************************************/
// Chip select is high at the end of this function

void TFT_ST7735_setAddrWindow(int16_t x0, int16_t y0, int16_t x1, int16_t y1)
{
    TFT_ST7735_setWindow(x0, y0, x1, y1);
  (void)TFT_ST7735_Set_Chip_Select(CHIP_SELECT_HIGH);
}

/***************************************************************************************
** Function name:           TFT_ST7735_setWindow
** Description:             define an area to receive a stream of pixels
***************************************************************************************/
// Chip select stays low, use setAddrWindow() from sketches

void TFT_ST7735_setWindow(int16_t x0, int16_t y0, int16_t x1, int16_t y1)
{
  addr_row = 0xFF;
  addr_col = 0xFF;

  // Column addr set
  TFT_ST7735_Set_Data_Command(REQUEST_COMMAND);
  TFT_ST7735_Set_Chip_Select(CHIP_SELECT_LOW);

  TFT_ST7735_WRITE_BYTE_SPI(ST7735_CASET);

  TFT_ST7735_Set_Data_Command(REQUEST_DATA);

  TFT_ST7735_WRITE_BYTE_SPI(0);

  TFT_ST7735_WRITE_BYTE_SPI(x0 + colstart);

  TFT_ST7735_WRITE_BYTE_SPI(0);

  TFT_ST7735_WRITE_BYTE_SPI(x1 + colstart);

  // Row addr set
  TFT_ST7735_Set_Data_Command(REQUEST_COMMAND);

  TFT_ST7735_WRITE_BYTE_SPI(ST7735_RASET);

  (void)TFT_ST7735_Set_Data_Command(REQUEST_DATA);

  TFT_ST7735_WRITE_BYTE_SPI(0);

  TFT_ST7735_WRITE_BYTE_SPI(y0 + rowstart);

  TFT_ST7735_WRITE_BYTE_SPI(0);

  TFT_ST7735_WRITE_BYTE_SPI(y1 + rowstart);

  // write to RAM
  TFT_ST7735_Set_Data_Command(REQUEST_COMMAND);

  TFT_ST7735_WRITE_BYTE_SPI(ST7735_RAMWR);

  (void)TFT_ST7735_Set_Data_Command(REQUEST_DATA);
}

/***************************************************************************************
** Function name:           TFT_ST7735_drawPixel
** Description:             push a single pixel at an arbitrary position
***************************************************************************************/
// Smarter version that takes advantage of often used orthogonal coordinate plots
// where either x or y does not change
void TFT_ST7735_drawPixel(uint16_t x, uint16_t y, uint16_t color)
{
    // Faster range checking, possible because x and y are unsigned
    if ((x >= _width) || (y >= _height)) return;

    TFT_ST7735_Set_Data_Command(REQUEST_COMMAND);
    TFT_ST7735_Set_Chip_Select(CHIP_SELECT_LOW);

    if (addr_col != x) {
        TFT_ST7735_WRITE_BYTE_SPI(ST7735_CASET);

        addr_col = x;
        (void)TFT_ST7735_Set_Data_Command(REQUEST_DATA);

        TFT_ST7735_WRITE_BYTE_SPI(0);

        TFT_ST7735_WRITE_BYTE_SPI(x + colstart);

        TFT_ST7735_Set_Data_Command(REQUEST_COMMAND);
    }

    if (addr_row != y) {
        TFT_ST7735_WRITE_BYTE_SPI(ST7735_RASET);

        addr_row = y;
        (void)TFT_ST7735_Set_Data_Command(REQUEST_DATA);

        TFT_ST7735_WRITE_BYTE_SPI(0);

        TFT_ST7735_WRITE_BYTE_SPI(y + rowstart);

        TFT_ST7735_Set_Data_Command(REQUEST_COMMAND);
    }

    TFT_ST7735_WRITE_BYTE_SPI(ST7735_RAMWR);

    (void)TFT_ST7735_Set_Data_Command(REQUEST_DATA);

    TFT_ST7735_WRITE_BYTE_SPI(color >> 8);

    TFT_ST7735_WRITE_BYTE_SPI(color);

    (void)TFT_ST7735_Set_Chip_Select(CHIP_SELECT_HIGH);
}

/***************************************************************************************
** Function name:           pushColor
** Description:             push a single pixel
***************************************************************************************/
void TFT_ST7735_pushColor(uint16_t color)
{
  TFT_ST7735_Set_Chip_Select(CHIP_SELECT_LOW);

  TFT_ST7735_WRITE_BYTE_SPI(color >> 8);
  TFT_ST7735_WRITE_BYTE_SPI(color);

  TFT_ST7735_Set_Chip_Select(CHIP_SELECT_HIGH);
}

/***************************************************************************************
** Function name:           pushColor
** Description:             push a single colour to "len" pixels
***************************************************************************************/
void TFT_ST7735_pushColor_len(uint16_t color, uint16_t len)
{
  int i;
  TFT_ST7735_Set_Chip_Select(CHIP_SELECT_LOW);

  for(i = 0; i < len; i++)
  {
      TFT_ST7735_WRITE_BYTE_SPI(color >> 8);
      TFT_ST7735_WRITE_BYTE_SPI(color);
  }

  (void)TFT_ST7735_Set_Chip_Select(CHIP_SELECT_HIGH);
}

/***************************************************************************************
** Function name:           pushColors
** Description:             push an aray of pixels for BMP image drawing
***************************************************************************************/
// Sends an array of 16-bit color values to the TFT; used
// externally by BMP examples.  Assumes that setAddrWindow() has
// previously been called to define the bounds.  Max 255 pixels at
// a time (BMP examples read in small chunks due to limited RAM).

void TFT_ST7735_pushColors(uint16_t *data, uint8_t len)
{
  uint16_t color;

  TFT_ST7735_Set_Chip_Select(CHIP_SELECT_LOW);

  while (len--) {
    color = *(data++);
    TFT_ST7735_WRITE_BYTE_SPI(color >> 8);
    TFT_ST7735_WRITE_BYTE_SPI(color);
  }

  (void)TFT_ST7735_Set_Chip_Select(CHIP_SELECT_HIGH);
}

/***************************************************************************************
** Function name:           pushColors
** Description:             push an array of pixels for 16 bit raw image drawing
***************************************************************************************/
// Assumed that setAddrWindow() has previously been called

void TFT_ST7735_pushColors_8(uint8_t *data, uint16_t len)
{
  len = len << 1;

  TFT_ST7735_Set_Chip_Select(CHIP_SELECT_LOW);
  while (len--) {
      TFT_ST7735_WRITE_BYTE_SPI(*(data++));
  }
  (void)TFT_ST7735_Set_Chip_Select(CHIP_SELECT_HIGH);
}

/***************************************************************************************
** Function name:           TFT_ST7735_drawLine
** Description:             draw a line between 2 arbitrary points
***************************************************************************************/

// Bresenham's algorithm - thx wikipedia - speed enhanced by Bodmer to use
// an eficient FastH/V Line draw routine for line segments of 2 pixels or more
// enhanced further using code from Xark and Spellbuilder (116 byte penalty)

// Select which version, fastest or compact
#ifdef FAST_LINE

void TFT_ST7735_drawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color)
{
  int8_t steep = abs(y1 - y0) > abs(x1 - x0);

  if (steep) {
    tftswap(&x0, &y0);
    tftswap(&x1, &y1);
  }

  if (x0 > x1) {
    tftswap(&x0, &x1);
    tftswap(&y0, &y1);
  }

  if (x1 < 0) return;

  int16_t dx, dy;
  dx = x1 - x0;
  dy = abs(y1 - y0);

  int16_t err = dx / 2;
  int8_t ystep = (y0 < y1) ? 1 : (-1);

  if (steep)  // y increments every iteration (y0 is x-axis, and x0 is y-axis)
  {
    if (x1 >= _height) x1 = _height - 1;

    for (; x0 <= x1; x0++) {
    if ((x0 >= 0) && (y0 >= 0) && (y0 < _width)) break;
    err -= dy;
    if (err < 0) {
      err += dx;
      y0 += ystep;
    }
    }

    if (x0 > x1) return;

    TFT_ST7735_setWindow(y0, x0, y0, _height);
    for (; x0 <= x1; x0++) {
      TFT_ST7735_WRITE_BYTE_SPI(color >> 8);
      TFT_ST7735_WRITE_BYTE_SPI(color);
      err -= dy;
      if (err < 0) {
        y0 += ystep;
        if ((y0 < 0) || (y0 >= _width)) break;
        err += dx;
           //while (!(SPSR & _BV(SPIF))); // Safe, but can comment out and rely on delay
                 TFT_ST7735_setWindow(y0, x0+1, y0, _height);
      }
    }
  }
  else  // x increments every iteration (x0 is x-axis, and y0 is y-axis)
  {
    if (x1 >= _width) x1 = _width - 1;

    for (; x0 <= x1; x0++) {
    if ((x0 >= 0) && (y0 >= 0) && (y0 < _height)) break;
    err -= dy;
    if (err < 0) {
      err += dx;
      y0 += ystep;
    }
    }

    if (x0 > x1) return;

           TFT_ST7735_setWindow(x0, y0, _width, y0);
    for (; x0 <= x1; x0++) {
        TFT_ST7735_WRITE_BYTE_SPI(color >> 8);
        TFT_ST7735_WRITE_BYTE_SPI(color);
      err -= dy;
      if (err < 0) {
        y0 += ystep;
        if ((y0 < 0) || (y0 >= _height)) break;
        err += dx;
           //while (!(SPSR & _BV(SPIF))); // Safe, but can comment out and rely on delay
                     TFT_ST7735_setWindow(x0+1, y0, _width, y0);
      }
    }
  }
      (void)TFT_ST7735_Set_Chip_Select(CHIP_SELECT_HIGH);
}

#else // FAST_LINE not defined so use more compact version

// Slower but more compact line drawing function
void TFT_ST7735_drawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color)
{
  int8_t steep = abs(y1 - y0) > abs(x1 - x0);
  if (steep) {
    tftswap(&x0, &y0);
    tftswap(&x1, &y1);
  }

  if (x0 > x1) {
    tftswap(&x0, &x1);
    tftswap(&y0, &y1);
  }

  int16_t dx = x1 - x0, dy = abs(y1 - y0);


  int16_t err = dx >> 1, ystep = -1, xs = x0, dlen = 0;
  if (y0 < y1) ystep = 1;

  // Split into steep and not steep for FastH/V separation
  if (steep) {
    for (; x0 <= x1; x0++) {
      dlen++;
      err -= dy;
      if (err < 0) {
        err += dx;
        if (dlen == 1) TFT_ST7735_drawPixel(y0, xs, color);
        else TFT_ST7735_drawFastVLine(y0, xs, dlen, color);
        dlen = 0; y0 += ystep; xs = x0 + 1;
      }
    }
    if (dlen) TFT_ST7735_drawFastVLine(y0, xs, dlen, color);
  }
  else
  {
    for (; x0 <= x1; x0++) {
      dlen++;
      err -= dy;
      if (err < 0) {
        err += dx;
        if (dlen == 1) TFT_ST7735_drawPixel(xs, y0, color);
        else TFT_ST7735_drawFastHLine(xs, y0, dlen, color);
        dlen = 0; y0 += ystep; xs = x0 + 1;
      }
    }
    if (dlen) TFT_ST7735_drawFastHLine(xs, y0, dlen, color);
  }
}

#endif // FAST_LINE option

/***************************************************************************************
** Function name:           TFT_ST7735_drawFastVLine
** Description:             draw a vertical line
***************************************************************************************/
void TFT_ST7735_drawFastVLine(int16_t x, int16_t y, int16_t h, uint16_t color)
{
#ifdef CLIP_CHECK
  // Rudimentary clipping
  if ((x >= _width) || (y >= _height)) return;
  if ((y + h - 1) >= _height) h = _height - y;
#endif

  TFT_ST7735_setWindow(x, y, x, _height);

  while(h != 0)
  {
      TFT_ST7735_WRITE_BYTE_SPI(color >> 8);
      TFT_ST7735_WRITE_BYTE_SPI(color);

      h--;
  }

  (void)TFT_ST7735_Set_Chip_Select(CHIP_SELECT_HIGH);
}

/***************************************************************************************
** Function name:           TFT_ST7735_drawFastHLine
** Description:             draw a horizontal line
***************************************************************************************/
void TFT_ST7735_drawFastHLine(int16_t x, int16_t y, int16_t w, uint16_t color)
{
#ifdef CLIP_CHECK
  // Rudimentary clipping
  if ((x >= _width) || (y >= _height)) return;
  if ((x + w - 1) >= _width)  w = _width - x;
#endif

  TFT_ST7735_setWindow(x, y, _width, y);

  while(w != 0)
  {
      TFT_ST7735_WRITE_BYTE_SPI(color >> 8);
      TFT_ST7735_WRITE_BYTE_SPI(color);

      w--;
  }

  (void)TFT_ST7735_Set_Chip_Select(CHIP_SELECT_HIGH);
}

/***************************************************************************************
** Function name:           TFT_ST7735_fillRect
** Description:             draw a filled rectangle
***************************************************************************************/
void TFT_ST7735_fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color)
{
#ifdef CLIP_CHECK
  // rudimentary clipping (TFT_ST7735_drawChar w/big text requires this)
  if ((x > _width) || (y > _height) || (w==0) || (h==0)) return;
  if ((x + w - 1) > _width)  w = _width  - x;
  if ((y + h - 1) > _height) h = _height - y;
#endif

  TFT_ST7735_setWindow(x, y, x + w - 1, y + h - 1);

  if (h > w) tftswap(&h, &w);

  for(int count_h = 0; count_h < h; count_h++)
  {
      for(int count_w = 0; count_w < w; count_w++)
      {
          TFT_ST7735_WRITE_BYTE_SPI(color >> 8);
          TFT_ST7735_WRITE_BYTE_SPI(color);
      }
  }

  (void)TFT_ST7735_Set_Chip_Select(CHIP_SELECT_HIGH);
}

/***************************************************************************************
** Function name:           color565
** Description:             convert three 8 bit RGB levels to a 16 bit colour value
***************************************************************************************/
uint16_t TFT_ST7735_color565(uint8_t r, uint8_t g, uint8_t b)
{
  return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
}

/***************************************************************************************
** Function name:           setRotation
** Description:             rotate the screen orientation m = 0-3 or 4-7 for BMP drawing
***************************************************************************************/

void TFT_ST7735_setRotation(uint8_t m)
{
  addr_row = 0xFF;
  addr_col = 0xFF;

  rotation = m % 4;

  TFT_ST7735_writecommand(ST7735_MADCTL);
  switch (rotation) {
    case 0:
     if (tabcolor == INITR_BLACKTAB) {
       TFT_ST7735_writedata(MADCTL_MX | MADCTL_MY | MADCTL_RGB);
     } else if(tabcolor == INITR_GREENTAB2) {
       TFT_ST7735_writedata(MADCTL_MX | MADCTL_MY | MADCTL_RGB);
       colstart = 2;
       rowstart = 1;
     } else {
       TFT_ST7735_writedata(MADCTL_MX | MADCTL_MY | MADCTL_BGR);
     }
      _width  = ST7735_TFTWIDTH;
      _height = ST7735_TFTHEIGHT;
      break;
    case 1:
     if (tabcolor == INITR_BLACKTAB) {
       TFT_ST7735_writedata(MADCTL_MY | MADCTL_MV | MADCTL_RGB);
     } else if(tabcolor == INITR_GREENTAB2) {
       TFT_ST7735_writedata(MADCTL_MY | MADCTL_MV | MADCTL_RGB);
       colstart = 1;
       rowstart = 2;
     } else {
       TFT_ST7735_writedata(MADCTL_MY | MADCTL_MV | MADCTL_BGR);
     }
      _width  = ST7735_TFTHEIGHT;
      _height = ST7735_TFTWIDTH;
      break;
    case 2:
     if (tabcolor == INITR_BLACKTAB) {
       TFT_ST7735_writedata(MADCTL_RGB);
     } else if(tabcolor == INITR_GREENTAB2) {
       TFT_ST7735_writedata(MADCTL_RGB);
       colstart = 2;
       rowstart = 1;
     } else {
       TFT_ST7735_writedata(MADCTL_BGR);
     }
      _width  = ST7735_TFTWIDTH;
      _height = ST7735_TFTHEIGHT;
      break;
    case 3:
     if (tabcolor == INITR_BLACKTAB) {
       TFT_ST7735_writedata(MADCTL_MX | MADCTL_MV | MADCTL_RGB);
     } else if(tabcolor == INITR_GREENTAB2) {
       TFT_ST7735_writedata(MADCTL_MX | MADCTL_MV | MADCTL_RGB);
       colstart = 1;
       rowstart = 2;
     } else {
       TFT_ST7735_writedata(MADCTL_MX | MADCTL_MV | MADCTL_BGR);
     }
      _width  = ST7735_TFTHEIGHT;
      _height = ST7735_TFTWIDTH;
      break;

  // These next rotations are for bottom up BMP drawing
  /*  case 4:
      TFT_ST7735_writedata(ST7735_MADCTL_MX | ST7735_MADCTL_MY | ST7735_MADCTL_BGR);
      _width  = ST7735_TFTWIDTH;
      _height = ST7735_TFTHEIGHT;
      break;
    case 5:
      TFT_ST7735_writedata(ST7735_MADCTL_MV | ST7735_MADCTL_MX | ST7735_MADCTL_BGR);
      _width  = ST7735_TFTHEIGHT;
      _height = ST7735_TFTWIDTH;
      break;
    case 6:
      TFT_ST7735_writedata(ST7735_MADCTL_BGR);
      _width  = ST7735_TFTWIDTH;
      _height = ST7735_TFTHEIGHT;
      break;
    case 7:
      TFT_ST7735_writedata(ST7735_MADCTL_MY | ST7735_MADCTL_MV | ST7735_MADCTL_BGR);
      _width  = ST7735_TFTHEIGHT;
      _height = ST7735_TFTWIDTH;
      break;
  */
  }
}

/***************************************************************************************
** Function name:           invertDisplay
** Description:             invert the display colours i = 1 invert, i = 0 normal
***************************************************************************************/
void TFT_ST7735_invertDisplay(unsigned char i)
{
  // Send the command twice as otherwise it does not always work!
  TFT_ST7735_writecommand(i ? ST7735_INVON : ST7735_INVOFF);
  TFT_ST7735_writecommand(i ? ST7735_INVON : ST7735_INVOFF);
}

/***************************************************************************************
** Function name:           write
** Description:             draw characters piped through serial stream
***************************************************************************************/
size_t TFT_ST7735_write(uint8_t uniCode)
{
  if (uniCode == '\r') return 1;
  unsigned int width = 0;
  unsigned int height = 0;
  //Serial.print((char) uniCode); // Debug line sends all printed TFT text to serial port

#ifdef LOAD_FONT2
  if (textfont == 2)
  {
      // This is 20us faster than using the fontdata structure (0.443ms per character instead of 0.465ms)
      width = TFT_ST7735_PGM_READ_BYTE(widtbl_f16 + uniCode-32);
      height = chr_hgt_f16;
      // Font 2 is rendered in whole byte widths so we must allow for this
      width = (width + 6) / 8;  // Width in whole bytes for font 2, should be + 7 but must allow for font width change
      width = width * 8;        // Width converted back to pixles
  }
  #ifdef LOAD_RLE
  else
  #endif
#endif


#ifdef LOAD_RLE
  {
      // Uses the fontinfo struct array to avoid lots of 'if' or 'switch' statements
      // A tad slower than above but this is not significant and is more convenient for the RLE fonts
      // Yes, this code can be needlessly executed when textfont == 1...
      width = TFT_ST7735_PGM_READ_BYTE(fontdata[textfont].widthtbl + uniCode - 32);
      height = fontdata[textfont].height;
  }
#endif

#ifdef LOAD_GLCD
  if (textfont==1)
  {
      width =  6;
      height = 8;
  }
#else
  if (textfont==1) return 0;
#endif

  height = height * textsize;

  if (uniCode == '\n') {
    cursor_y += height;
    cursor_x  = 0;
  }
  else
  {
    if (textwrap && (cursor_x + width * textsize >= (uint16_t)_width))
    {
      cursor_y += height;
      cursor_x = 0;
    }
    cursor_x += TFT_ST7735_drawChar_uniCode(uniCode, cursor_x, cursor_y, textfont);
  }
  return 1;
}

/***************************************************************************************
** Function name:           TFT_ST7735_drawChar
** Description:             draw a unicode onto the screen
***************************************************************************************/
int TFT_ST7735_drawChar_uniCode(unsigned int uniCode, int x, int y, int font)
{

  if (font==1)
  {
#ifdef LOAD_GLCD
      TFT_ST7735_drawChar(x, y, uniCode, textcolor, textbgcolor, textsize);
      return 6 * textsize;
#else
      return 0;
#endif
  }

  unsigned int width  = 0;
  unsigned int height = 0;
  const uint8_t* flash_address = 0;
  uniCode -= 32;

#ifdef LOAD_FONT2
  if (font == 2)
  {
      flash_address = chrtbl_f16[uniCode];
      width = TFT_ST7735_PGM_READ_BYTE(widtbl_f16 + uniCode);
      height = chr_hgt_f16;
  }
  #ifdef LOAD_RLE
  else
  #endif
#endif

#ifdef LOAD_RLE
  {
      flash_address = fontdata[font].chartbl + uniCode;
      width = TFT_ST7735_PGM_READ_BYTE(fontdata[font].widthtbl + uniCode);
      height = fontdata[font].height;
  }
#endif

  int w = width;
  int pX      = 0;
  int pY      = y;
  uint8_t line = 0;

  uint8_t tl = textcolor;
  uint8_t th = textcolor >> 8;
  uint8_t bl = textbgcolor;
  uint8_t bh = textbgcolor >> 8;

#ifdef LOAD_FONT2
  if (font == 2) {
    w = w + 6; // Should be + 7 but we need to compensate for width increment
    w = w / 8;
    if (x + width * textsize >= _width) return width * textsize ;

    if (textcolor == textbgcolor || textsize != 1) {

      for (unsigned int i = 0; i < height; i++)
      {
        if (textcolor != textbgcolor) TFT_ST7735_fillRect(x, pY, width * textsize, textsize, textbgcolor);

        for (int k = 0; k < w; k++)
        {
          line = TFT_ST7735_PGM_READ_BYTE(flash_address + (w * i) + k);
          if (line) {
            if (textsize == 1) {
              pX = x + k * 8;
              if (line & 0x80) TFT_ST7735_drawPixel(pX, pY, textcolor);
              if (line & 0x40) TFT_ST7735_drawPixel(pX + 1, pY, textcolor);
              if (line & 0x20) TFT_ST7735_drawPixel(pX + 2, pY, textcolor);
              if (line & 0x10) TFT_ST7735_drawPixel(pX + 3, pY, textcolor);
              if (line & 0x08) TFT_ST7735_drawPixel(pX + 4, pY, textcolor);
              if (line & 0x04) TFT_ST7735_drawPixel(pX + 5, pY, textcolor);
              if (line & 0x02) TFT_ST7735_drawPixel(pX + 6, pY, textcolor);
              if (line & 0x01) TFT_ST7735_drawPixel(pX + 7, pY, textcolor);
            }
            else {
              pX = x + k * 8 * textsize;
              if (line & 0x80) TFT_ST7735_fillRect(pX, pY, textsize, textsize, textcolor);
              if (line & 0x40) TFT_ST7735_fillRect(pX + textsize, pY, textsize, textsize, textcolor);
              if (line & 0x20) TFT_ST7735_fillRect(pX + 2 * textsize, pY, textsize, textsize, textcolor);
              if (line & 0x10) TFT_ST7735_fillRect(pX + 3 * textsize, pY, textsize, textsize, textcolor);
              if (line & 0x08) TFT_ST7735_fillRect(pX + 4 * textsize, pY, textsize, textsize, textcolor);
              if (line & 0x04) TFT_ST7735_fillRect(pX + 5 * textsize, pY, textsize, textsize, textcolor);
              if (line & 0x02) TFT_ST7735_fillRect(pX + 6 * textsize, pY, textsize, textsize, textcolor);
              if (line & 0x01) TFT_ST7735_fillRect(pX + 7 * textsize, pY, textsize, textsize, textcolor);
            }
          }
        }
        pY += textsize;
      }
    }
    else
      // Faster drawing of characters and background using block write
    {

      TFT_ST7735_setWindow(x, y, (x + w * 8) - 1, y + height - 1);

      uint8_t mask;
      for (unsigned int i = 0; i < height; i++)
      {
        for (int k = 0; k < w; k++)
        {
          line = TFT_ST7735_PGM_READ_BYTE(flash_address + w * i + k);
          pX = x + k * 8;
          mask = 0x80;
          while (mask) {
            if (line & mask) {
                TFT_ST7735_WRITE_BYTE_SPI(th);
                TFT_ST7735_WRITE_BYTE_SPI(tl);
            }
            else {
                TFT_ST7735_WRITE_BYTE_SPI(bh);
                TFT_ST7735_WRITE_BYTE_SPI(bl);
            }
            mask = mask >> 1;
          }
        }
        pY += textsize;
      }
      TFT_ST7735_writeEnd();
    }
  }

  #ifdef LOAD_RLE
  else
  #endif
#endif  //FONT2

#ifdef LOAD_RLE  //674 bytes of code
  // Font is not 2 and hence is RLE encoded
  {
      // Dummy write to ensure SPIF flag gets set for first check in while() loop
      TFT_ST7735_WRITE_BYTE_SPI(0);

    w *= height; // Now w is total number of pixels in the character
    if ((textsize != 1) || (textcolor == textbgcolor)) {
      if (textcolor != textbgcolor) TFT_ST7735_fillRect(x, pY, width * textsize, textsize * height, textbgcolor);
      int px = 0, py = pY; // To hold character block start and end column and row values
      int pc = 0; // Pixel count
      uint8_t np = textsize * textsize; // Number of pixels in a drawn pixel

      uint8_t tnp = 0; // Temporary copy of np for while loop
      uint8_t ts = textsize - 1; // Temporary copy of textsize
      // 16 bit pixel count so maximum font size is equivalent to 180x180 pixels in area
      // w is total number of pixels to plot to fill character block
      while (pc < w)
      {
        line = TFT_ST7735_PGM_READ_BYTE((void**)flash_address);
        flash_address++; // 20 bytes smaller by incrementing here
        if (line & 0x80) {
          line &= 0x7F;
          line++;
          if (ts) {
            px = x + textsize * (pc % width); // Keep these px and py calculations outside the loop as they are slow
            py = y + textsize * (pc / width);
          }
          else {
            px = x + pc % width; // Keep these px and py calculations outside the loop as they are slow
            py = y + pc / width;
          }
          while (line--) { // In this case the while(line--) is faster
            pc++; // This is faster than putting pc+=line before while() as we use up SPI wait time

            TFT_ST7735_setWindow(px, py, px + ts, py + ts);

            if (ts) {
              tnp = np;
              while (tnp--) {
                  TFT_ST7735_WRITE_BYTE_SPI(th);
                  TFT_ST7735_WRITE_BYTE_SPI(tl);
              }
            }
            else {
                TFT_ST7735_WRITE_BYTE_SPI(th);
                TFT_ST7735_WRITE_BYTE_SPI(tl);
            }
            px += textsize;

            if (px >= (x + width * textsize))
            {
              px = x;
              py += textsize;
            }
          }
        }
        else {
          line++;
          pc += line;
        }
      }
      TFT_ST7735_writeEnd();
    }
    else // Text colour != background && textsize = 1
         // so use faster drawing of characters and background using block write
    {

      TFT_ST7735_setWindow(x, y, x + width - 1, y + height - 1);

      // Maximum font size is equivalent to 180x180 pixels in area
      while (w > 0)
      {
        line = TFT_ST7735_PGM_READ_BYTE((uint8_t*)(flash_address++)); // 8 bytes smaller when incrementing here
        if (line & 0x80) {
          line &= 0x7F;
          line++; w -= line;
          while(line--)
          {
              TFT_ST7735_WRITE_BYTE_SPI(textcolor >> 8);
              TFT_ST7735_WRITE_BYTE_SPI(textcolor);
          }
        }
        else {
          line++; w -= line;
          while(line--)
          {
              TFT_ST7735_WRITE_BYTE_SPI(textbgcolor >> 8);
              TFT_ST7735_WRITE_BYTE_SPI(textbgcolor);
          }
        }
      }
      TFT_ST7735_writeEnd();
    }
  }
  // End of RLE font rendering
#endif
  return width * textsize;    // x +
}

/***************************************************************************************
** Function name:           TFT_ST7735_drawString
** Description :            draw string with padding if it is defined
***************************************************************************************/
int TFT_ST7735_drawString(char *string, int poX, int poY, int font)
{
  int16_t sumX = 0;
  uint8_t padding = 1;
  unsigned int cheight = 0;

  if (textdatum || padX)
  {
    // Find the pixel width of the string in the font
    unsigned int cwidth  = TFT_ST7735_textWidth(string, font);

    // Get the pixel height of the font
    cheight = TFT_ST7735_PGM_READ_BYTE( &fontdata[font].height ) * textsize;

    switch(textdatum) {
      case TC_DATUM:
        poX -= cwidth/2;
        padding = 2;
        break;
      case TR_DATUM:
        poX -= cwidth;
        padding = 3;
        break;
      case ML_DATUM:
        poY -= cheight/2;
        padding = 1;
        break;
      case MC_DATUM:
        poX -= cwidth/2;
        poY -= cheight/2;
        padding = 2;
        break;
      case MR_DATUM:
        poX -= cwidth;
        poY -= cheight/2;
        padding = 3;
        break;
      case BL_DATUM:
        poY -= cheight;
        padding = 1;
        break;
      case BC_DATUM:
        poX -= cwidth/2;
        poY -= cheight;
        padding = 2;
        break;
      case BR_DATUM:
        poX -= cwidth;
        poY -= cheight;
        padding = 3;
        break;
    }
    // Check coordinates are OK, adjust if not
    if (poX < 0) poX = 0;
    if (poX+cwidth>_width)   poX = _width - cwidth;
    if (poY < 0) poY = 0;
    if (poY+cheight>_height) poY = _height - cheight;
  }

  while (*string) sumX += TFT_ST7735_drawChar_uniCode(*(string++), poX+sumX, poY, font);

//#define PADDING_DEBUG

#ifndef PADDING_DEBUG
  if((padX>sumX) && (textcolor!=textbgcolor))
  {
    int padXc = poX+sumX; // Maximum left side padding
    switch(padding) {
      case 1:
        TFT_ST7735_fillRect(padXc,poY,padX-sumX,cheight, textbgcolor);
        break;
      case 2:
        TFT_ST7735_fillRect(padXc,poY,(padX-sumX)>>1,cheight, textbgcolor);
        padXc = (padX-sumX)>>1;
        if (padXc>poX) padXc = poX;
        TFT_ST7735_fillRect(poX - padXc,poY,(padX-sumX)>>1,cheight, textbgcolor);
        break;
      case 3:
        if (padXc>padX) padXc = padX;
        TFT_ST7735_fillRect(poX + sumX - padXc,poY,padXc-sumX,cheight, textbgcolor);
        break;
    }
  }
#else

  // This is debug code to show text (green box) and blanked (white box) areas
  // to show that the padding areas are being correctly sized and positioned
  if((padX>sumX) && (textcolor!=textbgcolor))
  {
    int padXc = poX+sumX; // Maximum left side padding
    drawRect(poX,poY,sumX,cheight, TFT_GREEN);
    switch(padding) {
      case 1:
        drawRect(padXc,poY,padX-sumX,cheight, TFT_WHITE);
        break;
      case 2:
        drawRect(padXc,poY,(padX-sumX)>>1, cheight, TFT_WHITE);
        padXc = (padX-sumX)>>1;
        if (padXc>poX) padXc = poX;
        drawRect(poX - padXc,poY,(padX-sumX)>>1,cheight, TFT_WHITE);
        break;
      case 3:
        if (padXc>padX) padXc = padX;
        drawRect(poX + sumX - padXc,poY,padXc-sumX,cheight, TFT_WHITE);
        break;
    }
  }
#endif

return sumX;
}

/***************************************************************************************
** Function name:           drawCentreString
** Descriptions:            draw string centred on dX
***************************************************************************************/
int TFT_ST7735_drawCentreString(char *string, int dX, int poY, int font)
{
  unsigned char tempdatum = textdatum;
  int sumX = 0;
  textdatum = TC_DATUM;
  sumX = TFT_ST7735_drawString(string, dX, poY, font);
  textdatum = tempdatum;
  return sumX;
}

/***************************************************************************************
** Function name:           drawRightString
** Descriptions:            draw string right justified to dX
***************************************************************************************/
int TFT_ST7735_drawRightString(char *string, int dX, int poY, int font)
{
  unsigned char tempdatum = textdatum;
  int sumX = 0;
  textdatum = TR_DATUM;
  sumX = TFT_ST7735_drawString(string, dX, poY, font);
  textdatum = tempdatum;
  return sumX;
}

/***************************************************************************************
** Function name:           drawNumber
** Description:             draw a long integer
***************************************************************************************/
int TFT_ST7735_drawNumber(long long_num, int poX, int poY, int font)
{
  char str[12];
  TFT_ST7735_ltoa(long_num, str, 10);
  return TFT_ST7735_drawString(str, poX, poY, font);
}

/***************************************************************************************
** Function name:           drawFloat
** Descriptions:            drawFloat, prints 7 non zero digits maximum
***************************************************************************************/
// Adapted to assemble and print a string, this permits alignment relative to a datum
// looks complicated but much more compact and actually faster than using print class
int TFT_ST7735_drawFloat(float floatNumber, int dp, int poX, int poY, int font)
{
  char str[14];               // Array to contain decimal string
  uint8_t ptr = 0;            // Initialise pointer for array
  int8_t  digits = 1;         // Count the digits to avoid array overflow
  float rounding = 0.5;       // Round up down delta

  if (dp > 7) dp = 7; // Limit the size of decimal portion

  // Adjust the rounding value
  for (uint8_t i = 0; i < dp; ++i) rounding /= 10.0;

  if (floatNumber < -rounding)    // add sign, avoid adding - sign to 0.0!
  {
    str[ptr++] = '-'; // Negative number
    str[ptr] = 0; // Put a null in the array as a precaution
    digits = 0;   // Set digits to 0 to compensate so pointer value can be used later
    floatNumber = -floatNumber; // Make positive
  }

  floatNumber += rounding; // Round up or down

  // For error put ... in string and return (all TFT_ST7735 library fonts contain . character)
  if (floatNumber >= 2147483647) {
    strcpy(str, "...");
    return TFT_ST7735_drawString(str, poX, poY, font);
  }
  // No chance of overflow from here on

  // Get integer part
  unsigned long temp = (unsigned long)floatNumber;

  // Put integer part into array
  TFT_ST7735_ltoa(temp, str + ptr, 10);

  // Find out where the null is to get the digit count loaded
  while ((uint8_t)str[ptr] != 0) ptr++; // Move the pointer along
  digits += ptr;                  // Count the digits

  str[ptr++] = '.'; // Add decimal point
  str[ptr] = '0';   // Add a dummy zero
  str[ptr + 1] = 0; // Add a null but don't increment pointer so it can be overwritten

  // Get the decimal portion
  floatNumber = floatNumber - temp;

  // Get decimal digits one by one and put in array
  // Limit digit count so we don't get a false sense of resolution
  uint8_t i = 0;
  while ((i < dp) && (digits < 9)) // while (i < dp) for no limit but array size must be increased
  {
    i++;
    floatNumber *= 10;       // for the next decimal
    temp = floatNumber;      // get the decimal
    TFT_ST7735_ltoa(temp, str + ptr, 10);
    ptr++; digits++;         // Increment pointer and digits count
    floatNumber -= temp;     // Remove that digit
  }
  
  // Finally we can plot the string and return pixel length
  return TFT_ST7735_drawString(str, poX, poY, font);
}

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

 ****************************************************/
