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
    ProgressStream(File& file, size_t totalSize) : _file(file), _totalSize(totalSize), _bytesRead(0), _lastUpdate(0), _lastBytesRead(0) {}
    
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
    size_t _lastBytesRead;
    
    void update_progress() {
        unsigned long now = millis();
        if (now - _lastUpdate > 500) { // Update at most every 500ms
            unsigned long duration = now - _lastUpdate;
            size_t bytes_diff = _bytesRead - _lastBytesRead;
            _lastUpdate = now;
            _lastBytesRead = _bytesRead;

            if (duration > 0) {
                g_dash_state.upload_speed_kbps = (bytes_diff * 1000) / duration / 1024;
            }

            if (_totalSize > 0) {
                int percent = (int)(((uint64_t)_bytesRead * 100) / _totalSize);
                g_dash_state.sync_progress_percent = percent;
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

class ProgressWriteStream : public Stream {
public:
    ProgressWriteStream(File& file, size_t totalSize) : _file(file), _totalSize(totalSize), _bytesWritten(0), _lastUpdate(0), _lastBytesWritten(0) {}
    
    int available() override { return 0; }
    int read() override { return -1; }
    int peek() override { return -1; }
    void flush() override { _file.flush(); }
    
    size_t write(uint8_t b) override {
        size_t n = _file.write(b);
        _bytesWritten += n;
        update_progress();
        return n;
    }
    
    size_t write(const uint8_t *buffer, size_t size) override {
        size_t n = _file.write(buffer, size);
        _bytesWritten += n;
        update_progress();
        return n;
    }
    
private:
    File& _file;
    size_t _totalSize;
    size_t _bytesWritten;
    unsigned long _lastUpdate;
    size_t _lastBytesWritten;
    
    void update_progress() {
        unsigned long now = millis();
        if (now - _lastUpdate > 500) {
            unsigned long duration = now - _lastUpdate;
            size_t bytes_diff = _bytesWritten - _lastBytesWritten;
            _lastUpdate = now;
            _lastBytesWritten = _bytesWritten;

            if (duration > 0) {
                g_dash_state.download_speed_kbps = (bytes_diff * 1000) / duration / 1024;
            }

            if (_totalSize > 0) {
                int percent = (int)(((uint64_t)_bytesWritten * 100) / _totalSize);
                g_dash_state.sync_progress_percent = percent;
                if (g_dash_state.sync_total_files > 0) {
                    snprintf(g_dash_state.last_action, sizeof(g_dash_state.last_action), 
                             "Dn %d%% (%d/%d)", percent, g_dash_state.sync_current_file, g_dash_state.sync_total_files);
                } else {
                    snprintf(g_dash_state.last_action, sizeof(g_dash_state.last_action), "Downloading %d%%", percent);
                }
            }
        }
    }
};

String WebDAVClient::urlDecode(const String& str) {
    String ret = "";
    char temp[] = "0x00";
    for (size_t i=0; i<str.length(); i++) {
        if (str[i] == '%') {
            if (i+2 < str.length()) {
                temp[2] = str[i+1];
                temp[3] = str[i+2];
                ret += (char)strtol(temp, NULL, 16);
                i += 2;
            }
        } else if (str[i] == '+') {
            ret += ' ';
        } else {
            ret += str[i];
        }
    }
    return ret;
}

class XMLParserStream : public Stream {
public:
    std::vector<RemoteFile>& files;
    String currentText;
    String currentTag;
    bool inTag;
    RemoteFile currentFile;
    bool inResponse;
    bool isCollection;
    
    XMLParserStream(std::vector<RemoteFile>& f) : files(f), inTag(false), inResponse(false), isCollection(false) {}
    
    int available() override { return 0; }
    int read() override { return -1; }
    int peek() override { return -1; }
    void flush() override {}
    
    size_t write(const uint8_t *buffer, size_t size) override {
        for(size_t i=0; i<size; i++) {
            write(buffer[i]);
        }
        return size;
    }

    size_t write(uint8_t b) override {
        char c = (char)b;
        if (c == '<') {
            inTag = true;
            // Only process text if the previous tag was an opening tag (not containing "</")
            if (inResponse && currentTag.indexOf("</") == -1) {
                currentText.trim(); // Trim whitespace just in case
                if (currentTag.endsWith("href>")) {
                    currentFile.name = currentText;
                } else if (currentTag.endsWith("getcontentlength>")) {
                    currentFile.size = currentText.toInt();
                } else if (currentTag.endsWith("collection/>") || currentTag.endsWith("collection>")) {
                    isCollection = true;
                }
            }
            currentText = "";
            currentTag = "<";
        } else if (c == '>') {
            inTag = false;
            currentTag += '>';
            
            if (currentTag.endsWith("response>")) {
                if (currentTag.indexOf("/") != -1) { // </d:response>
                    if (!isCollection) {
                        currentFile.name = WebDAVClient::urlDecode(currentFile.name);
                        int lastSlash = currentFile.name.lastIndexOf('/');
                        if (lastSlash != -1) {
                            currentFile.name = currentFile.name.substring(lastSlash + 1);
                        }
                        if (currentFile.name.length() > 0) {
                            files.push_back(currentFile);
                        }
                    }
                    inResponse = false;
                } else { // <d:response>
                    inResponse = true;
                    currentFile = RemoteFile();
                    isCollection = false;
                }
            }
        } else {
            if (inTag) {
                currentTag += c;
            } else {
                currentText += c;
            }
        }
        return 1;
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

bool WebDAVClient::list_files(const char* remote_dir, std::vector<RemoteFile>& files) {
    String url = _server_url;
    if (!url.endsWith("/")) {
        url += "/";
    }
    
    String encodedPath = "";
    const char* p = remote_dir;
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
    url += encodedPath;

    stick_log_printf("[webdav] Listing files at %s\n", url.c_str());

    HTTPClient http;
    http.setTimeout(20000);
    http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
    http.begin(url);
    http.setAuthorization(_username.c_str(), _password.c_str());
    http.addHeader("Depth", "1");
    
    String propfindXML = "<?xml version=\"1.0\"?>\n"
                         "<d:propfind xmlns:d=\"DAV:\">\n"
                         "  <d:prop>\n"
                         "    <d:getlastmodified/>\n"
                         "    <d:getcontentlength/>\n"
                         "    <d:resourcetype/>\n"
                         "  </d:prop>\n"
                         "</d:propfind>";

    int httpCode = http.sendRequest("PROPFIND", propfindXML);
    bool success = false;
    
    if (httpCode == 207) {
        XMLParserStream parser(files);
        http.writeToStream(&parser);
        success = true;
        stick_log_printf("[webdav] Found %d files.\n", files.size());
    } else {
        stick_log_printf("[webdav] PROPFIND failed. HTTP Code: %d\n", httpCode);
    }
    http.end();
    return success;
}

bool WebDAVClient::download_file(const char* remote_path, const char* local_path) {
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
    String url = _server_url;
    if (!url.endsWith("/")) url += "/";
    url += encodedPath;

    stick_log_printf("[webdav] Downloading %s to %s\n", url.c_str(), local_path);

    HTTPClient http;
    http.setTimeout(30000);
    http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
    http.begin(url);
    http.setAuthorization(_username.c_str(), _password.c_str());

    // We can use a GET request
    const char * headerKeys[] = {"Content-Length"};
    http.collectHeaders(headerKeys, 1);
    
    int httpCode = http.GET();
    bool success = false;

    if (httpCode == HTTP_CODE_OK) {
        File file = SD_MMC.open(local_path, FILE_WRITE);
        if (file) {
            size_t totalSize = 0;
            if (http.hasHeader("Content-Length")) {
                totalSize = http.header("Content-Length").toInt();
            }
            
            ProgressWriteStream progressStream(file, totalSize);
            unsigned long start_time = millis();
            int bytesWritten = http.writeToStream(&progressStream);
            unsigned long duration = millis() - start_time;
            
            file.close();
            
            if (bytesWritten > 0) {
                success = true;
                if (duration > 0) {
                    uint32_t speed_kbps = (bytesWritten * 1000) / duration / 1024;
                    g_dash_state.download_speed_kbps = speed_kbps;
                }
                stick_log_printf("[webdav] Download success! (%d bytes)\n", bytesWritten);
            } else {
                stick_log_printf("[webdav] Download failed to write any bytes.\n");
            }
        } else {
            stick_log_printf("[webdav] Failed to open local file for write: %s\n", local_path);
        }
    } else {
        stick_log_printf("[webdav] GET failed. HTTP Code: %d, Error: %s\n", httpCode, http.errorToString(httpCode).c_str());
    }

    http.end();
    return success;
}
