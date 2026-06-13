#include "sync_engine.h"
#include "logger.h"
#include "SD_MMC.h"
#include "lcd.h"
#include <WiFi.h>
#include <map>
#include <vector>

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
    // 1. Wait for WiFi to be connected before attempting anything cloud-related.
    if (WiFi.status() != WL_CONNECTED) {
        stick_log_printf("[sync] WiFi not connected yet. Waiting...\n");
        return;
    }

    // 2. Check server connection
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

    // 4. Download remote files
    download_remote_files();
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
    // Skip files that exceed the max size limit
    if (_config.max_file_size_mb > 0 && current_size > (size_t)_config.max_file_size_mb * 1024 * 1024) {
        stick_log_printf("[sync] Skipping (too large, %u MB limit): %s\n", _config.max_file_size_mb, path.c_str());
        return;
    }
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

void SyncEngine::download_remote_files() {
    std::vector<RemoteFile> remote_files;
    stick_log_printf("[sync] Listing remote files...\n");
    // Pass empty string as root — server_url already points to the user's home dir.
    // Passing "/" would result in a double-slash in the URL.
    if (!_client->list_files("", remote_files)) {
        return;
    }

    std::vector<RemoteFile> to_download;
    for (const auto& rf : remote_files) {
        if (rf.name.startsWith(".")) continue;

        // Skip files exceeding the max size limit
        if (_config.max_file_size_mb > 0 && rf.size > (size_t)_config.max_file_size_mb * 1024 * 1024) {
            stick_log_printf("[sync] Skipping remote (too large): %s\n", rf.name.c_str());
            continue;
        }

        String local_path = "/" + rf.name;
        
        // Does it exist locally?
        File f = SD_MMC.open(local_path.c_str(), FILE_READ);
        if (!f) {
            to_download.push_back(rf);
        } else {
            size_t local_size = f.size();
            f.close();
            
            // If we have a pending local change, DON'T OVERWRITE it!
            auto it = _file_states.find(local_path);
            if (it != _file_states.end() && !it->second.uploaded) {
                continue;
            }

            // Only download if sizes differ
            if (local_size != rf.size) {
                to_download.push_back(rf);
            }
        }
    }

    g_dash_state.sync_total_files = to_download.size();
    if (g_dash_state.sync_total_files == 0) {
        return;
    }

    g_dash_state.is_syncing = true;
    for (size_t i = 0; i < to_download.size(); ++i) {
        g_dash_state.sync_current_file = i + 1;
        const RemoteFile& rf = to_download[i];
        String local_path = "/" + rf.name;
        String remote_path = rf.name;

        stick_log_printf("[sync] Downloading: %s (%u bytes)\n", rf.name.c_str(), rf.size);
        strncpy(g_dash_state.last_action, "Downloading...", sizeof(g_dash_state.last_action));

        if (_client->download_file(remote_path.c_str(), local_path.c_str())) {
            // Register it in _file_states so we don't immediately re-upload it
            FileState fs;
            fs.last_size = rf.size;
            fs.last_changed_time = millis();
            fs.uploaded = true; // It's in sync with the cloud
            _file_states[local_path] = fs;
            
            strncpy(g_dash_state.last_action, "Download OK", sizeof(g_dash_state.last_action));
        } else {
            stick_log_printf("[sync] Download failed: %s\n", rf.name.c_str());
            strncpy(g_dash_state.last_action, "Download Failed", sizeof(g_dash_state.last_action));
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

    while (1) {
        // run() returns early if WiFi isn't connected yet.
        // Retry quickly (every 10s) until we get a successful sync,
        // then switch to the full interval.
        bool synced = false;
        while (!synced) {
            engine->run();
            // If WiFi + server connected we ran a full cycle
            if (WiFi.status() == WL_CONNECTED) {
                synced = true;
            } else {
                vTaskDelay(pdMS_TO_TICKS(10000)); // retry every 10s while waiting for WiFi
            }
        }
        // Full sync done — wait the configured interval before the next one
        vTaskDelay(pdMS_TO_TICKS(engine->get_sync_interval_ms()));
    }
}

void start_sync_engine(CloudClient* client, const CloudConfig& config) {
    // Create the engine on the heap so it lives forever
    SyncEngine* engine = new SyncEngine(client, config);
    // Stack size 8192 since HTTPClient can use significant stack
    xTaskCreate(sync_task_func, "SyncTask", 8192, engine, 1, NULL);
}
