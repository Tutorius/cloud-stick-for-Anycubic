#include "logger.h"
#include "usb_storage.h"
#include "SD_MMC.h"
#include <stdarg.h>

static const char* LOG_FILENAME = "/.cloud-stick/stick.log";
static bool logger_ready = false;

void log_init() {
    if (!SD_MMC.exists("/.cloud-stick")) {
        SD_MMC.mkdir("/.cloud-stick");
    }
    
    // Test if we can open the file
    File f = SD_MMC.open(LOG_FILENAME, FILE_APPEND);
    if (f) {
        logger_ready = true;
        f.println();
        f.println("=== SYSTEM BOOT ===");
        f.close();
    } else {
        USBSerial.println("[logger] Failed to open log file.");
    }
}

void stick_log_printf(const char* format, ...) {
    char buf[256];
    va_list args;
    va_start(args, format);
    vsnprintf(buf, sizeof(buf), format, args);
    va_end(args);

    // Print to USB serial
    USBSerial.print(buf);

    // Print to file if ready
    if (logger_ready) {
        File f = SD_MMC.open(LOG_FILENAME, FILE_APPEND);
        if (f) {
            f.print(buf);
            f.close();
        }
    }
}
