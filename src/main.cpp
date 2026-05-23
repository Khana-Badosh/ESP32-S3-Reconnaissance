// ─────────────────────────────────────────────────────────────────────────────
// ESP32-S3 PC Diagnostic Tool
//
// Architecture:
//   • USB HID Keyboard — types commands into the target PC's Run dialog
//   • USB MSC Drive    — appears as "ESP32" removable drive on the target PC
//   • PowerShell       — collect.ps1 runs on the target PC, writes results
//                        directly to the MSC drive partition
//
// Flash layout (see partitions.csv):
//   0x10000  app0      firmware
//   0x600000 ffat      FAT16 filesystem (pre-flashed via tools/build_fs.py)
//
// To re-flash the filesystem image:
//   python tools/build_fs.py          → regenerates tools/fat16.bin
//   python tools/flash_fs.py          → flashes it to 0x600000
// ─────────────────────────────────────────────────────────────────────────────

#include <USB.h>
#include <USBHIDKeyboard.h>
#include <USBMSC.h>
#include <Preferences.h>
#include <esp_partition.h>
#include <esp_task_wdt.h>
#include <driver/gpio.h>

#include "config.h"
#include "keyboard_defs.h"
#include "msc_drive.h"
#include "diagnostic.h"
#include "cli.h"

// ─── Hardware objects ─────────────────────────────────────────────────────────
USBHIDKeyboard Keyboard;
USBMSC         msc;

// ─── setup() ─────────────────────────────────────────────────────────────────
void setup()
{
  // GPIO0 strapping fix — must be driven high briefly to avoid boot mode issues
  gpio_set_direction(GPIO_NUM_0, GPIO_MODE_OUTPUT);
  gpio_set_level(GPIO_NUM_0, 1);
  delay(50);
  gpio_set_direction(GPIO_NUM_0, GPIO_MODE_INPUT);

  // Extend watchdog — USB enumeration and diagnostic waits can be long
  esp_task_wdt_deinit();
  esp_task_wdt_init(30, false);

  // Initialise MSC drive (finds partition, registers callbacks)
  mscDriveInit(msc);

  // Keyboard must be initialised before USB.begin()
  Keyboard.begin();
  USB.begin();

  Serial0.begin(115200);
  delay(2000);                          // allow USB enumeration
  while (Serial0.available()) Serial0.read();

  esp_task_wdt_add(NULL);
  esp_task_wdt_reset();

  // Status LED and trigger button
  pinMode(PIN_TRIGGER, INPUT_PULLUP);
  pinMode(PIN_LED,     OUTPUT);
  digitalWrite(PIN_LED, LOW);

  // Startup banner
  Serial0.println(F("\n========================================"));
  Serial0.println(F(" ESP32-S3 PC Diagnostic Tool"));
  Serial0.println(F("========================================"));
  mscDrivePrintStatus();
  Serial0.println(F("\nREADY  — press BOOT or send 'start'"));
  Serial0.println(F("Type 'help' for available commands\n"));

  // Brief LED pulse to signal ready
  digitalWrite(PIN_LED, HIGH); delay(300); digitalWrite(PIN_LED, LOW);
}

// ─── loop() ──────────────────────────────────────────────────────────────────
void loop()
{
  esp_task_wdt_reset();

  unsigned long now = millis();

  // ── Button (debounced, non-blocking) ──
  static bool     buttonWasPressed = false;
  static unsigned long buttonTime  = 0;
  bool buttonDown = (digitalRead(PIN_TRIGGER) == LOW);

  if (buttonDown && !buttonWasPressed && !diagIsRunning()) {
    buttonTime      = now;
    buttonWasPressed = true;
  }
  if (buttonWasPressed && buttonDown && (now - buttonTime) > DEBOUNCE_MS && !diagIsRunning()) {
    diagStart(Keyboard);
  }
  if (!buttonDown) buttonWasPressed = false;

  // ── Serial CLI (non-blocking) ──
  static String serialBuf = "";
  while (Serial0.available()) {
    char c = Serial0.read();
    if (c == '\n') {
      serialBuf.trim();
      if (serialBuf.length() > 0) cliProcess(serialBuf, Keyboard);
      serialBuf = "";
    } else if (c != '\r') {
      serialBuf += c;
      if (serialBuf.length() > 256) serialBuf = "";
    }
  }

  // ── Diagnostic state machine ──
  if (diagIsRunning()) diagTick(Keyboard, now);
}