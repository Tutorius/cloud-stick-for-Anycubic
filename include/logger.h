#pragma once

#include <Arduino.h>

// Initializes the logger by creating or opening the log file on the SD card.
// Must be called AFTER SD_MMC is initialized.
void log_init();

// Logs a formatted message to both USBSerial and the log file on the SD card.
void log_printf(const char* format, ...);
