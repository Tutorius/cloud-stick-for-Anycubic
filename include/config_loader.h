#pragma once

#include <Arduino.h>

struct CloudConfig {
    bool is_valid; // true if config was loaded and doesn't contain template values
    String ssid;
    String wifi_password;
    String server_url;
    String username;
    String password;
    uint32_t sync_interval_s;
    uint32_t settle_time_s;
};

// Loads the configuration from the SD card.
// If the config file does not exist, a template is created.
// Returns a populated CloudConfig struct.
CloudConfig load_config_from_sd();
