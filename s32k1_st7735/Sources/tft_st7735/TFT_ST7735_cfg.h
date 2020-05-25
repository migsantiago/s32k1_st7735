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

#ifndef TST_ST7735_CFG_H
#define TST_ST7735_CFG_H

//////////////////////////////////////////////////////////////////////////////
/// Define special typedefs here (uint8_t, uint32_t, etc.)
//////////////////////////////////////////////////////////////////////////////
#include <stdint.h>

//                            USER DEFINED SETTINGS V16
//            Set fonts to be loaded, pins used and SPI control method

// Define the type of display from the colour of the tab on the screen protector
// Comment out all but one of these options

//#define TAB_COLOUR INITB
//#define TAB_COLOUR INITR_GREENTAB
//#define TAB_COLOUR INITR_REDTAB
//#define TAB_COLOUR INITR_BLACKTAB
#define TAB_COLOUR INITR_BLACKTAB

/**
 * Delays for the reset sequence
 */
#define TFT_ST7735_FIRST_RESET_HIGH_DELAY                   (10)
#define TFT_ST7735_SECOND_RESET_LOW_DELAY                   (10)
#define TFT_ST7735_THIRD_RESET_HIGH_DELAY                   (150)

// ##################################################################################
//
// Define the fonts that are to be used here
//
// ##################################################################################

// Comment out the #defines below with // to stop that font being loaded
// As supplied font 8 is disabled by commenting out
//
// If all fonts are loaded the extra FLASH space required is about 17000 bytes...
// To save FLASH space only enable the fonts you need!

#define LOAD_GLCD   // Font 1. Original Adafruit 8 pixel font needs ~1820 bytes in FLASH
#define LOAD_FONT2  // Font 2. Small 16 pixel high font, needs ~3534 bytes in FLASH, 96 characters
#define LOAD_FONT4  // Font 4. Medium 26 pixel high font, needs ~5848 bytes in FLASH, 96 characters
#define LOAD_FONT6  // Font 6. Large 48 pixel font, needs ~2666 bytes in FLASH, only characters 1234567890:-.apm
#define LOAD_FONT7  // Font 7. 7 segment 48 pixel font, needs ~2438 bytes in FLASH, only characters 1234567890:.
#define LOAD_FONT8  // Font 8. Large 75 pixel font needs ~3256 bytes in FLASH, only characters 1234567890:-.

// ##################################################################################
//
// Other speed up options
//
// ##################################################################################

// Uncomment the following #define to invoke a 20% faster drawLine() function
// This speeds up other funtions such as triangle outline drawing too
// Code size penalty is about 72 bytes

#define FAST_LINE

// Comment out the following #define to stop boundary checking and clipping
// for fillRectangle()and fastH/V lines. This speeds up other funtions such as text
// rendering where size>1. Sketch then must not draw graphics/text outside screen
// boundary. Code saving for no bounds check (i.e. commented out) is 316 bytes

//#define CLIP_CHECK

#endif /* #ifndef TST_ST7735_CFG_H */
