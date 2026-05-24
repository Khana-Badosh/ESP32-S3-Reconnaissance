// ─────────────────────────────────────────────────────────────────────────────
// msc_drive.cpp — USB Mass Storage drive implementation
//
// SAFETY RULE: onWrite() does the minimum possible work — flash RMW only.
// All detection logic runs in loop() via mscDriveFileDone(), never in the
// USB task. Doing non-trivial work in USB callbacks causes timing violations
// that crash USB enumeration ("Unknown USB Device" on Windows).
// ─────────────────────────────────────────────────────────────────────────────

#include <Arduino.h>
#include "msc_drive.h"
#include "config.h"

#include <USBMSC.h>
#include <esp_partition.h>
#include <string.h>
#include <algorithm>

using std::min;
using std::max;

// ── State ─────────────────────────────────────────────────────────────────────
static const esp_partition_t *s_partition    = nullptr;
static volatile bool          s_active       = false;
static SemaphoreHandle_t      s_mutex        = nullptr;

// Written by USB task (onWrite), read by main task (mscDriveFileDone).
// Kept as simple atomics — no structs, no parsing, no loops in the USB task.
volatile uint32_t             g_mscDataBytes  = 0;   // bytes written to data area
static volatile unsigned long s_lastWriteMs   = 0;   // millis() of last write of any kind
static bool                   s_doneReported  = false;

static uint8_t __attribute__((aligned(4))) s_rmwBuf[FLASH_ERASE_SIZE];

// ── MSC callbacks ─────────────────────────────────────────────────────────────

static int32_t onRead(uint32_t lba, uint32_t offset, void *buf, uint32_t len)
{
  if (!s_partition || !s_mutex) return -1;
  if (xSemaphoreTake(s_mutex, pdMS_TO_TICKS(300)) != pdTRUE) return -1;
  esp_err_t err = esp_partition_read(s_partition,
                                     lba * MSC_SECTOR_SIZE + offset, buf, len);
  xSemaphoreGive(s_mutex);
  return (err == ESP_OK) ? (int32_t)len : -1;
}

static int32_t onWrite(uint32_t lba, uint32_t offset, uint8_t *buf, uint32_t len)
{
  if (!s_partition || !s_mutex) return -1;
  if (xSemaphoreTake(s_mutex, pdMS_TO_TICKS(500)) != pdTRUE) return -1;

  // ── Read-Modify-Write ──────────────────────────────────────────────────────
  uint32_t writeStart = lba * MSC_SECTOR_SIZE + offset;
  uint32_t writeEnd   = writeStart + len;
  uint32_t blockStart = (writeStart / FLASH_ERASE_SIZE) * FLASH_ERASE_SIZE;
  esp_err_t err = ESP_OK;

  while (blockStart < writeEnd && err == ESP_OK) {
    err = esp_partition_read(s_partition, blockStart, s_rmwBuf, FLASH_ERASE_SIZE);
    if (err != ESP_OK) break;
    uint32_t ps = max(writeStart, blockStart);
    uint32_t pe = min(writeEnd,   blockStart + FLASH_ERASE_SIZE);
    memcpy(s_rmwBuf + (ps - blockStart), buf + (ps - writeStart), pe - ps);
    err = esp_partition_erase_range(s_partition, blockStart, FLASH_ERASE_SIZE);
    if (err != ESP_OK) break;
    err = esp_partition_write(s_partition, blockStart, s_rmwBuf, FLASH_ERASE_SIZE);
    blockStart += FLASH_ERASE_SIZE;
  }

  // ── Minimal state update — no parsing, no branches on content ─────────────
  if (err == ESP_OK) {
    s_lastWriteMs = millis();
    if (lba >= MSC_DATA_START_LBA) {
      g_mscDataBytes += len;
    }
  }

  xSemaphoreGive(s_mutex);
  return (err == ESP_OK) ? (int32_t)len : -1;
}

static bool onStartStop(uint8_t /*power*/, bool start, bool load_eject)
{
  if (load_eject && start)  s_active = true;
  if (load_eject && !start) s_active = false;
  return true;
}

// ── Public API ────────────────────────────────────────────────────────────────

void mscDriveInit(USBMSC &msc)
{
  s_mutex = xSemaphoreCreateMutex();
  s_partition = esp_partition_find_first(
      ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_DATA_FAT, "ffat");
  if (!s_partition) return;

  msc.vendorID(MSC_VENDOR_ID);
  msc.productID(MSC_PRODUCT_ID);
  msc.productRevision(MSC_PRODUCT_REV);
  msc.onRead(onRead);
  msc.onWrite(onWrite);
  msc.onStartStop(onStartStop);
  msc.mediaPresent(true);
  msc.begin(s_partition->size / MSC_SECTOR_SIZE, MSC_SECTOR_SIZE);
  s_active = true;
}

bool mscDriveIsActive() { return s_active; }

bool mscDriveFileDone()
{
  // All detection logic lives here in the main task — never in onWrite.
  //
  // Two conditions:
  // 1. THRESHOLD: enough data written to rule out early pauses
  //    (systeminfo buffers for 10-15s before flushing — few bytes written
  //    during that time, keeping us safely below the threshold)
  // 2. SETTLE: FILE_DONE_SETTLE_MS of complete write silence
  //    (during script: FAT cluster writes happen constantly alongside data
  //     writes; after Out-File closes: all write activity stops)

  if (s_doneReported)                         return false;
  if (g_mscDataBytes < FILE_DONE_THRESHOLD)   return false;

  unsigned long last = s_lastWriteMs;
  if (last == 0)                              return false;
  if ((millis() - last) < FILE_DONE_SETTLE_MS) return false;

  s_doneReported = true;
  return true;
}

void mscDriveSessionReset()
{
  g_mscDataBytes = 0;
  s_lastWriteMs  = 0;
  s_doneReported = false;
}

void mscDrivePrintStatus()
{
  if (s_partition) {
    Serial0.printf("[MSC] Drive ready — %lu KB at 0x%06lX\n",
                   s_partition->size / 1024, s_partition->address);
  } else {
    Serial0.println(F("[MSC] ERROR: FAT partition not found!"));
  }
}