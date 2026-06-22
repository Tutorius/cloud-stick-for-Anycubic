#pragma once
#include <cstdint>

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
 * @brief Display a success message with a green checkmark icon.
 *
 * @param message Null-terminated string to display below the icon.
 */
void lcd_show_success(const char* message);

/**
 * @brief Display an error message with a red cross icon.
 *
 * @param message Null-terminated string to display below the icon.
 */
void lcd_show_error(const char* message);

/**
 * @brief Display the loading splash screen:
 *        Black background, hourglass bitmap on the left,
 *        "Loading..." text in FreeSans9pt7b on the right.
 */
void lcd_show_loading();

/**
 * @brief Display a generic text message centered on the screen.
 *
 * @param message Null-terminated string to display.
 */
void lcd_show_text(const char* message);

/**
 * @brief Clear the screen to black.
 */
void lcd_clear();

struct DashboardState {
    int wifi_signal; // 0=disconnected, 1=low, 2=med, 3=high
    bool internet_connected;
    bool server_connected;
    bool sd_mounted;
    bool is_syncing;
    uint32_t upload_speed_kbps;
    uint32_t download_speed_kbps;
    uint32_t read_speed_kbps;
    uint32_t write_speed_kbps;
    int sync_current_file;
    int sync_total_files;
    int sync_progress_percent;
    char last_action[64];
};

/**
 * @brief Initialize the dashboard layout (draws static elements).
 */
void lcd_dashboard_init();

/**
 * @brief Dynamically update the dashboard with new state without flickering.
 */
void lcd_dashboard_update(const DashboardState& state);
