# 🐟 FishFeeder — automated catfish feeder

[![CI](https://github.com/mlgerdev/fishfeeder/actions/workflows/ci.yml/badge.svg)](https://github.com/mlgerdev/fishfeeder/actions/workflows/ci.yml)
[![Web flasher](https://github.com/mlgerdev/fishfeeder/actions/workflows/deploy-pages.yml/badge.svg)](https://github.com/mlgerdev/fishfeeder/actions/workflows/deploy-pages.yml)
[![License: MIT](https://img.shields.io/badge/License-MIT-00C2A8.svg)](LICENSE)

An automated catfish feeder built on:

* **UI node** — [Waveshare ESP32-S3-Touch-LCD-4.3B](https://docs.waveshare.com/ESP32-S3-Touch-LCD-4.3B)
  (ESP32-S3-WROOM-1-**N16R8**, 4.3" 800x480 ST7262 RGB panel, GT911 touch) running an
  **LVGL 9** touch UI: feed now, 4 daily schedule slots, feed history, link status.
* **Relay node** — ESP8266 NodeMCU V3 + 5V 1CH relay that pulses the feeder motor.
* **Link** — [ESP-NOW](https://docs.espressif.com/projects/esp-idf/en/stable/esp32/index.html)
  (wireless, no broker, no wires). Both nodes join the same WiFi so their radios share a
  channel; with WiFi down they fall back to channel 1 and feeding still works offline.

```
+---------------------------+      ESP-NOW (16B checksummed packets)     +------------------+
|  ESP32-S3 Touch-LCD-4.3B  |  -------------------------------------->  |  ESP8266 NodeMCU |
|  LVGL 9 · schedule · NTP  |            FEED / PING  ->                 |  5V 1CH relay    |
|  feed history · settings  |  <-               ACK                      |  feeder motor    |
+---------------------------+                                            +------------------+
```

## Repository map

```
platformio.ini            two firmware environments (ESP32-S3 + ESP8266)
boards/                   custom board def for the Waveshare 4.3B (16MB/OPI PSRAM)
include/lv_conf.h         LVGL 9.2 config (RGB565, millis tick, fonts)
include/shared/           protocol.h + config.h — the radio contract
src/ui_node/              ESP32-S3 firmware (LVGL UI, XL9555, GT911, schedule, comm)
src/relay_node/           ESP8266 firmware (relay, safety caps, ACKs)
.github/workflows/        ci.yml · deploy-pages.yml · figma-sync.yml
docs/                     web flasher (ESP Web Tools) + hardware & tooling guides
figma/                    design tokens + Figma export target (LVGL Flow XML)
tools/figma_tokens.py     Figma -> include/figma_tokens.h generator
.mcp.json                 MCP servers: LVGL AI · Context7 · GitHub
.devcontainer/            GitHub Codespaces (PlatformIO preinstalled)
```

## Quick start

### 0. Prereqs
[PlatformIO Core](https://platformio.org/install/cli) (`pip install platformio`)
or VS Code + the PlatformIO IDE extension, or just open the repo in **GitHub Codespaces**
(everything is preinstalled via `.devcontainer/`).

### 1. Secrets
```bash
cp src/ui_node/secrets.example.h    src/ui_node/secrets.h
cp src/relay_node/secrets.example.h src/relay_node/secrets.h
```
Fill in WiFi (2.4 GHz) + timezone. The relay MAC comes from step 2.

### 2. Flash the relay node first
```bash
pio run -e relay-node-esp8266 -t upload
pio device monitor -e relay-node-esp8266   # copy the printed RELAY MAC
```
Put the MAC into `src/ui_node/secrets.h` (`RELAY_MAC`).

### 3. Flash the UI node
```bash
pio run -e ui-node-esp32s3 -t upload
```

> No cable at hand? Use the **web flasher** (Chrome/Edge):
> **https://mlgerdev.github.io/fishfeeder/** — it installs both firmwares from the
> browser. Note: Pages on a *private* repo needs GitHub Pro; make the repo public
> or flip Settings → Pages → Source = **GitHub Actions**.

### 4. Daily use
The UI node syncs time via NTP (Asia/Manila by default — change `TZ_STRING`).
Tap **FEED NOW** or edit slots on the **Schedule** screen (switch = enable,
tap row = edit time/duration). **Status** shows IP, RSSI, relay link and the
last 12 feeds.

## Firmware builds & flashing

| Env | Board | Command |
|-----|-------|---------|
| `ui-node-esp32s3` | Waveshare ESP32-S3-Touch-LCD-4.3B | `pio run -e ui-node-esp32s3 -t upload` |
| `relay-node-esp8266` | NodeMCU V3 | `pio run -e relay-node-esp8266 -t upload` |

CI builds both on every push and attaches binaries as artifacts. The
**deploy-pages** workflow additionally merges the ESP32-S3 full-flash image
(`esptool merge_bin`) and publishes it with the ESP Web Tools manifests.

## Design flow: Figma -> LVGL

1. Install **"LVGL Flow for Figma"** from Figma Community (Figma **desktop** app).
2. Open the project in **LVGL Pro Editor v2.0.0+** → Figma button → **Start Service**.
3. Design on an 800x480 frame using the tokens in [`figma/design-spec.md`](figma/design-spec.md).
4. Export screens → LVGL XML lands in `figma/export/` → preview at
   <https://viewer.lvgl.io/>.
5. Optional: set `FIGMA_TOKEN` + `FIGMA_FILE_KEY` repo secrets and the
   **figma-sync** workflow regenerates `include/figma_tokens.h` daily.

Full walkthrough: [`docs/figma-lvgl.md`](docs/figma-lvgl.md).

## AI tooling (plugins · skills · connectors · MCP)

`.mcp.json` ships three MCP servers: **LVGL AI** (`https://lvgl.mcp.kapa.ai/`),
**Context7** (fresh LVGL/ESP-IDF/PlatformIO docs) and **GitHub**. Claude Code
picks it up automatically; Cursor/Windsurf/VS Code instructions in
[`docs/ai-tooling.md`](docs/ai-tooling.md).

## Hardware & wiring

Pin maps, relay wiring diagram, power budget and troubleshooting:
[`docs/hardware.md`](docs/hardware.md).

## Security notes

* **Never commit** `secrets.h` (git-ignored) — WiFi passwords live there.
* The GitHub PAT used to bootstrap this repo was shared in chat: **rotate/revoke
  it** at https://github.com/settings/personal-access-tokens once you're set up.
* The Figma token (`figd_...`) is equally sensitive — store it only as the
  `FIGMA_TOKEN` repo secret, never in code.

## Roadmap (v2 ideas)

- [ ] OTA updates from the UI node (app1 partition is already reserved)
- [ ] Improv WiFi provisioning from the web flasher (set `improv: true`)
- [ ] MQTT / Home Assistant bridge on the UI node
- [ ] SD-card feed logs (CS is already wired via XL9555 EXIO4)
- [ ] Vsync-aligned rendering + bounce buffers for tear-free animation
- [ ] Second feeder channel (the ESP8266 has plenty of free pins)

## License

[MIT](LICENSE) — © 2026 mlgerdev
