// ─────────────────────────────────────────────────────────────────────────────
// ESP32-S3 PC Diagnostic Tool — main.cpp
// ─────────────────────────────────────────────────────────────────────────────

#include <USB.h>
#include <USBHIDKeyboard.h>
#include <USBMSC.h>
#include <Preferences.h>
#include <esp_partition.h>
#include <esp_task_wdt.h>
#include <driver/gpio.h>
#include <Adafruit_NeoPixel.h>

#include "config.h"
#include "keyboard_defs.h"
#include "msc_drive.h"
#include "diagnostic.h"
#include "cli.h"

USBHIDKeyboard    Keyboard;
USBMSC            msc;
Adafruit_NeoPixel pixel(NEOPIXEL_COUNT, PIN_NEOPIXEL, NEO_GRB + NEO_KHZ800);

// ── NeoPixel helpers ──────────────────────────────────────────────────────────
static void pixelSet(uint8_t r, uint8_t g, uint8_t b)
{
  pixel.setPixelColor(0, pixel.Color(r, g, b));
  pixel.show();
}
static void pixelOff() { pixelSet(0, 0, 0); }

// Blocking flash — safe to use in setup() and for the final done signal
// because we deliberately want to pause there.
static void pixelFlashBlocking(uint8_t r, uint8_t g, uint8_t b,
                                int times, int ms = LED_FLASH_MS)
{
  for (int i = 0; i < times; i++) {
    pixelSet(r, g, b); delay(ms);
    pixelOff();        delay(ms);
  }
}

// One green signal per diagnostic run — reset by ledWriteSessionReset()
static bool s_doneSignalSent = false;

// Called by main loop to reset state at the start of each diagnostic run
void ledWriteSessionReset()
{
  s_doneSignalSent = false;
  mscDriveSessionReset();
  pixelOff();
}

// ── setup() ──────────────────────────────────────────────────────────────────
void setup()
{
  gpio_set_direction(GPIO_NUM_0, GPIO_MODE_OUTPUT);
  gpio_set_level(GPIO_NUM_0, 1);
  delay(50);
  gpio_set_direction(GPIO_NUM_0, GPIO_MODE_INPUT);

  esp_task_wdt_deinit();
  esp_task_wdt_init(30, false);

  pixel.begin();
  pixelOff();

  // Init MSC drive before USB so callbacks are registered before enumeration
  mscDriveInit(msc);
  Keyboard.begin();

  // USB.begin() triggers host enumeration — may cause a brief reset on some
  // hosts. Red boot flash happens AFTER this settles so it only fires once.
  USB.begin();
  delay(2000);   // wait for USB enumeration to settle before flashing

  Serial0.begin(115200);
  while (Serial0.available()) Serial0.read();

  esp_task_wdt_add(NULL);
  esp_task_wdt_reset();

  pinMode(PIN_TRIGGER, INPUT_PULLUP);

  Serial0.println(F("\n========================================"));
  Serial0.println(F(" ESP32-S3 PC Diagnostic Tool"));
  Serial0.println(F("========================================"));
  mscDrivePrintStatus();
  Serial0.println(F("READY  — press BOOT or send 'start'\n"));

  // No startup LED flash — red fires on button press, green when done
}

// ── loop() ───────────────────────────────────────────────────────────────────
void loop()
{
  esp_task_wdt_reset();
  unsigned long now = millis();

  // ── Button ────────────────────────────────────────────────────────────────
  static bool          buttonWasPressed = false;
  static unsigned long buttonTime       = 0;
  bool buttonDown = (digitalRead(PIN_TRIGGER) == LOW);

  if (buttonDown && !buttonWasPressed) {
    buttonTime       = now;
    buttonWasPressed = true;
  }
  if (buttonWasPressed && buttonDown
      && (now - buttonTime) > DEBOUNCE_MS
      && !diagIsRunning()) {
    ledWriteSessionReset();
    buttonWasPressed = false;
    // Red flash immediately on button press — confirms trigger registered
    pixelFlashBlocking(LED_COLOR_BOOT, 3, 200);
    pixelSet(LED_COLOR_DATA);   // solid blue immediately after red — working
    diagStart(Keyboard);
  }
  if (!buttonDown) buttonWasPressed = false;

  // ── Serial CLI ────────────────────────────────────────────────────────────
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

  // ── LED file-close detection ─────────────────────────────────────────────
  // Blue is set solid the moment the button is pressed (in the button handler).
  // Green fires the instant PowerShell closes the output file — detected by a
  // root-directory-entry write (LBA 164-195) occurring after data writes.
  // This is machine-speed independent — no timing or settle logic needed.
  if (!s_doneSignalSent && mscDriveFileDone()) {
    s_doneSignalSent = true;
    pixelOff();
    pixelFlashBlocking(LED_COLOR_DONE, 3, 400);
    Serial0.println(F("[LED] File closed — 3 green flashes."));
  }

  // ── Diagnostic state machine ──────────────────────────────────────────────
  if (diagIsRunning()) diagTick(Keyboard, now);
}