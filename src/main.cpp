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
#include "webdav_client.h"
#include "sync_engine.h"



// Global dashboard state
DashboardState g_dash_state = {0};

// Task to periodically check internet reachability
void internet_check_task(void *pvParameters) {
    WiFiClient client;
    bool was_connected = false;
    while (1) {
        if (WiFi.status() == WL_CONNECTED) {
            if (!was_connected) {
                was_connected = true;
                snprintf(g_dash_state.last_action, sizeof(g_dash_state.last_action), "IP: %s", WiFi.localIP().toString().c_str());
            }

            int32_t rssi = WiFi.RSSI();
            if (rssi != 0) { // Ignore 0 which is often a transient read error
                if (rssi > -60) g_dash_state.wifi_signal = 3;
                else if (rssi > -75) g_dash_state.wifi_signal = 2;
                else g_dash_state.wifi_signal = 1; // 1 bar for weak signal, NO strike
            }

            // Ping google DNS to check real internet access
            if (client.connect("8.8.8.8", 53)) {
                g_dash_state.internet_connected = true;
                client.stop();
            } else {
                g_dash_state.internet_connected = false;
            }
        } else {
            if (was_connected) {
                was_connected = false;
                strncpy(g_dash_state.last_action, "Connecting...", sizeof(g_dash_state.last_action));
            }
            g_dash_state.wifi_signal = 0; // 0 draws the strike
            g_dash_state.internet_connected = false;
        }
        vTaskDelay(pdMS_TO_TICKS(5000)); // check every 5 seconds
    }
}

void setup() {
    // Wait for the USB host to finish enumerating and the power rail to
    // stabilise before starting peripherals. This prevents the concurrent
    // current draw (LCD + SD + LED + WiFi radio) from causing a voltage sag
    // that trips the brownout detector into a reset loop.
    delay(500);

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

    stick_log_printf("[main] Storage and USB initialized.\n");

    // Load configuration from SD card
    CloudConfig config = load_config_from_sd();

    // Connect to WiFi (Asynchronous).
    // Extra delay before starting the WiFi radio — its startup current spike
    // (~240 mA) is the largest single draw and is staggered here so it doesn't
    // overlap with USB enumeration.
    delay(300);
    if (config.is_valid && config.ssid.length() > 0) {
        stick_log_printf("[main] Connecting to WiFi: %s (async)\n", config.ssid.c_str());
        WiFi.begin(config.ssid.c_str(), config.wifi_password.c_str());
        // ESP32 WiFi will connect in the background and auto-reconnect.
        // The dashboard will show the disconnected state until it successfully connects.
    } else {
        stick_log_printf("[main] Skipping WiFi connection.\n");
    }

    // Start background internet checker
    xTaskCreate(internet_check_task, "InternetCheck", 4096, NULL, 1, NULL);

    if (config.is_valid) {
        // Instantiate the cloud client and start the sync engine
        CloudClient* cloud_client = new WebDAVClient(config.server_url, config.username, config.password);
        start_sync_engine(cloud_client, config);
    }

    // Initialize the static dashboard elements
    lcd_dashboard_init();

    // Set initial values
    g_dash_state.sd_mounted = true;
    if (config.is_valid && config.ssid.length() > 0) {
        strcpy(g_dash_state.last_action, "Connecting...");
    } else {
        strcpy(g_dash_state.last_action, "Ready.");
    }
    lcd_dashboard_update(g_dash_state);

    stick_log_printf("[main] Setup complete.\n");
}

void loop() {
    static unsigned long last_update = 0;
    static unsigned long upload_start_time = 0;
    static String last_logged_action = "";
    
    // Redraw the dashboard every 500ms
    if (millis() - last_update > 500) {
        last_update = millis();

        // Watchdog: If an upload gets stuck due to the server silently dropping the 
        // connection (e.g. 413 Payload Too Large on large files), the TCP window fills up 
        // and LwIP blocks forever. We detect a frozen progress string and forcefully reset WiFi.
        if (strncmp(g_dash_state.last_action, "Uploading", 9) == 0) {
            if (last_logged_action != g_dash_state.last_action) {
                upload_start_time = millis();
                last_logged_action = g_dash_state.last_action;
            } else if (millis() - upload_start_time > 30000) { // 30 seconds stuck on exact same string
                stick_log_printf("[watchdog] Upload stuck for 30s! Forcing disconnect to unblock.\n");
                WiFi.disconnect(false);
                WiFi.reconnect();
                upload_start_time = millis(); // Reset
            }
        } else {
            last_logged_action = g_dash_state.last_action;
            upload_start_time = millis();
        }

        // Draw the updated state to the LCD
        lcd_dashboard_update(g_dash_state);
    }
}