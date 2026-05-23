#pragma once

#include <USBMSC.h>

// ─────────────────────────────────────────────────────────────────────────────
// msc_drive.h — USB Mass Storage drive module
//
// Manages the FAT16 partition exposed to the host PC as a removable drive.
// The filesystem image is pre-flashed; this module exposes it over USB MSC
// using direct esp_partition_read/write with Read-Modify-Write for writes.
//
// Usage:
//   mscDriveInit(msc)          — call once in setup(), before USB.begin()
//   mscDriveIsActive()         — true while PC has the drive mounted
//   mscDrivePrintStatus()      — prints partition info to Serial0
// ─────────────────────────────────────────────────────────────────────────────

void mscDriveInit(USBMSC &msc);
bool mscDriveIsActive();
void mscDrivePrintStatus();