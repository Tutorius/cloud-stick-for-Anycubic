/**
 * @file lcd.cpp
 * @brief LCD driver implementation for the ST7735 0.96" 160x80 display
 *        driven by the TFT_eSPI library.
 *
 * All TFT_eSPI configuration is supplied via build flags in platformio.ini:
 *   USER_SETUP_LOADED=1   – tells the lib to skip its own User_Setup.h
 *   ST7735_DRIVER=1       – selects the ST7735 driver
 *   ST7735_GREENTAB160x80 – 160×80 variant (BGR, inverted, 26 px offset)
 *   TFT_WIDTH=80          – logical width  (portrait origin; lib transposes)
 *   TFT_HEIGHT=160        – logical height
 *   TFT_MOSI=3            – SDA
 *   TFT_SCLK=5            – SCL
 *   TFT_CS=4              – chip select
 *   TFT_DC=2              – data/command
 *   TFT_RST=1             – hardware reset
 *   TFT_BL=38             – backlight enable (active HIGH)
 *   LOAD_GFXFF=1          – enable GFX free-font support (FreeSans9pt7b)
 *   SPI_FREQUENCY=27000000
 */

#include "lcd.h"

#include <Arduino.h>
#include <TFT_eSPI.h>                  // Bodmer's TFT_eSPI library

// FreeSans9pt7b is already included transitively by TFT_eSPI.h via
// Fonts/GFXFF/gfxfont.h (line 52) when LOAD_GFXFF=1 is defined.
// Do NOT include it again here — that causes redefinition errors.

// ---------------------------------------------------------------------------
// Hourglass bitmap (24 × 24, 1-bpp, MSB first, 3 bytes per row)
// ---------------------------------------------------------------------------
static const unsigned char PROGMEM image_hourglass1_bits[] = {
    0x00, 0x00, 0x00,   // row  0
    0x07, 0xff, 0xf0,   // row  1
    0x04, 0x00, 0x10,   // row  2
    0x03, 0xff, 0xe0,   // row  3
    0x01, 0x00, 0x40,   // row  4
    0x01, 0x00, 0x40,   // row  5
    0x01, 0x7f, 0x40,   // row  6
    0x01, 0x3e, 0x40,   // row  7
    0x00, 0x9c, 0x80,   // row  8
    0x00, 0x49, 0x00,   // row  9
    0x00, 0x22, 0x00,   // row 10
    0x00, 0x14, 0x00,   // row 11
    0x00, 0x14, 0x00,   // row 12
    0x00, 0x22, 0x00,   // row 13
    0x00, 0x49, 0x00,   // row 14
    0x00, 0x80, 0x80,   // row 15
    0x01, 0x08, 0x40,   // row 16
    0x01, 0x3e, 0x40,   // row 17
    0x01, 0x7f, 0x40,   // row 18
    0x01, 0x00, 0x40,   // row 19
    0x03, 0xff, 0xe0,   // row 20
    0x04, 0x00, 0x10,   // row 21
    0x07, 0xff, 0xf0,   // row 22
    0x00, 0x00, 0x00,   // row 23
};

// ---------------------------------------------------------------------------
// TFT object — constructed once, zero-argument (all config via build flags)
// ---------------------------------------------------------------------------
static TFT_eSPI tft;

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

void lcd_init()
{
    pinMode(TFT_BL, OUTPUT);
    digitalWrite(TFT_BL, LOW); // Active-low backlight on T-Dongle-S3

    tft.init();
    tft.setRotation(1);
    tft.fillScreen(TFT_BLACK);
}

void lcd_show_success(const char* message) {
    static const unsigned char PROGMEM image_check_contour_bits[] = {0x00,0x00,0x0c,0x00,0x00,0x00,0x0c,0x00,0x00,0x00,0x33,0x00,0x00,0x00,0x33,0x00,0x0c,0x00,0xc0,0xc0,0x0c,0x00,0xc0,0xc0,0x33,0x03,0x03,0x00,0x33,0x03,0x03,0x00,0xc0,0xcc,0x0c,0x00,0xc0,0xcc,0x0c,0x00,0x30,0x30,0x30,0x00,0x30,0x30,0x30,0x00,0x0c,0x00,0xc0,0x00,0x0c,0x00,0xc0,0x00,0x03,0x03,0x00,0x00,0x03,0x03,0x00,0x00,0x00,0xcc,0x00,0x00,0x00,0xcc,0x00,0x00,0x00,0x30,0x00,0x00,0x00,0x30,0x00,0x00};

    tft.fillScreen(0x0);
    // Loading copy 1
    tft.setTextColor(0xFFFF);
    tft.setTextSize(1);
    tft.setFreeFont(&FreeSans9pt7b);
    tft.drawString(message, 11, 38);
    // check_contour
    tft.drawBitmap(67, 12, image_check_contour_bits, 26, 20, 0x4D6A);

}

void lcd_show_error(const char* message) {
    static const unsigned char PROGMEM image_cross_contour_bits[] = {0x0c,0x00,0xc0,0x0c,0x00,0xc0,0x33,0x03,0x30,0x33,0x03,0x30,0xc0,0xcc,0x0c,0xc0,0xcc,0x0c,0x30,0x30,0x30,0x30,0x30,0x30,0x0c,0x00,0xc0,0x0c,0x00,0xc0,0x03,0x03,0x00,0x03,0x03,0x00,0x0c,0x00,0xc0,0x0c,0x00,0xc0,0x30,0x30,0x30,0x30,0x30,0x30,0xc0,0xcc,0x0c,0xc0,0xcc,0x0c,0x33,0x03,0x30,0x33,0x03,0x30,0x0c,0x00,0xc0,0x0c,0x00,0xc0};

    tft.fillScreen(0x0);
    // Loading copy 1
    tft.setTextColor(0xFFFF);
    tft.setTextSize(1);
    tft.setFreeFont(&FreeSans9pt7b);
    tft.drawString(message, 31, 38);
    // cross_contour
    tft.drawBitmap(69, 12, image_cross_contour_bits, 22, 22, 0xF206);
}

void lcd_show_loading()
{
    tft.fillScreen(0x0000);
    tft.drawBitmap(15, 28, image_hourglass1_bits, 24, 24, 0xFFFF);
    tft.setTextColor(0xFFFF);
    tft.setTextSize(1);
    tft.setFreeFont(&FreeSans9pt7b);
    tft.drawString("Loading...", 48, 33);
}

void lcd_clear()
{
    tft.fillScreen(TFT_BLACK);
}