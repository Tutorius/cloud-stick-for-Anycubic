#include "esp_arduino_version.h"
#if ESP_ARDUINO_VERSION < ESP_ARDUINO_VERSION_VAL(3, 3, 0)
#error This sketch requires ESP32 Arduino Core version 3.3.0 or later
#endif

#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include "config.h"
#include "usb_storage.h"
#include "lcd.h"
#include "config_loader.h"
#include "logger.h"

// Global dashboard state
DashboardState g_dash_state = {0};

// Task to periodically check internet reachability
void internet_check_task(void *pvParameters) {
    WiFiClient client;
    while (1) {
        if (WiFi.status() == WL_CONNECTED) {
            // Signal strength mapping (rough estimation)
            int32_t rssi = WiFi.RSSI();
            if (rssi > -60) g_dash_state.wifi_signal = 3;
            else if (rssi > -75) g_dash_state.wifi_signal = 2;
            else if (rssi > -90) g_dash_state.wifi_signal = 1;
            else g_dash_state.wifi_signal = 0;

            // Ping google DNS to check real internet access
            if (client.connect("8.8.8.8", 53)) {
                g_dash_state.internet_connected = true;
                client.stop();
            } else {
                g_dash_state.internet_connected = false;
            }
        } else {
            g_dash_state.wifi_signal = 0;
            g_dash_state.internet_connected = false;
        }
        vTaskDelay(pdMS_TO_TICKS(5000)); // check every 5 seconds
    }
}

void setup() {
    // Bring up the display first so the user sees feedback immediately.
    lcd_init();
    lcd_show_loading();

    // Fire up the SD card, LEDs, and composite USB (MSC + CDC serial).
    // USBSerial.begin() is called inside init_usb_storage(), so any
    // prints before USB enumerates will be buffered and flushed once
    // the host connects.
    init_usb_storage();
    
    // Initialize logger to SD card
    log_init();

    log_printf("[main] Storage and USB initialized.\n");

    // Load configuration from SD card
    CloudConfig config = load_config_from_sd();

    if (config.is_valid) {
        log_printf("[main] Connecting to WiFi SSID: %s\n", config.ssid.c_str());
        lcd_show_text("Connecting WiFi...");
        
        WiFi.begin(config.ssid.c_str(), config.wifi_password.c_str());
        
        // Wait for connection with a timeout (e.g. 10 seconds)
        int timeout_ms = 10000;
        int elapsed = 0;
        while (WiFi.status() != WL_CONNECTED && elapsed < timeout_ms) {
            delay(500);
            elapsed += 500;
            log_printf(".");
        }
        log_printf("\n");

        if (WiFi.status() == WL_CONNECTED) {
            log_printf("[main] WiFi connected! IP: %s\n", WiFi.localIP().toString().c_str());
            lcd_show_text("WiFi Connected!");
            delay(1000); // Let the user read the status
            lcd_show_text(WiFi.localIP().toString().c_str());
        } else {
            log_printf("[main] WiFi connection timed out.\n");
            lcd_show_error("WiFi Failed.");
        }
        delay(2000); // Let the user read the status
    } else {
        log_printf("[main] Skipping WiFi connection.\n");
    }

    // Start background internet checker
    xTaskCreate(internet_check_task, "InternetCheck", 4096, NULL, 1, NULL);

    // Initialize the static dashboard elements
    lcd_dashboard_init();

    // Set initial values
    g_dash_state.sd_mounted = true;
    strcpy(g_dash_state.last_action, "Ready.");
    lcd_dashboard_update(g_dash_state);

    log_printf("[main] Setup complete.\n");
}

void loop() {
    static unsigned long last_update = 0;
    
    // Animate the dashboard every 500ms
    if (millis() - last_update > 500) {
        last_update = millis();

        // Simulate SD reading and writing speeds
        g_dash_state.read_speed_kbps = random(8000, 16000); // 8-16 MB/s
        g_dash_state.write_speed_kbps = random(4000, 10000); // 4-10 MB/s

        // Simulate Cloud up/down speeds (only when internet is connected)
        if (g_dash_state.internet_connected) {
            g_dash_state.upload_speed_kbps = random(2000, 6000);
            g_dash_state.download_speed_kbps = random(5000, 15000);
            g_dash_state.is_syncing = (millis() / 2000) % 2 == 0; // toggle sync icon
        } else {
            g_dash_state.upload_speed_kbps = 0;
            g_dash_state.download_speed_kbps = 0;
            g_dash_state.is_syncing = false;
        }

        // Draw the updated state to the LCD
        lcd_dashboard_update(g_dash_state);
    }
}