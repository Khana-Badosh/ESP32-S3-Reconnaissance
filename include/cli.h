#pragma once

#include <Arduino.h>
#include <USBHIDKeyboard.h>

// ─────────────────────────────────────────────────────────────────────────────
// cli.h — serial command-line interface
//
// Processes commands received over Serial0 (UART).
// To add a new command: add a branch in cliProcess() in cli.cpp.
// ─────────────────────────────────────────────────────────────────────────────

void cliProcess(const String &cmd, USBHIDKeyboard &kb);