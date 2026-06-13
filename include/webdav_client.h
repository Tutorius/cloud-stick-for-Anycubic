#pragma once

#include "cloud_client.h"
#include <HTTPClient.h>

class WebDAVClient : public CloudClient {
public:
    WebDAVClient(const String& server_url, const String& username, const String& password);
    virtual ~WebDAVClient();

    bool check_connection() override;
    bool upload_file(const char* local_path, const char* remote_path) override;

private:
    String _server_url;
    String _username;
    String _password;
    
    // Helper to properly encode URI paths
    String urlEncode(const char* msg);
};
