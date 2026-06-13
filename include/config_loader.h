#pragma once

#include <Arduino.h>

struct CloudConfig {
    bool is_valid;
    String ssid;
    String wifi_password;
    String server_url;
    String username;
    String password;
    uint32_t sync_interval_s;
    uint32_t settle_time_s;
    uint32_t max_file_size_mb; // 0 = no limit
};

// Loads the configuration from the SD card.
// If the config file does not exist, a template is created.
// Returns a populated CloudConfig struct.
CloudConfig load_config_from_sd();
