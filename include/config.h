#pragma once

#include <Arduino.h>
#include "esp_arduino_version.h"

#if ESP_ARDUINO_VERSION < ESP_ARDUINO_VERSION_VAL(3, 3, 0)
#error This sketch requires ESP32 Arduino Core version 3.3.0 or later
#endif

// SD MMC Pins
#define SD_MMC_D0_PIN  14
#define SD_MMC_D1_PIN  17
#define SD_MMC_D2_PIN  21
#define SD_MMC_D3_PIN  18
#define SD_MMC_CLK_PIN 12
#define SD_MMC_CMD_PIN 16

// LED Pins
#define LED_DI_PIN     40
#define LED_CI_PIN     39

// TFT-Rotation
#define TFT_ROTATION 3

// DELAY_FOR_RESTART
#define DELAY_FOR_RESTART 5000
