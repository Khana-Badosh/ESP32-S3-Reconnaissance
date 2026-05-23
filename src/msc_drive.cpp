// ─────────────────────────────────────────────────────────────────────────────
// msc_drive.cpp — USB Mass Storage drive implementation
// ─────────────────────────────────────────────────────────────────────────────

#include <Arduino.h>          // must be first — sets up FreeRTOS, Serial0, F()
#include "msc_drive.h"
#include "config.h"

#include <USBMSC.h>
#include <esp_partition.h>
#include <string.h>
#include <algorithm>

using std::min;
using std::max;

// ── Module-private state ──────────────────────────────────────────────────────
static const esp_partition_t *s_partition  = nullptr;
static volatile bool          s_active     = false;
static SemaphoreHandle_t      s_mutex      = nullptr;

// Read-Modify-Write scratch buffer — aligned for esp_partition_write
static uint8_t __attribute__((aligned(4))) s_rmwBuf[FLASH_ERASE_SIZE];

// ── MSC callbacks ─────────────────────────────────────────────────────────────

static int32_t onRead(uint32_t lba, uint32_t offset, void *buf, uint32_t len)
{
  if (!s_partition || !s_mutex) return -1;
  if (xSemaphoreTake(s_mutex, pdMS_TO_TICKS(300)) != pdTRUE) return -1;

  esp_err_t err = esp_partition_read(s_partition,
                                     lba * MSC_SECTOR_SIZE + offset,
                                     buf, len);
  xSemaphoreGive(s_mutex);
  return (err == ESP_OK) ? (int32_t)len : -1;
}

static int32_t onWrite(uint32_t lba, uint32_t offset, uint8_t *buf, uint32_t len)
{
  // ── Read-Modify-Write ────────────────────────────────────────────────────────
  // SPI NOR flash erase granularity = FLASH_ERASE_SIZE (4096 bytes = 8 sectors).
  // A plain erase-then-write of one 512-byte sector destroys the other 7 in the
  // same erase block, corrupting the FAT and triggering the Windows format prompt.
  //
  // For every 4096-byte erase block overlapping the write:
  //   1. Read the full block into s_rmwBuf
  //   2. Patch only the bytes being written
  //   3. Erase the block
  //   4. Write the full modified block back
  // ────────────────────────────────────────────────────────────────────────────
  if (!s_partition || !s_mutex) return -1;
  if (xSemaphoreTake(s_mutex, pdMS_TO_TICKS(500)) != pdTRUE) return -1;

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

  xSemaphoreGive(s_mutex);
  return (err == ESP_OK) ? (int32_t)len : -1;
}

static bool onStartStop(uint8_t /*power*/, bool start, bool load_eject)
{
  if (load_eject && start) {
    s_active = true;
    Serial0.println(F("[MSC] PC mounted drive."));
  } else if (load_eject && !start) {
    s_active = false;
    Serial0.println(F("[MSC] PC ejected drive."));
  }
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

bool mscDriveIsActive()
{
  return s_active;
}

void mscDrivePrintStatus()
{
  if (s_partition) {
    Serial0.printf("[MSC] Drive ready — %lu KB at 0x%06lX\n",
                   s_partition->size / 1024,
                   s_partition->address);
  } else {
    Serial0.println(F("[MSC] ERROR: FAT partition not found!"));
    Serial0.println(F("      Re-flash fat16.bin via: python tools/flash_fs.py"));
  }
}