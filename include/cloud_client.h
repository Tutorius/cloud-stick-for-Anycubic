#pragma once

#include <Arduino.h>

class CloudClient {
public:
    virtual ~CloudClient() {}

    /**
     * @brief Checks if the server is reachable and credentials are valid.
     * @return true if connected successfully, false otherwise.
     */
    virtual bool check_connection() = 0;

    /**
     * @brief Uploads a file to the cloud.
     * @param local_path The absolute path to the local file on the SD card.
     * @param remote_path The relative path on the cloud server.
     * @return true if upload was successful, false otherwise.
     */
    virtual bool upload_file(const char* local_path, const char* remote_path) = 0;
};
