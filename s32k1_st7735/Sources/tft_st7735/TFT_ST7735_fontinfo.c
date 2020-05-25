/***************************************************
  DESCRIPTION

  TFT graphics library for any uC or SoC with open
  implementations on SPI callbacks.

  No specific HW implementations are available.

  This means: no dependency on Arduino, yay!

  New implementation was brought back from C++ to C
  for more compatibility on more environments.

  Code cleanup and reordering was also applied.

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

#include "TFT_ST7735.h"

// This is a structure to conveniently hold information on the fonts
// Stores font character image address pointer, width table and height

const fontinfo fontdata [] = {
   { 0, 0, 0 },

   { 0, 0, 8 },

  #ifdef LOAD_FONT2
   { (const unsigned char *)chrtbl_f16, widtbl_f16, chr_hgt_f16},
  #else
   { 0, 0, 0 },
  #endif

   { 0, 0, 0 },

  #ifdef LOAD_FONT4
   { (const unsigned char *)chrtbl_f32, widtbl_f32, chr_hgt_f32},
  #else
   { 0, 0, 0 },
  #endif

   { 0, 0, 0 },

  #ifdef LOAD_FONT6
   { (const unsigned char *)chrtbl_f64, widtbl_f64, chr_hgt_f64},
  #else
   { 0, 0, 0 },
  #endif

  #ifdef LOAD_FONT7
   { (const unsigned char *)chrtbl_f7s, widtbl_f7s, chr_hgt_f7s},
  #else
   { 0, 0, 0 },
  #endif

  #ifdef LOAD_FONT8
   { (const unsigned char *)chrtbl_f72, widtbl_f72, chr_hgt_f72}
  #else
   { 0, 0, 0 }
  #endif
};
