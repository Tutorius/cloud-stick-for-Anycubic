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
#include <TFT_eSPI.h> // Bodmer's TFT_eSPI library

// FreeSans9pt7b is already included transitively by TFT_eSPI.h via
// Fonts/GFXFF/gfxfont.h (line 52) when LOAD_GFXFF=1 is defined.
// Do NOT include it again here — that causes redefinition errors.

static const unsigned char PROGMEM image_hourglass1_bits[] = {
    0x00, 0x00, 0x00, 0x07, 0xff, 0xf0, 0x04, 0x00, 0x10, 0x03, 0xff, 0xe0,
    0x01, 0x00, 0x40, 0x01, 0x00, 0x40, 0x01, 0x7f, 0x40, 0x01, 0x3e, 0x40,
    0x00, 0x9c, 0x80, 0x00, 0x49, 0x00, 0x00, 0x22, 0x00, 0x00, 0x14, 0x00,
    0x00, 0x14, 0x00, 0x00, 0x22, 0x00, 0x00, 0x49, 0x00, 0x00, 0x80, 0x80,
    0x01, 0x08, 0x40, 0x01, 0x3e, 0x40, 0x01, 0x7f, 0x40, 0x01, 0x00, 0x40,
    0x03, 0xff, 0xe0, 0x04, 0x00, 0x10, 0x07, 0xff, 0xf0, 0x00, 0x00, 0x00};
static const unsigned char PROGMEM image_check_contour_bits[] = {
    0x00, 0x00, 0x0c, 0x00, 0x00, 0x00, 0x0c, 0x00, 0x00, 0x00, 0x33, 0x00,
    0x00, 0x00, 0x33, 0x00, 0x0c, 0x00, 0xc0, 0xc0, 0x0c, 0x00, 0xc0, 0xc0,
    0x33, 0x03, 0x03, 0x00, 0x33, 0x03, 0x03, 0x00, 0xc0, 0xcc, 0x0c, 0x00,
    0xc0, 0xcc, 0x0c, 0x00, 0x30, 0x30, 0x30, 0x00, 0x30, 0x30, 0x30, 0x00,
    0x0c, 0x00, 0xc0, 0x00, 0x0c, 0x00, 0xc0, 0x00, 0x03, 0x03, 0x00, 0x00,
    0x03, 0x03, 0x00, 0x00, 0x00, 0xcc, 0x00, 0x00, 0x00, 0xcc, 0x00, 0x00,
    0x00, 0x30, 0x00, 0x00, 0x00, 0x30, 0x00, 0x00};
static const unsigned char PROGMEM image_cross_contour_bits[] = {
    0x0c, 0x00, 0xc0, 0x0c, 0x00, 0xc0, 0x33, 0x03, 0x30, 0x33, 0x03,
    0x30, 0xc0, 0xcc, 0x0c, 0xc0, 0xcc, 0x0c, 0x30, 0x30, 0x30, 0x30,
    0x30, 0x30, 0x0c, 0x00, 0xc0, 0x0c, 0x00, 0xc0, 0x03, 0x03, 0x00,
    0x03, 0x03, 0x00, 0x0c, 0x00, 0xc0, 0x0c, 0x00, 0xc0, 0x30, 0x30,
    0x30, 0x30, 0x30, 0x30, 0xc0, 0xcc, 0x0c, 0xc0, 0xcc, 0x0c, 0x33,
    0x03, 0x30, 0x33, 0x03, 0x30, 0x0c, 0x00, 0xc0, 0x0c, 0x00, 0xc0};
static const unsigned char PROGMEM image_wifi_bits[] = {
    0x01, 0xf0, 0x00, 0x06, 0x0c, 0x00, 0x18, 0x03, 0x00, 0x21, 0xf0, 0x80,
    0x46, 0x0c, 0x40, 0x88, 0x02, 0x20, 0x10, 0xe1, 0x00, 0x23, 0x18, 0x80,
    0x04, 0x04, 0x00, 0x08, 0x42, 0x00, 0x01, 0xb0, 0x00, 0x02, 0x08, 0x00,
    0x00, 0x40, 0x00, 0x00, 0xa0, 0x00, 0x00, 0x40, 0x00};

