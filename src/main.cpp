#include "esp_arduino_version.h"
#if ESP_ARDUINO_VERSION < ESP_ARDUINO_VERSION_VAL(3, 3, 0)
#error This sketch requires ESP32 Arduino Core version 3.3.0 or later
#endif

#include <Arduino.h>
#include "config.h"
#include "usb_storage.h"
#include "lcd.h"

void setup() {
    Serial.begin(115200);

    // Wait up to 2 s for the USB CDC host to enumerate so we don't lose
    // any early serial output (HWCDC buffers are discarded on crash/reset).
    delay(2000);
    Serial.println("[main] setup() started");
    Serial.flush();

    // Bring up the display first so the user sees feedback immediately.
    lcd_init();
    lcd_show_loading();

    // Fire up the SD card, LEDs, and USB Mass Storage
    // init_usb_storage();

    Serial.println("[main] setup() done");
    Serial.flush();
}

void loop() {
    // Put your main application code here
}