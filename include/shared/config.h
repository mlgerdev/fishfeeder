/**
 * Shared FishFeeder configuration constants — used by BOTH firmware nodes.
 * Keep protocol.h and this file identical on both sides.
 */
#pragma once

// --- ESP-NOW radio ---
#define FF_ESPNOW_CHANNEL 1 // fallback channel when WiFi is unavailable

// --- Feeding safety limits (enforced on the relay node) ---
#define FF_FEED_DURATION_MIN_MS 200UL    // shortest allowed pulse
#define FF_FEED_DURATION_MAX_MS 60000UL  // longest allowed pulse (hard cap 60 s)
#define FF_FEED_DURATION_DEFAULT_MS 3000UL // "Feed now" pulse

// --- Link health ---
#define FF_HEARTBEAT_PERIOD_MS 30000UL // UI node pings the relay node
#define FF_NODE_TIMEOUT_MS 90000UL     // relay considered offline after this