// Dashboard
static const unsigned char PROGMEM image_cloud_1_bits[] = {
    0x07, 0xc0, 0x00, 0x08, 0x20, 0x00, 0x10, 0x10, 0x00,
    0x30, 0x08, 0x00, 0x40, 0x0e, 0x00, 0x80, 0x01, 0x00,
    0x80, 0x00, 0x80, 0x40, 0x00, 0x80, 0x3f, 0xff, 0x00};
static const unsigned char PROGMEM image_cloud_sync_bits[] = {
    0x00, 0x00, 0x00, 0x07, 0xc0, 0x00, 0x08, 0x20, 0x00, 0x10, 0x10, 0x00,
    0x30, 0x08, 0x00, 0x40, 0x0e, 0x00, 0x80, 0x01, 0x00, 0x82, 0x20, 0x80,
    0x47, 0x20, 0x80, 0x2f, 0xaf, 0x00, 0x02, 0x20, 0x00, 0x02, 0xf8, 0x00,
    0x02, 0x70, 0x00, 0x02, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
static const unsigned char PROGMEM image_folder_explorer_bits[] = {
    0x3c, 0x00, 0x00, 0x42, 0x00, 0x00, 0x81, 0xff, 0x00, 0x80, 0x00, 0x80,
    0xbf, 0xfe, 0x80, 0xc0, 0x01, 0x80, 0x80, 0x00, 0x80, 0x80, 0x00, 0x80,
    0x80, 0x00, 0x80, 0x80, 0x00, 0x80, 0x80, 0x00, 0x80, 0x80, 0x00, 0x80,
    0x80, 0x00, 0x80, 0x80, 0x00, 0x80, 0x80, 0x00, 0x80, 0x7f, 0xff, 0x00};
static const unsigned char PROGMEM image_internet_bits[] = {
    0x03, 0xc0, 0x0d, 0xb0, 0x32, 0x4c, 0x24, 0x24, 0x44, 0x22, 0x7f,
    0xfe, 0x88, 0x11, 0x88, 0x11, 0x88, 0x11, 0x88, 0x11, 0x7f, 0xfe,
    0x44, 0x22, 0x24, 0x24, 0x32, 0x4c, 0x0d, 0xb0, 0x03, 0xc0};
static const unsigned char PROGMEM image_menu_information_sign_white_bits[] = {
    0x07, 0xc0, 0x18, 0x30, 0x23, 0x08, 0x42, 0x84, 0x43, 0x04, 0x80,
    0x02, 0x83, 0x82, 0x82, 0x82, 0x82, 0x82, 0x82, 0x82, 0x42, 0x84,
    0x43, 0x84, 0x20, 0x08, 0x18, 0x30, 0x07, 0xc0, 0x00, 0x00};
static const unsigned char PROGMEM image_micro_sd_bits[] = {
    0x3f, 0xc0, 0x60, 0x20, 0x40, 0x20, 0x40, 0x20, 0x47, 0x30, 0x48,
    0x90, 0x4f, 0x88, 0x40, 0x08, 0x49, 0x10, 0x4a, 0x90, 0x44, 0x88,
    0x40, 0x08, 0x47, 0x88, 0x58, 0x68, 0x37, 0xb0, 0x00, 0x00};
static const unsigned char PROGMEM image_refresh_1_bits[] = {
    0x01, 0xe0, 0x7c, 0x38, 0x1c, 0x0c, 0x3c, 0x0c, 0x74,
    0x06, 0x64, 0x06, 0x60, 0x06, 0x60, 0x26, 0x60, 0x2e,
    0x30, 0x3c, 0x30, 0x38, 0x18, 0x3e, 0x07, 0x00};
static const unsigned char PROGMEM image_refresh_bits[] = {
    0x00, 0x00, 0x0f, 0x90, 0x3f, 0xd0, 0x70, 0xf0, 0x40, 0x70, 0xc1,
    0xf0, 0x80, 0x00, 0x80, 0x00, 0x80, 0x08, 0x00, 0x08, 0x7c, 0x08,
    0x70, 0x10, 0x78, 0x70, 0x5f, 0xe0, 0x4f, 0x80, 0x00, 0x00};
static const unsigned char PROGMEM image_wifi_2_bars_bits[] = {
    0x01, 0xf0, 0x00, 0x06, 0x0c, 0x00, 0x18, 0x03, 0x00, 0x21, 0xf0, 0x80,
    0x46, 0x0c, 0x40, 0x88, 0x02, 0x20, 0x10, 0xe1, 0x00, 0x23, 0x18, 0x80,
    0x04, 0x04, 0x00, 0x08, 0x42, 0x00, 0x01, 0xf0, 0x00, 0x03, 0xb8, 0x00,
    0x01, 0x50, 0x00, 0x00, 0xe0, 0x00, 0x00, 0x40, 0x00, 0x00, 0x00, 0x00};
static const unsigned char PROGMEM image_wifi_50_bits[] = {
    0x01, 0xf0, 0x00, 0x06, 0x0c, 0x00, 0x18, 0x03, 0x00, 0x21, 0xf0, 0x80,
    0x46, 0x0c, 0x40, 0x88, 0x02, 0x20, 0x50, 0xe1, 0x40, 0x23, 0xf8, 0x80,
    0x17, 0x1d, 0x00, 0x0e, 0xee, 0x00, 0x05, 0xf4, 0x00, 0x03, 0xb8, 0x00,
    0x01, 0x50, 0x00, 0x00, 0xe0, 0x00, 0x00, 0x40, 0x00, 0x00, 0x00, 0x00};
static const unsigned char PROGMEM image_wifi_full_bits[] = {
    0x01, 0xf0, 0x00, 0x07, 0xfc, 0x00, 0x1e, 0x0f, 0x00, 0x39, 0xf3, 0x80,
    0x77, 0xfd, 0xc0, 0xef, 0x1e, 0xe0, 0x5c, 0xe7, 0x40, 0x3b, 0xfb, 0x80,
    0x17, 0x1d, 0x00, 0x0e, 0xee, 0x00, 0x05, 0xf4, 0x00, 0x03, 0xb8, 0x00,
    0x01, 0x50, 0x00, 0x00, 0xe0, 0x00, 0x00, 0x40, 0x00, 0x00, 0x00, 0x00};

// ---------------------------------------------------------------------------
// TFT object — constructed once, zero-argument (all config via build flags)
// ---------------------------------------------------------------------------
static TFT_eSPI tft;

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

void lcd_init() {
  pinMode(TFT_BL, OUTPUT);
  digitalWrite(TFT_BL, LOW); // Active-low backlight on T-Dongle-S3

  tft.init();
  tft.setRotation(1);
  tft.fillScreen(TFT_BLACK);
}

void lcd_show_success(const char *message) {

  tft.fillScreen(0x0);
  // Loading copy 1
  tft.setTextColor(0xFFFF);
  tft.setTextSize(1);
  tft.setFreeFont(&FreeSans9pt7b);
  tft.drawString(message, 11, 38);
  // check_contour
  tft.drawBitmap(67, 12, image_check_contour_bits, 26, 20, 0x4D6A);
}

void lcd_show_error(const char *message) {

  tft.fillScreen(0x0);
  // Loading copy 1
  tft.setTextColor(0xFFFF);
  tft.setTextSize(1);
  tft.setFreeFont(&FreeSans9pt7b);
  tft.drawString(message, 31, 38);
  // cross_contour
  tft.drawBitmap(69, 12, image_cross_contour_bits, 22, 22, 0xF206);
}

void lcd_show_state(const char *state) {
  tft.fillScreen(0x0000);

  if (strcmp(state, "loading") == 0) {
    tft.drawBitmap(15, 28, image_hourglass1_bits, 24, 24, 0xFFFF);
    tft.setTextColor(0xFFFF);
    tft.setTextSize(1);
    tft.setFreeFont(&FreeSans9pt7b);
    tft.drawString("Starting...", 50, 33);
  } else if (strcmp(state, "wifi_connecting") == 0) {
    tft.setTextColor(0xFFFF);
    tft.setTextSize(1);
    tft.setFreeFont(&FreeSans9pt7b);
    tft.drawString("Connecting...", 27, 38);
    // wifi
    tft.drawBitmap(71, 16, image_wifi_bits, 19, 15, 0xFFE7);
  } else {
    lcd_clear();
  }
}

static DashboardState prev_state = {-1, false, false, false, false, 0, 0, 0, 0, 0, 0, -1, {0}};

void format_speed(uint32_t speed_kbps, char *buf, size_t buf_size) {
  if (speed_kbps < 1000) {
    snprintf(buf, buf_size, "%lu kb/s", speed_kbps);
  } else {
    snprintf(buf, buf_size, "%lu mb/s", speed_kbps / 1000);
  }
}

void lcd_dashboard_init() {
  lcd_clear();

  // Static elements
  tft.drawBitmap(3, 24, image_cloud_sync_bits, 17, 16, 0xFFFF);
  tft.drawBitmap(3, 43, image_folder_explorer_bits, 17, 16, 0xFFFF);
  tft.drawBitmap(4, 63, image_menu_information_sign_white_bits, 15, 16, 0xFFFF);

  tft.setTextColor(0x9936);
  tft.setFreeFont(); // use default font
  tft.drawString("Cloud Stick", 90, 6);
  tft.drawLine(89, 14, 154, 14, 0x9936);

  // Reset previous state so everything draws on first update
  prev_state.wifi_signal = -1;
  prev_state.internet_connected = !prev_state.internet_connected;
  prev_state.server_connected = !prev_state.server_connected;
  prev_state.sd_mounted = !prev_state.sd_mounted;
  prev_state.is_syncing = !prev_state.is_syncing;
  prev_state.upload_speed_kbps = 0xFFFFFFFF;
  prev_state.download_speed_kbps = 0xFFFFFFFF;
  prev_state.read_speed_kbps = 0xFFFFFFFF;
  prev_state.write_speed_kbps = 0xFFFFFFFF;
  prev_state.sync_progress_percent = -1;
  prev_state.last_action[0] = '\0';
}

void lcd_dashboard_update(const DashboardState &state) {
  tft.setFreeFont(); // default font for stats

  // WiFi Icon
  if (state.wifi_signal != prev_state.wifi_signal) {
    tft.fillRect(2, 1, 19, 18, TFT_BLACK); // clear old icon
    if (state.wifi_signal == 3)
      tft.drawBitmap(2, 2, image_wifi_full_bits, 19, 16, 0xFFFF);
    else if (state.wifi_signal == 2)
      tft.drawBitmap(2, 2, image_wifi_50_bits, 19, 16, 0xFFFF);
    else if (state.wifi_signal == 1)
      tft.drawBitmap(2, 2, image_wifi_2_bars_bits, 19, 16, 0xFFFF);
    else if (state.wifi_signal == 0) {
      tft.drawBitmap(2, 2, image_wifi_full_bits, 19, 16, 0x7BEF); // dim
      tft.drawLine(2, 1, 19, 18, 0xFFFF);                         // strike
    }
    prev_state.wifi_signal = state.wifi_signal;
  }

  // Internet Icon
  if (state.internet_connected != prev_state.internet_connected) {
    tft.fillRect(26, 1, 18, 18, TFT_BLACK);
    if (state.internet_connected) {
      tft.drawBitmap(27, 2, image_internet_bits, 16, 16, 0xFFFF);
    } else {
      tft.drawBitmap(27, 2, image_internet_bits, 16, 16, 0x7BEF);
      tft.drawLine(26, 1, 43, 18, 0xFFFF);
    }
    prev_state.internet_connected = state.internet_connected;
  }

  // Server Icon (Cloud)
  if (state.server_connected != prev_state.server_connected) {
    tft.fillRect(47, 1, 18, 18, TFT_BLACK);
    if (state.server_connected) {
      tft.drawBitmap(48, 5, image_cloud_1_bits, 17, 9, 0xFFFF);
    } else {
      tft.drawBitmap(48, 5, image_cloud_1_bits, 17, 9, 0x7BEF);
      tft.drawLine(47, 1, 64, 18, 0xFFFF);
    }
    prev_state.server_connected = state.server_connected;
  }

  // SD Card Icon
  if (state.sd_mounted != prev_state.sd_mounted) {
    tft.fillRect(48, 2, 16, 18, TFT_BLACK);
    if (state.sd_mounted) {
      tft.drawBitmap(49, 3, image_micro_sd_bits, 14, 16, 0xFFFF);
    } else {
      tft.drawBitmap(49, 3, image_micro_sd_bits, 14, 16, 0x7BEF);
      tft.drawLine(48, 2, 63, 18, 0xFFFF);
    }
    prev_state.sd_mounted = state.sd_mounted;
  }

  // Sync Icon
  if (state.is_syncing != prev_state.is_syncing) {
    tft.fillRect(67, 1, 18, 18, TFT_BLACK);
    if (state.is_syncing) {
      tft.drawBitmap(69, 2, image_refresh_bits, 13, 16, 0xFFFF);
    }
    prev_state.is_syncing = state.is_syncing;
  }

  char buf[16];

  // Upload Speed
  if (state.upload_speed_kbps != prev_state.upload_speed_kbps) {
    format_speed(state.upload_speed_kbps, buf, sizeof(buf));
    tft.setTextColor(0x24BE, TFT_BLACK); // fg, bg to overwrite old text
    tft.drawString("U:         ", 26,
                   24); // clear old text by padding with spaces
    tft.drawString(String("U: ") + buf, 26, 24);
    prev_state.upload_speed_kbps = state.upload_speed_kbps;
  }

  // Download Speed
  if (state.download_speed_kbps != prev_state.download_speed_kbps) {
    format_speed(state.download_speed_kbps, buf, sizeof(buf));
    tft.setTextColor(0x4D6A, TFT_BLACK);
    tft.drawString("D:         ", 26, 32);
    tft.drawString(String("D: ") + buf, 26, 32);
    prev_state.download_speed_kbps = state.download_speed_kbps;
  }

  // Read Speed
  if (state.read_speed_kbps != prev_state.read_speed_kbps) {
    format_speed(state.read_speed_kbps, buf, sizeof(buf));
    tft.setTextColor(0xFFE0, TFT_BLACK);
    tft.drawString("R:         ", 26, 43);
    tft.drawString(String("R: ") + buf, 26, 43);
    prev_state.read_speed_kbps = state.read_speed_kbps;
  }

  // Write Speed
  if (state.write_speed_kbps != prev_state.write_speed_kbps) {
    format_speed(state.write_speed_kbps, buf, sizeof(buf));
    tft.setTextColor(0xF810, TFT_BLACK);
    tft.drawString("W:         ", 26, 52);
    tft.drawString(String("W: ") + buf, 26, 52);
    prev_state.write_speed_kbps = state.write_speed_kbps;
  }

  // Last Action
  if (strcmp(state.last_action, prev_state.last_action) != 0) {
    tft.fillRect(27, 68, 130, 10, TFT_BLACK); // clear old text
    tft.setTextColor(0xFC00); // no background so it looks cleaner
    tft.drawString(state.last_action, 27, 68);
    strncpy(prev_state.last_action, state.last_action,
            sizeof(prev_state.last_action));
  }

  // Progress Bar
  if (state.sync_progress_percent != prev_state.sync_progress_percent || state.is_syncing != prev_state.is_syncing) {
      if (state.is_syncing && state.sync_progress_percent >= 0 && state.sync_progress_percent <= 100) {
          int bar_width = (160 * state.sync_progress_percent) / 100;
          if (bar_width > 0) tft.fillRect(0, 78, bar_width, 2, 0xFC00);
          if (bar_width < 160) tft.fillRect(bar_width, 78, 160 - bar_width, 2, TFT_BLACK);
      } else {
          tft.fillRect(0, 78, 160, 2, TFT_BLACK);
      }
      prev_state.sync_progress_percent = state.sync_progress_percent;
  }
}

void lcd_show_loading() { lcd_show_state("loading"); }

void lcd_clear() { tft.fillScreen(TFT_BLACK); }

void lcd_show_text(const char *message) {
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(0xFFFF);
  tft.setTextSize(1);
  tft.setFreeFont(&FreeSans9pt7b);
  tft.setTextDatum(MC_DATUM); // Middle center
  tft.drawString(message, 80, 40);
  tft.setTextDatum(TL_DATUM); // Reset to top left
}