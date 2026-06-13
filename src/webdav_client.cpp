#include "webdav_client.h"
#include "logger.h"
#include "SD_MMC.h"
#include <WiFiClientSecure.h>

WebDAVClient::WebDAVClient(const String& server_url, const String& username, const String& password)
    : _server_url(server_url), _username(username), _password(password) {
    if (!_server_url.endsWith("/")) {
        _server_url += "/";
    }
}

WebDAVClient::~WebDAVClient() {}

bool WebDAVClient::check_connection() {
    HTTPClient http;
    // We can use PROPFIND with depth 0 to check if the root URL is accessible.
    http.begin(_server_url);
    http.setAuthorization(_username.c_str(), _password.c_str());
    http.addHeader("Depth", "0");
    http.addHeader("Content-Type", "application/xml");

    // Some servers require a body for PROPFIND, but an empty body is often fine for depth 0.
    int httpCode = http.sendRequest("PROPFIND", "");
    
    bool success = false;
    // 207 Multi-Status is the expected success code for PROPFIND.
    if (httpCode == 207 || httpCode == 200) {
        success = true;
    } else {
        stick_log_printf("[webdav] Connection check failed. HTTP Code: %d\n", httpCode);
    }
    
    http.end();
    return success;
}

String WebDAVClient::urlEncode(const char* msg) {
    const char *hex = "0123456789ABCDEF";
    String encodedMsg = "";
    while (*msg != '\0') {
        if ( ('a' <= *msg && *msg <= 'z')
             || ('A' <= *msg && *msg <= 'Z')
             || ('0' <= *msg && *msg <= '9')
             || *msg == '-' || *msg == '_' || *msg == '.' || *msg == '~' ) {
            encodedMsg += *msg;
        } else {
            encodedMsg += '%';
            encodedMsg += hex[*msg >> 4];
            encodedMsg += hex[*msg & 15];
        }
        msg++;
    }
    return encodedMsg;
}

#include "lcd.h"

extern DashboardState g_dash_state;

class ProgressStream : public Stream {
public:
    ProgressStream(File& file, size_t totalSize) : _file(file), _totalSize(totalSize), _bytesRead(0), _lastUpdate(0) {}
    
    int available() override { return _file.available(); }
    
    int read() override { 
        int b = _file.read(); 
        if (b >= 0) {
            _bytesRead++;
            update_progress();
        }
        return b; 
    }
    
    int peek() override { return _file.peek(); }
    void flush() override { _file.flush(); }
    
    size_t write(uint8_t) override { return 0; }
    
    size_t readBytes(char *buffer, size_t length) override {
        size_t n = _file.read((uint8_t*)buffer, length);
        _bytesRead += n;
        update_progress();
        return n;
    }

    size_t readBytes(uint8_t *buffer, size_t length) override {
        size_t n = _file.read(buffer, length);
        _bytesRead += n;
        update_progress();
        return n;
    }
    
private:
    File& _file;
    size_t _totalSize;
    size_t _bytesRead;
    unsigned long _lastUpdate;
    
    void update_progress() {
        if (millis() - _lastUpdate > 500) { // Update at most every 500ms
            _lastUpdate = millis();
            if (_totalSize > 0) {
                int percent = (int)(((uint64_t)_bytesRead * 100) / _totalSize);
                if (g_dash_state.sync_total_files > 0) {
                    snprintf(g_dash_state.last_action, sizeof(g_dash_state.last_action), 
                             "Uploading %d%% (%d/%d)", percent, g_dash_state.sync_current_file, g_dash_state.sync_total_files);
                } else {
                    snprintf(g_dash_state.last_action, sizeof(g_dash_state.last_action), "Uploading %d%%", percent);
                }
            }
        }
    }
};

bool WebDAVClient::upload_file(const char* local_path, const char* remote_path) {
    File file = SD_MMC.open(local_path, FILE_READ);
    if (!file) {
        stick_log_printf("[webdav] Failed to open local file for upload: %s\n", local_path);
        return false;
    }

    size_t fileSize = file.size();
    
    // Construct the remote URL. 
    // remote_path is expected to NOT have a leading slash relative to _server_url.
    // e.g. remote_path = "Documents/test.txt"
    // We should urlencode the path segments.
    String encodedPath = "";
    const char* p = remote_path;
    while (*p != '\0') {
        if (*p == '/') {
            encodedPath += '/';
            p++;
        } else {
            const char* start = p;
            while (*p != '\0' && *p != '/') p++;
            String segment = String(start).substring(0, p - start);
            encodedPath += urlEncode(segment.c_str());
        }
    }

    String url = _server_url + encodedPath;
    
    stick_log_printf("[webdav] Uploading %s to %s (%lu bytes)...\n", local_path, url.c_str(), fileSize);

    HTTPClient http;
    // We don't verify SSL for simplicity, but in production we should.
    // For large uploads, ensure timeouts are large enough.
    http.setTimeout(30000); 
    http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
    http.begin(url);
    http.setAuthorization(_username.c_str(), _password.c_str());
    http.addHeader("Content-Type", "application/octet-stream");
    
    ProgressStream progressStream(file, fileSize);

    // We pass the Stream directly to HTTPClient. This is extremely memory efficient!
    int httpCode = http.sendRequest("PUT", &progressStream, fileSize);
    
    bool success = false;
    if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_CREATED || httpCode == 204) {
        stick_log_printf("[webdav] Upload success: %d\n", httpCode);
        success = true;
    } else {
        stick_log_printf("[webdav] Upload failed. HTTP Code: %d, Error: %s\n", httpCode, http.errorToString(httpCode).c_str());
    }

    http.end();
    file.close();
    return success;
}
