#pragma once

#include <Arduino.h>

struct WifiConfig {
    bool is_valid; // true if config was loaded and doesn't contain template values
    String ssid;
    String password;
};

// Loads the WiFi configuration from the SD card.
// If the config file does not exist, a template is created.
// Returns a populated WifiConfig struct.
WifiConfig load_config_from_sd();
