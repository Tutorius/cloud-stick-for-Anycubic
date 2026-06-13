#pragma once

#include "cloud_client.h"
#include "config_loader.h"

class SyncEngine {
public:
    SyncEngine(CloudClient* client, const CloudConfig& config);
    ~SyncEngine();

    /**
     * @brief Load persisted state and log startup info.
     */
    void begin();

    // Returns sync interval in milliseconds (min 5 seconds to avoid hammering server)
    unsigned long get_sync_interval_ms() const {
        unsigned long ms = _config.sync_interval_s * 1000UL;
        return ms < 5000 ? 5000 : ms;
    }

    /**
     * @brief Run one full sync cycle.
     * @return true if a complete sync was performed (WiFi + server reachable),
     *         false if skipped due to no connectivity.
     */
    bool run();

private:
    CloudClient* _client;
    CloudConfig _config;

    void scan_directory(const char* dir_path);
    void check_file(const String& path, size_t current_size);
    void upload_pending_files();
    void download_remote_files();

    void save_state();
    void load_state();
};

// Global function to start the sync engine task
void start_sync_engine(CloudClient* client, const CloudConfig& config);
