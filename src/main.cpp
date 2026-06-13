#include "esp_arduino_version.h"
#if ESP_ARDUINO_VERSION < ESP_ARDUINO_VERSION_VAL(3, 3, 0)
#error This sketch requires ESP32 Arduino Core version 3.3.0 or later
#endif

#include <Arduino.h>
#include "config.h"
#include "usb_storage.h"
#include "lcd.h"

// In TinyUSB mode (ARDUINO_USB_MODE=0), Serial maps to UART0 (physical
// TX/RX pins), not to USB. We use USBSerial so output appears in the
// virtual serial port on the USB connector.

void setup() {
    // Bring up the display first so the user sees feedback immediately.
    lcd_init();
    lcd_show_loading();

    // Fire up the SD card, LEDs, and composite USB (MSC + CDC serial).
    // USBSerial.begin() is called inside init_usb_storage(), so any
    // prints before USB enumerates will be buffered and flushed once
    // the host connects.
    init_usb_storage();
    lcd_show_success("Mount successful");
}

void loop() {
    // Put your main application code here
}