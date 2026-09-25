/*
** Cloud-Stick for Anycubic
** 
** Set the "#define TFT_ROTATION" to 1 (normal mode, stick pointing to left) or 3 (stick pointing to right). For Anycubic Kobra 4, the 3 is correct choose
**
** Set "#define DELAY_FOR_RESTART" to a value in Milliseconds, values lower 1000 disable automatic restart after download. Other values lets wait the value in ms before restarting
**
** With the  "#define ENABLE_RESTART" and "#define ENABLE_DELETEALL" active the named dunctions work
** RESTART: copying a file named "restart.cmd" to the Webdav-server restarts the stick after the waittime
** DELETEALL : *** caution : deletes all files on stick AND Webdav-server. copying a file named "deleteall.cmd" starts this function. Use it carefully, if you know that you want it
**
** "#define NOUPLOAD" : the stick can not upload files to the Webdav-server
*/

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

// TFT-Rotation ( 1 for "normal" operation (stick pointing to the left), 3 for stick pointing to the right )
#define TFT_ROTATION 3

// DELAY_FOR_RESTART after download in ms, a value lower than 1000ms disables the function of automatical restart
#define DELAY_FOR_RESTART 10000

// Enable the Restart and Delete-all-function, comment it out to disable it
#define ENABLE_RESTART 1
#define ENABLE_DELETEALL 1

// Disable Upload-function (Stick to WebDAV-server), uncomment to enable it
// 3D-printers normally don't put things on a stick...
#define NOUPLOAD 1
