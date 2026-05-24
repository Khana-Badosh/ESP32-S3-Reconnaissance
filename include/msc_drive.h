#pragma once
#include <USBMSC.h>

void mscDriveInit(USBMSC &msc);
bool mscDriveIsActive();
void mscDrivePrintStatus();

// Returns true once when the result file is complete:
//   >= FILE_DONE_THRESHOLD bytes written AND FILE_DONE_SETTLE_MS of silence.
// Safe to call every loop() tick — all logic runs in the main task.
bool mscDriveFileDone();

// Reset detection state — call at the start of each diagnostic run.
void mscDriveSessionReset();

// Total data bytes written this session (lba >= MSC_DATA_START_LBA).
extern volatile uint32_t g_mscDataBytes;