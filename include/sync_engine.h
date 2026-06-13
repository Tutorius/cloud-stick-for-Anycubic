#pragma once

#include "cloud_client.h"
#include "config_loader.h"

class SyncEngine {
public:
    SyncEngine(CloudClient* client, const CloudConfig& config);
    ~SyncEngine();

    /**
     * @brief Start the background synchronization task.
     */
    void begin();

    /**
     * @brief Call this from loop() or let the FreeRTOS task run it. 
     * Since we are using FreeRTOS, the task will just loop this internally.
     */
    void run();

private:
    CloudClient* _client;
    CloudConfig _config;

    void scan_directory(const char* dir_path);
    void check_file(const String& path, size_t current_size);
    void upload_pending_files();
};

// Global function to start the sync engine task
void start_sync_engine(CloudClient* client, const CloudConfig& config);
