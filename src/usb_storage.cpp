#include "config.h"
#include "usb_storage.h"
#include <FastLED.h>
#include "USB.h"
#include "USBMSC.h"
#include "SD_MMC.h"

USBMSC MSC;
CRGB leds[1];

static uint32_t DISK_SECTOR_COUNT = 0;
static uint16_t DISK_SECTOR_SIZE = 0;

static int32_t onWrite(uint32_t lba, uint32_t offset, uint8_t *buffer, uint32_t bufsize) {
    uint32_t secSize = SD_MMC.sectorSize();
    if (!secSize) return 0;
    for (int x = 0; x < bufsize / secSize; x++) {
        uint8_t blkbuffer[secSize];
        memcpy(blkbuffer, (uint8_t *)buffer + secSize * x, secSize);
        if (!SD_MMC.writeRAW(blkbuffer, lba + x)) return 0;
    }
    return bufsize;
}

static int32_t onRead(uint32_t lba, uint32_t offset, void *buffer, uint32_t bufsize) {
    uint32_t secSize = SD_MMC.sectorSize();
    if (!secSize) return false;
    for (int x = 0; x < bufsize / secSize; x++) {
        if (!SD_MMC.readRAW((uint8_t *)buffer + (x * secSize), lba + x)) return 0;
    }
    return bufsize;
}

static bool onStartStop(uint8_t power_condition, bool start, bool load_eject) {
    Serial.printf("MSC START/STOP: power: %u, start: %u, eject: %u\n", power_condition, start, load_eject);
    leds[0] = CRGB::Red;
    FastLED.show();
    return true;
}

static void usbEventCallback(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data) {
    if (event_base == ARDUINO_USB_EVENTS) {
        arduino_usb_event_data_t *data = (arduino_usb_event_data_t *)event_data;
        switch (event_id) {
            case ARDUINO_USB_STARTED_EVENT: Serial.println("USB PLUGGED"); break;
            case ARDUINO_USB_STOPPED_EVENT: Serial.println("USB UNPLUGGED"); break;
            case ARDUINO_USB_SUSPEND_EVENT: Serial.printf("USB SUSPENDED: remote_wakeup_en: %u\n", data->suspend.remote_wakeup_en); break;
            case ARDUINO_USB_RESUME_EVENT:  Serial.println("USB RESUMED"); break;
            default: break;
        }
    }
}

void init_usb_storage() {
    FastLED.addLeds<APA102, LED_DI_PIN, LED_CI_PIN, BGR>(leds, 1);
    FastLED.setBrightness(25);

    SD_MMC.setPins(SD_MMC_CLK_PIN, SD_MMC_CMD_PIN, SD_MMC_D0_PIN, SD_MMC_D1_PIN, SD_MMC_D2_PIN, SD_MMC_D3_PIN);

    if (!SD_MMC.begin()) {
        leds[0] = CRGB::Red;
        FastLED.show();
        while (1) {
            Serial.println("SD Card Mount Failed");
            delay(1000);
        }
    }
    
    leds[0] = CRGB::Green;
    FastLED.show();

    USB.onEvent(usbEventCallback);
    MSC.vendorID("ESP32");       
    MSC.productID("USB_MSC");    
    MSC.productRevision("1.0");  
    MSC.onStartStop(onStartStop);
    MSC.onRead(onRead);
    MSC.onWrite(onWrite);
    MSC.mediaPresent(true);

    DISK_SECTOR_COUNT = SD_MMC.numSectors();
    DISK_SECTOR_SIZE =  SD_MMC.sectorSize();

    MSC.begin(DISK_SECTOR_COUNT, DISK_SECTOR_SIZE);
    USB.begin();
}