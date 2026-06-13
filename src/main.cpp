#ifndef ARDUINO_USB_MODE
#error This ESP32 SoC has no Native USB interface
#elif ARDUINO_USB_MODE == 1
#warning This sketch should be used when USB is in OTG mode
void setup() {}
void loop() {}
#else

#include "esp_arduino_version.h"
#if ESP_ARDUINO_VERSION < ESP_ARDUINO_VERSION_VAL(3, 3, 0)
#error This sketch requires ESP32 Arduino Core version 3.3.0 or later
#endif

#include <Arduino.h>
#include "config.h"
#include "usb_storage.h"

void setup() {
    Serial.begin(115200);
    
    // Fire up the SD card, LEDs, and USB Mass Storage
    init_usb_storage();
    
    Serial.println("USB Storage initialized! Ready for main tasks.");
}

void loop() {
    // Put your main application code here
}

#endif /* ARDUINO_USB_MODE */