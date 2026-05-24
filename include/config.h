#pragma once

// ─────────────────────────────────────────────────────────────────────────────
// config.h — project-wide configuration
// ─────────────────────────────────────────────────────────────────────────────

// ── Hardware ──────────────────────────────────────────────────────────────────
#define PIN_TRIGGER     0    // BOOT button (active LOW)
#define PIN_NEOPIXEL   48    // WS2812B RGB LED on ESP32-S3-DevKitC-1
                             // NOT a plain GPIO — requires NeoPixel library
#define NEOPIXEL_COUNT  1    // one onboard LED

// ── NeoPixel colours (R, G, B) ────────────────────────────────────────────────
#define LED_COLOR_BOOT    20,   0,   0    // red   — 3 flashes at boot/trigger
#define LED_COLOR_DATA     0,   0,  20    // blue  — flashes while working
#define LED_COLOR_DONE     0,  20,   0    // green — 3 flashes when complete
#define LED_COLOR_OFF      0,   0,   0    // off

// ── USB MSC identity ──────────────────────────────────────────────────────────
#define MSC_VENDOR_ID   "Espressif"
#define MSC_PRODUCT_ID  "ESP32"
#define MSC_PRODUCT_REV "1.0"

// ── Timing (milliseconds) ─────────────────────────────────────────────────────
#define DEBOUNCE_MS           50
#define RUN_DIALOG_DELAY_MS  800
#define CMD_POWERSHELL_MS   3000
#define CMD_GENERIC_MS      1000
#define COMPLETION_WAIT_MS  30000
#define LED_FLASH_MS         200

// ── Flash / MSC geometry ──────────────────────────────────────────────────────
#define FLASH_ERASE_SIZE    4096
#define MSC_SECTOR_SIZE      512


// File-complete detection:
// Green fires when >= FILE_DONE_THRESHOLD bytes have been written to the data
// area AND FILE_DONE_SETTLE_MS of write silence follows. The threshold rules
// out false triggers from early write pauses (e.g. during systeminfo execution).
// Tune FILE_DONE_THRESHOLD if collect.ps1 output size changes significantly.
#define FILE_DONE_THRESHOLD   35000   // bytes — tune if collect.ps1 output size changes
#define FILE_DONE_SETTLE_MS    5000   // ms of write silence after threshold

// First LBA of the FAT16 data area (past reserved sectors + FATs + root dir).
// Writes at LBA >= this value are file data, not filesystem metadata.
// Geometry: Reserved=4, FAT=80 sectors x2, RootDir=32 → data starts at 196.
#define MSC_DATA_START_LBA  196