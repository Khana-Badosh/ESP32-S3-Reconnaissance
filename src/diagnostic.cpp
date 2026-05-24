// ─────────────────────────────────────────────────────────────────────────────
// diagnostic.cpp — fully non-blocking diagnostic state machine
// ─────────────────────────────────────────────────────────────────────────────

#include <Arduino.h>
#include "diagnostic.h"
#include "config.h"
#include "keyboard_defs.h"
#include "payloads.h"

enum DiagState {
  STATE_IDLE,
  STATE_STARTUP_WAIT,      // wait after red flash before Win+R (avoids key collision)
  STATE_WINR_PRESS,
  STATE_WINR_HOLD,
  STATE_WINR_RELEASE,
  STATE_WINR_WAIT,
  STATE_CMD_TYPE,
  STATE_CMD_ENTER_PRESS,
  STATE_CMD_ENTER_HOLD,
  STATE_CMD_ENTER_RELEASE,
  STATE_CMD_WAIT,
  STATE_WAIT_COMPLETION,
  STATE_DONE,
};

static DiagState     s_state   = STATE_IDLE;
static int           s_cmdIdx  = 0;
static unsigned long s_stateTs = 0;
static bool          s_running = false;
static bool          s_complete = false;  // set true only when fully done

static void transition(DiagState next, unsigned long now)
{
  s_state   = next;
  s_stateTs = now;
}

static bool elapsed(unsigned long now, unsigned long ms)
{
  return (now - s_stateTs) >= ms;
}

void diagStart(USBHIDKeyboard &kb)
{
  if (s_running) return;
  s_cmdIdx   = 0;
  s_running  = true;
  s_complete = false;
  transition(STATE_STARTUP_WAIT, millis());
  Serial0.println(F("[DIAG] Starting..."));
}

bool diagIsRunning()  { return s_running; }
bool diagIsComplete() { return s_complete; }

void diagTick(USBHIDKeyboard &kb, unsigned long now)
{
  switch (s_state) {

  case STATE_STARTUP_WAIT:
    // Wait long enough for the red boot flash (3 x 200ms on + 200ms off = 1200ms)
    // plus a small margin before we send any keystrokes.
    if (elapsed(now, 1400)) {
      Serial0.println(F("[DIAG] Sending Win+R..."));
      transition(STATE_WINR_PRESS, now);
    }
    break;

  case STATE_WINR_PRESS:
    // 500ms settling time before first keypress — ensures the USB HID
    // keyboard descriptor has been fully accepted by the host and the
    // desktop has focus. Without this, Win+R sometimes fires into nothing.
    if (elapsed(now, 500)) {
      kb.press(KEY_LEFT_GUI);
      kb.press('r');
      transition(STATE_WINR_HOLD, now);
    }
    break;

  case STATE_WINR_HOLD:
    if (elapsed(now, 80)) {
      kb.releaseAll();
      transition(STATE_WINR_RELEASE, now);
    }
    break;

  case STATE_WINR_RELEASE:
    if (elapsed(now, 50)) transition(STATE_WINR_WAIT, now);
    break;

  case STATE_WINR_WAIT:
    if (elapsed(now, RUN_DIALOG_DELAY_MS)) {
      Serial0.println(F("[DIAG] Typing command..."));
      transition(STATE_CMD_TYPE, now);
    }
    break;

  case STATE_CMD_TYPE:
    if (s_cmdIdx < payloadCount) {
      Serial0.printf("[DIAG] Payload %d/%d\n", s_cmdIdx + 1, payloadCount);
      kb.print(payloads[s_cmdIdx]);
      transition(STATE_CMD_ENTER_PRESS, now);
    }
    break;

  case STATE_CMD_ENTER_PRESS:
    if (elapsed(now, 100)) {
      kb.press(KEY_RETURN);
      transition(STATE_CMD_ENTER_HOLD, now);
    }
    break;

  case STATE_CMD_ENTER_HOLD:
    if (elapsed(now, 80)) {
      kb.releaseAll();
      s_cmdIdx++;
      transition(STATE_CMD_ENTER_RELEASE, now);
    }
    break;

  case STATE_CMD_ENTER_RELEASE:
    if (elapsed(now, 50)) transition(STATE_CMD_WAIT, now);
    break;

  case STATE_CMD_WAIT:
    if (elapsed(now, CMD_POWERSHELL_MS)) {
      if (s_cmdIdx < payloadCount) {
        transition(STATE_WINR_PRESS, now);
      } else {
        Serial0.printf("[DIAG] Waiting %d s for script to finish...\n",
                       COMPLETION_WAIT_MS / 1000);
        transition(STATE_WAIT_COMPLETION, now);
      }
    }
    break;

  case STATE_WAIT_COMPLETION:
    if (elapsed(now, COMPLETION_WAIT_MS)) {
      s_complete = true;   // only set here — after full wait
      Serial0.println(F("[DIAG] Done."));
      transition(STATE_DONE, now);
    }
    break;

  case STATE_DONE:
    if (elapsed(now, 4000)) {
      s_running  = false;
      s_complete = false;
      s_state    = STATE_IDLE;
      Serial0.println(F("[DIAG] Ready."));
    }
    break;

  default: break;
  }
}