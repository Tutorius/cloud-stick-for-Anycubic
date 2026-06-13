#include "config_loader.h"
#include <ArduinoJson.h>
#include "SD_MMC.h"
#include "usb_storage.h"
#include "logger.h"

static const char* CONFIG_DIR = "/.cloud-stick";
static const char* CONFIG_FILENAME = "/.cloud-stick/config.json";
static const char* TEMPLATE_SSID = "YOUR_SSID";
static const char* TEMPLATE_PASS = "YOUR_PASSWORD";
static const char* TEMPLATE_SERVER = "https://example.com/webdav/";

CloudConfig load_config_from_sd() {
    CloudConfig config;
    config.is_valid = false;
    config.ssid = "";
    config.wifi_password = "";
    config.server_url = "";
    config.username = "";
    config.password = "";
    config.sync_interval_s = 60;
    config.settle_time_s = 2;

    // Ensure directory exists
    if (!SD_MMC.exists(CONFIG_DIR)) {
        stick_log_printf("[config] Directory %s not found. Creating...\n", CONFIG_DIR);
        SD_MMC.mkdir(CONFIG_DIR);
    }

    // Check if the file exists
    if (!SD_MMC.exists(CONFIG_FILENAME)) {
        stick_log_printf("[config] %s not found. Creating template...\n", CONFIG_FILENAME);
        
        File file = SD_MMC.open(CONFIG_FILENAME, FILE_WRITE);
        if (!file) {
            stick_log_printf("[config] Failed to create template file!\n");
            return config;
        }

        JsonDocument doc;
        doc["ssid"] = TEMPLATE_SSID;
        doc["wifi_password"] = TEMPLATE_PASS;
        doc["server_url"] = TEMPLATE_SERVER;
        doc["username"] = "user";
        doc["password"] = "pass";
        doc["sync_interval_s"] = 60;
        doc["settle_time_s"] = 2;
        
        if (serializeJson(doc, file) == 0) {
            stick_log_printf("[config] Failed to write to template file\n");
        }
        file.close();
        stick_log_printf("[config] Template file created. Please update it to connect.\n");
        return config; 
    }

    // Open file for reading
    File file = SD_MMC.open(CONFIG_FILENAME, FILE_READ);
    if (!file) {
        stick_log_printf("[config] Failed to open config file for reading\n");
        return config;
    }

    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, file);
    if (error) {
        stick_log_printf("[config] deserializeJson() failed: %s\n", error.c_str());
        file.close();
        return config;
    }
    file.close();

    // Extract values
    config.ssid = doc["ssid"] | "";
    config.wifi_password = doc["wifi_password"] | "";
    config.server_url = doc["server_url"] | "";
    config.username = doc["username"] | "";
    config.password = doc["password"] | "";
    config.sync_interval_s = doc["sync_interval_s"] | 60;
    config.settle_time_s = doc["settle_time_s"] | 2;

    // Check if the user has changed the template
    if (config.ssid == TEMPLATE_SSID || config.ssid.length() == 0 || config.server_url == TEMPLATE_SERVER) {
        stick_log_printf("[config] Config still contains template values. Skipping Cloud connection.\n");
        config.is_valid = false;
    } else {
        stick_log_printf("[config] Cloud config loaded successfully.\n");
        config.is_valid = true;
    }

    return config;
}
