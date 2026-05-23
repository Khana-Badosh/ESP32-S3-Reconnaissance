#pragma once

// ─────────────────────────────────────────────────────────────────────────────
// keyboard_defs.h — HID keyboard keycodes used by this project
//
// Only codes actively referenced in the codebase are listed here.
// For the full HID usage table see:
//   https://usb.org/sites/default/files/hut1_3_0.pdf  (section 10)
// ─────────────────────────────────────────────────────────────────────────────

// Modifier keys
#define KEY_LEFT_GUI    0x83   // Windows key

// Control keys
#define KEY_RETURN      0xB0
#define KEY_TAB         0xB3
#define KEY_ESC         0xB1
#define KEY_BACKSPACE   0xB2

// Function keys (available for future payloads)
#define KEY_F1          0xC2
#define KEY_F2          0xC3
#define KEY_F3          0xC4
#define KEY_F4          0xC5
#define KEY_F5          0xC6