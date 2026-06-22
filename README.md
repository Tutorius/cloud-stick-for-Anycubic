# Cloud Stick

An ESP32-S3 powered USB flash drive that automatically syncs its contents with a WebDAV cloud server over WiFi. When plugged into a computer, it mounts as a standard USB mass storage device, but in the background, it continuously uploads new or modified files to a WebDAV server and downloads any changes from the server to the SD card.

Designed for the **LilyGO T-Dongle-S3** (or any ESP32-S3 with an SD card slot and an ST7735 display). [Requirements](#requirements) and [setup instructions](#getting-started) are below.

## Features

- **Plug-and-play USB Mass Storage**: Appears as a normal thumb drive to the host PC.
- **Two-way WebDAV Sync**: Uploads new/modified local files and downloads remote changes.
- **LCD Dashboard**: Shows WiFi/server status, current sync action, and live transfer speeds.
- **Offline Resilience**: Sync state is saved to the SD card, allowing it to survive reboots and connection drops.

## Table of Contents

<detail>
  <summary>Click to expand</summary>

- [Cloud Stick](#cloud-stick)
  - [Features](#features)
  - [Table of Contents](#table-of-contents)
  - [Getting Started](#getting-started)
    - [Requirements](#requirements)
      - [Hardware](#hardware)
    - [1. Build and Flash](#1-build-and-flash)
    - [2. Configuration](#2-configuration)
      - [WebDAV URL](#webdav-url)
    - [3. Usage](#3-usage)
  - [In-Depth Technical Details](#in-depth-technical-details)
    - [USB Mass Storage (MSC) vs WiFi](#usb-mass-storage-msc-vs-wifi)
    - [The Sync Engine (`sync_engine.cpp`)](#the-sync-engine-sync_enginecpp)
    - [WebDAV and Memory Limits (`webdav_client.cpp`)](#webdav-and-memory-limits-webdav_clientcpp)
    - [Live Progress \& Watchdogs](#live-progress--watchdogs)
  - [License \& Acknowledgements](#license--acknowledgements)

</detail>


## Getting Started

### Requirements

#### Hardware

- LilyGO T-Dongle-S3 (or any ESP32-S3 with an SD card slot and an ST7735 display)
- MicroSD card (FAT32 formatted)

> You can find them pretty cheaply on AliExpress (10€-20€), just search for `LILYGO® T-Dongle-S3 ESP32-S3`
> But they also have an [official store](https://lilygo.cc/products/t-dongle-s3).
>
>Make sure to get the version with the SD card slot and ST7735 display.

### 1. Build and Flash

This project is built using [PlatformIO](https://platformio.org/). Clone the repository:

```bash
git clone https://github.com/JMcrafter26/cloud-stick.git
cd cloud-stick
```

And then build and flash the firmware to the ESP32-S3:

```bash
pio run -t upload
```

or click the "Upload" button in VSCode with the PlatformIO extension installed.

> [!NOTE]
> If you encounter any issues, make sure PlatformIO installed the required libraries mentioned in `platformio.ini`. If you get errors about missing libraries, run: `pio lib install`

### 2. Configuration

The stick looks for a configuration file on the SD card.

> The following JSON file will be created automatically on first boot, but you can also create it manually. Place it at `/.cloud-stick/config.json` on the SD card.

```json
{
    "ssid": "Your_WiFi_Name",
    "wifi_password": "Your_WiFi_Password",
    "server_url": "http://192.168.1.100/dav.php/@Home", # This is the WebDAV URL of your server. (See below for examples)
    "username": "webdav_user",
    "password": "webdav_password",
    "sync_interval_s": 60, # how often to check for changes (in seconds)
    "settle_time_s": 2, # how long to wait before considering a file safe to upload
    "max_file_size_mb": 50
}
```

#### WebDAV URL

The `server_url` varies depending on the WebDAV server software you are using.

For example, Nextcloud uses `https://yourdomain.com/remote.php/dav/files/username/` and FileRun uses `http://yourdomain.com/dav.php/@Home`. Check your server's documentation for the correct URL.

> Also, make sure the WebDAV server and path is accessible from the ESP32-S3's WiFi network

### 3. Usage

Plug it into a USB port. The dashboard will show the boot sequence, connect to WiFi, and immediately begin syncing. You can drag and drop files onto the drive normally.

---

## In-Depth Technical Details

If you're looking to modify the code or just curious about how it handles the edge cases of running a cloud sync engine on a microcontroller, here's how the internals work.

### USB Mass Storage (MSC) vs WiFi

The ESP32-S3 has a native USB peripheral. We use the `USBMSC` class to expose the SD card (via the SD_MMC driver) directly to the host PC. Because both the host PC and the ESP32 need to access the SD card simultaneously, we rely on the `SD_MMC` library's sector-level arbitration.

> I used the https://github.com/Xinyuan-LilyGO/T-Dongle-S3 example as a reference for the USB MSC implementation. It uses the `USBMSC` class from the `TinyUSB` library.

### The Sync Engine (`sync_engine.cpp`)

The sync loop runs in a dedicated FreeRTOS task. To avoid uploading a file while the host PC is still writing to it, the engine uses a "double-scan" approach with a settle time:

1. Scan the SD card and record the sizes of all files.
2. Wait `settle_time_s` (e.g., 2 seconds).
3. Scan again. If a file's size hasn't changed during the settle window, it's considered "safe" to upload.

Sync state is persisted in `/.cloud-stick/sync_state.json`. If you unplug the stick mid-upload, it will seamlessly pick up where it left off on the next boot.

### WebDAV and Memory Limits (`webdav_client.cpp`)

WebDAV uses XML for its `PROPFIND` responses. A typical directory listing from a server like Nextcloud or FileRun can be massive—far exceeding the ESP32's available RAM.

To solve this, the `XMLParserStream` parses the XML directly off the TCP socket. It extracts filenames and file sizes on the fly byte-by-byte without ever buffering the whole XML payload.

> **Note:** I tested it with FileRun and Nextcloud servers but other WebDav servers should work as long as they don't require any non-standard authentication or headers. [See the WebDAV spec](https://datatracker.ietf.org/doc/html/rfc4918) for details (really technical stuff).

### Live Progress & Watchdogs

Because WebDAV servers often sit behind proxies (like Nginx) with strict `client_max_body_size` limits, uploading a file that is too large can cause the server to drop the connection unexpectedly.
If the server drops the connection but keeps the socket in a weird state, the ESP32's `WiFiClientSecure` can deadlock while trying to push data into a full TCP window.

To prevent the sync engine from freezing:

1. The `HTTPClient` is fed via a custom `ProgressStream` that calculates upload/download speeds every 500ms and updates the LCD dashboard.
2. The main display loop includes a software watchdog. If it detects the upload percentage is frozen for exactly 30 seconds, it forcefully restarts the WiFi interface, cleanly tearing down the hung socket and allowing the sync engine to log a failure and move on.

## License & Acknowledgements

This project is licensed under the MIT License. See the [LICENSE](LICENSE) file for details.

**Acknowledgements:**

- **fastled/FastLED** - for the RGB LED status indicators
- **bodmer/TFT_eSPI** - for the TFT display library
- **bblanchon/ArduinoJson** - for the JSON parsing library
- **espressif/arduino-esp32** - for the ESP32 Arduino core
- **Xinyuan-LilyGO/T-Dongle-S3** - for the reference implementation and product/reference materials of the T-Dongle-S3
- **tinyusb-org/tinyusb** - for the USB stack
- **sbrin/lopaka** - for the dashboard icons and design
- Inspiration from various WebDAV client implementations and the WebDAV spec (RFC 4918) for handling edge cases in file uploads and directory listings.
