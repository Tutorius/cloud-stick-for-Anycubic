#include "config_loader.h"
#include <ArduinoJson.h>
#include "SD_MMC.h"
#include "usb_storage.h"

static const char* CONFIG_FILENAME = "/config.json";
static const char* TEMPLATE_SSID = "YOUR_SSID";
static const char* TEMPLATE_PASS = "YOUR_PASSWORD";

WifiConfig load_config_from_sd() {
    WifiConfig config;
    config.is_valid = false;
    config.ssid = "";
    config.password = "";

    // Check if the file exists
    if (!SD_MMC.exists(CONFIG_FILENAME)) {
        USBSerial.printf("[config] %s not found. Creating template...\n", CONFIG_FILENAME);
        
        File file = SD_MMC.open(CONFIG_FILENAME, FILE_WRITE);
        if (!file) {
            USBSerial.println("[config] Failed to create template file!");
            return config;
        }

        JsonDocument doc;
        doc["ssid"] = TEMPLATE_SSID;
        doc["password"] = TEMPLATE_PASS;
        
        if (serializeJson(doc, file) == 0) {
            USBSerial.println("[config] Failed to write to template file");
        }
        file.close();
        USBSerial.println("[config] Template file created. Please update it to connect to WiFi.");
        return config; // Return invalid config so we don't connect using template values
    }

    // Open file for reading
    File file = SD_MMC.open(CONFIG_FILENAME, FILE_READ);
    if (!file) {
        USBSerial.println("[config] Failed to open config file for reading");
        return config;
    }

    // Allocate a JSON document
    // 512 bytes is plenty for a simple config
    JsonDocument doc;

    // Parse the JSON document
    DeserializationError error = deserializeJson(doc, file);
    if (error) {
        USBSerial.print("[config] deserializeJson() failed: ");
        USBSerial.println(error.c_str());
        file.close();
        return config;
    }

    file.close();

    // Extract values
    const char* ssid = doc["ssid"] | "";
    const char* password = doc["password"] | "";

    config.ssid = ssid;
    config.password = password;

    // Check if the user has changed the template
    if (config.ssid == TEMPLATE_SSID || config.ssid.length() == 0) {
        USBSerial.println("[config] SSID is still the template value. Skipping WiFi.");
        config.is_valid = false;
    } else {
        USBSerial.println("[config] WiFi config loaded successfully.");
        config.is_valid = true;
    }

    return config;
}
