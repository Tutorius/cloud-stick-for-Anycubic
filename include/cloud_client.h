#pragma once

#include <Arduino.h>
#include <vector>

struct RemoteFile {
    String name;
    size_t size;
    unsigned long last_modified; // Or we can use a String for simplicity, but size is usually enough to detect changes.
};

class CloudClient {
public:
    virtual ~CloudClient() {}

    /**
     * @brief Checks if the remote server is reachable and credentials are valid.
     */
    virtual bool check_connection() = 0;

    /**
     * @brief Uploads a file from the local SD card to the remote server.
     */
    virtual bool upload_file(const char* local_path, const char* remote_path) = 0;

    /**
     * @brief Lists files in a remote directory.
     * @param remote_dir The directory to list (e.g., "/" or empty for root)
     * @param files Vector to be populated with the list of remote files.
     * @return true on success, false on failure.
     */
    virtual bool list_files(const char* remote_dir, std::vector<RemoteFile>& files) = 0;

    /**
     * @brief Downloads a file from the remote server to the local SD card.
     */
    virtual bool download_file(const char* remote_path, const char* local_path) = 0;
};
