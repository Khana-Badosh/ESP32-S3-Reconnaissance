// ─────────────────────────────────────────────────────────────────────────────
// cli.cpp — serial command-line interface
// ─────────────────────────────────────────────────────────────────────────────

#include "cli.h"
#include "config.h"
#include "diagnostic.h"
#include "msc_drive.h"
#include "payloads.h"

// ── Command handlers ──────────────────────────────────────────────────────────

static void cmdHelp()
{
  Serial0.println(F("\n┌─ Commands ──────────────────────────────┐"));
  Serial0.println(F("│ start   Start diagnostic on target PC   │"));
  Serial0.println(F("│ status  Show system status              │"));
  Serial0.println(F("│ payloads List configured payloads       │"));
  Serial0.println(F("│ help    Show this message               │"));
  Serial0.println(F("├─ Hardware ──────────────────────────────┤"));
  Serial0.println(F("│ BOOT button  → start diagnostic         │"));
  Serial0.println(F("│ LED off      → idle / ready             │"));
  Serial0.println(F("│ LED blinking → diagnostic running       │"));
  Serial0.println(F("│ LED solid    → diagnostic complete      │"));
  Serial0.println(F("├─ Workflow ──────────────────────────────┤"));
  Serial0.println(F("│ 1. Plug into target PC                  │"));
  Serial0.println(F("│ 2. Press BOOT — Run dialog appears      │"));
  Serial0.println(F("│ 3. PowerShell runs collect.ps1          │"));
  Serial0.println(F("│ 4. Wait for LED to go solid             │"));
  Serial0.println(F("│ 5. Plug into your laptop                │"));
  Serial0.println(F("│ 6. Open ESP32 drive — read diag files   │"));
  Serial0.println(F("└─────────────────────────────────────────┘\n"));
}

static void cmdStatus()
{
  Serial0.println(F("\n--- Status ---"));
  Serial0.printf("MSC drive active : %s\n", mscDriveIsActive() ? "yes (PC has drive mounted)" : "no");
  Serial0.printf("Diagnostic       : %s\n", diagIsRunning()    ? "running" : "idle");
  Serial0.printf("Payloads loaded  : %d\n", payloadCount);
  Serial0.println(F("---\n"));
}

static void cmdPayloads()
{
  Serial0.println(F("\n--- Configured Payloads ---"));
  for (int i = 0; i < payloadCount; i++) {
    // Print just the first 80 chars to keep the output readable
    Serial0.printf("  %d: %.80s...\n", i + 1, payloads[i]);
  }
  Serial0.println(F("---\n"));
}

// ── Entry point ───────────────────────────────────────────────────────────────

void cliProcess(const String &cmd, USBHIDKeyboard &kb)
{
  if      (cmd == "start")    diagStart(kb);
  else if (cmd == "status")   cmdStatus();
  else if (cmd == "payloads") cmdPayloads();
  else if (cmd == "help")     cmdHelp();
  else {
    Serial0.printf("Unknown command: '%s'  (type 'help')\n", cmd.c_str());
  }
}