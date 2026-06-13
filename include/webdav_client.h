#pragma once

#include "cloud_client.h"
#include <HTTPClient.h>

class WebDAVClient : public CloudClient {
public:
    WebDAVClient(const String& server_url, const String& username, const String& password);
    virtual ~WebDAVClient();

    bool check_connection() override;
    bool upload_file(const char* local_path, const char* remote_path) override;
    bool list_files(const char* remote_dir, std::vector<RemoteFile>& files) override;
    bool download_file(const char* remote_path, const char* local_path) override;

    // Public static so XMLParserStream (defined in webdav_client.cpp) can call it
    static String urlDecode(const String& str);

private:
    String _server_url;
    String _username;
    String _password;

    String urlEncode(const char* msg);
};

