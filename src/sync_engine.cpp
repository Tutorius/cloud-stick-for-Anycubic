#include "sync_engine.h"
#include "logger.h"
#include "SD_MMC.h"
#include "lcd.h"
#include <map>

// Global dashboard state defined in main.cpp
extern DashboardState g_dash_state;

struct FileState {
    size_t last_size;
    unsigned long last_changed_time; // millis() when size was last observed to change
    bool uploaded;
};

// Keep state in memory for now. 
// A robust implementation would save this map to /.cloud-stick/sync_state.json
static std::map<String, FileState> _file_states;

SyncEngine::SyncEngine(CloudClient* client, const CloudConfig& config) 
    : _client(client), _config(config) {}

SyncEngine::~SyncEngine() {}

void SyncEngine::begin() {
    stick_log_printf("[sync] SyncEngine starting. Interval: %lu s, Settle: %lu s\n", 
        _config.sync_interval_s, _config.settle_time_s);
}

void SyncEngine::run() {
    // 1. Check server connection
    bool connected = _client->check_connection();
    g_dash_state.server_connected = connected;

    if (!connected) {
        stick_log_printf("[sync] Server not reachable. Skipping sync cycle.\n");
        return;
    }

    // 2. Scan the SD card
    stick_log_printf("[sync] Scanning SD card for changes...\n");
    scan_directory("/");

    // 3. Upload pending files
    upload_pending_files();
}

void SyncEngine::scan_directory(const char* dir_path) {
    File root = SD_MMC.open(dir_path);
    if (!root || !root.isDirectory()) {
        stick_log_printf("[sync] Failed to open directory: %s\n", dir_path);
        return;
    }

    File file = root.openNextFile();
    while (file) {
        String fileName = file.name();
        String fullPath = String(dir_path);
        if (!fullPath.endsWith("/")) fullPath += "/";
        fullPath += fileName;

        // Skip hidden files, system volume info, and our cloud-stick folder
        if (fileName.startsWith(".") || fileName == "System Volume Information") {
            file = root.openNextFile();
            continue;
        }

        if (file.isDirectory()) {
            scan_directory(fullPath.c_str());
        } else {
            size_t size = file.size();
            check_file(fullPath, size);
        }
        file = root.openNextFile();
    }
}

void SyncEngine::check_file(const String& path, size_t current_size) {
    auto it = _file_states.find(path);
    
    if (it == _file_states.end()) {
        // New file
        FileState fs;
        fs.last_size = current_size;
        fs.last_changed_time = millis();
        fs.uploaded = false;
        _file_states[path] = fs;
        stick_log_printf("[sync] Tracking new file: %s (%u bytes)\n", path.c_str(), current_size);
    } else {
        FileState& fs = it->second;
        if (fs.last_size != current_size) {
            // File is changing (being written to by USB Host)
            fs.last_size = current_size;
            fs.last_changed_time = millis();
            fs.uploaded = false;
            stick_log_printf("[sync] File size changing: %s (%u bytes)\n", path.c_str(), current_size);
        }
    }
}

void SyncEngine::upload_pending_files() {
    std::vector<String> pending_uploads;
    unsigned long settle_ms = _config.settle_time_s * 1000;

    for (auto& pair : _file_states) {
        FileState& fs = pair.second;
        if (!fs.uploaded && (millis() - fs.last_changed_time > settle_ms)) {
            pending_uploads.push_back(pair.first);
        }
    }

    g_dash_state.sync_total_files = pending_uploads.size();
    if (g_dash_state.sync_total_files == 0) {
        return;
    }

    g_dash_state.is_syncing = true;
    for (size_t i = 0; i < pending_uploads.size(); ++i) {
        g_dash_state.sync_current_file = i + 1;
        const String& path = pending_uploads[i];
        FileState& fs = _file_states[path];

        stick_log_printf("[sync] File settled: %s. Uploading...\n", path.c_str());
        strncpy(g_dash_state.last_action, "Uploading...", sizeof(g_dash_state.last_action));
        
        String remote_path = path.startsWith("/") ? path.substring(1) : path;

        unsigned long start_time = millis();
        bool success = _client->upload_file(path.c_str(), remote_path.c_str());
        unsigned long duration = millis() - start_time;

        if (success) {
            fs.uploaded = true;
            if (duration > 0) {
                uint32_t speed_kbps = (fs.last_size * 1000) / duration / 1024;
                g_dash_state.upload_speed_kbps = speed_kbps;
            }
            stick_log_printf("[sync] Upload successful: %s\n", path.c_str());
            strncpy(g_dash_state.last_action, "Upload OK", sizeof(g_dash_state.last_action));
        } else {
            stick_log_printf("[sync] Upload failed: %s\n", path.c_str());
            strncpy(g_dash_state.last_action, "Upload Failed", sizeof(g_dash_state.last_action));
        }
    }
    g_dash_state.is_syncing = false;
    g_dash_state.sync_total_files = 0;
    g_dash_state.sync_current_file = 0;
}

// FreeRTOS Task
static void sync_task_func(void* pvParameters) {
    SyncEngine* engine = static_cast<SyncEngine*>(pvParameters);
    engine->begin();

    // Since _config is copied into engine, we don't have direct access.
    // Let's assume a default delay, but we'll wake up more frequently to check settle times.
    // E.g., wake up every 2 seconds to check if files are ready to upload.
    while (1) {
        engine->run();
        // Delay for a short period before scanning again. 
        // In a real app we might only do check_connection() every sync_interval, 
        // but scan SD card more frequently. For now we sleep 5s.
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}

void start_sync_engine(CloudClient* client, const CloudConfig& config) {
    // Create the engine on the heap so it lives forever
    SyncEngine* engine = new SyncEngine(client, config);
    // Stack size 8192 since HTTPClient can use significant stack
    xTaskCreate(sync_task_func, "SyncTask", 8192, engine, 1, NULL);
}
