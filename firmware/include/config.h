#pragma once
#include <Arduino.h>

// ============================================================================
// HARDWARE PIN DEFINITIONS (ARC Reactor PCB Rev. A)
// ============================================================================
#define PIN_LED_DATA        3   // GPIO3 -> U3 (SN74AHCT1G125) -> D3..D11

#define NUM_LEDS_TOTAL      9
#define LED_CORE_IDX        0   // D3: Center LED
#define LED_RING_START_IDX  1   // D4..D11: 8 Outer Ring LEDs (Clockwise)
#define NUM_RING_LEDS       8

// ============================================================================
// WI-FI ACCESS POINT & NVS CONFIG
// ============================================================================
#define AP_SSID_PREFIX      "ARC_REACTOR_"    // Appends 8-char MAC hex UUID
#define AP_PASS             "iamironman"      // Min 8 chars
#define AP_CHANNEL          6
#define AP_MAX_CONN         4

#define NVS_NAMESPACE       "arc_cfg"
#define NVS_SAVE_DELAY_MS   1500              // Debounce delay before flash write

// ============================================================================
// SYSTEM TIMING & ANIMATION CONSTANTS
// ============================================================================
#define FX_FRAME_RATE_HZ    60
#define FX_FRAME_PERIOD_MS  (1000 / FX_FRAME_RATE_HZ)
#define TELEMETRY_RATE_MS   250
