#pragma once

/**
 * @file lcd.h
 * @brief LCD driver interface for the ST7735 0.96" 160x80 display
 *        using the TFT_eSPI library.
 *
 * Pin assignments (configured in platformio.ini build flags):
 *   TFT_MOSI (SDA)  → GPIO 3
 *   TFT_SCLK (SCL)  → GPIO 5
 *   TFT_CS          → GPIO 4
 *   TFT_DC          → GPIO 2
 *   TFT_RST         → GPIO 1
 *   TFT_BL          → GPIO 38
 */

/**
 * @brief Initialise the TFT display and turn on the backlight.
 *        Must be called once from setup() before any draw calls.
 */
void lcd_init();

/**
 * @brief Display the loading splash screen:
 *        Black background, hourglass bitmap on the left,
 *        "Loading..." text in FreeSans9pt7b on the right.
 */
void lcd_show_loading();

/**
 * @brief Clear the screen to black.
 */
void lcd_clear();
