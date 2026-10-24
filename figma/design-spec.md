# FishFeeder design spec (Figma <-> ui.cpp tokens)

Display: **800 x 480**, landscape. Theme: dark aqua. Typeface: **Montserrat**
(LVGL built-ins, sizes 14 / 16 / 20 / 28 / 40).

## Color tokens

| Token | Hex | Used for |
|-------|-----|----------|
| `bg` | `#0B1220` | screen background |
| `card` | `#13223D` | cards, nav bar, rollers |
| `cardHi` | `#1B3054` | schedule rows, editor overlay |
| `accent` | `#00C2A8` | FEED NOW button, online states |
| `accentDim` | `#0A5F55` | nav buttons, toast |
| `warn` | `#FFB454` | relay offline, sending state |
| `danger` | `#FF6B6B` | feed failed, WiFi offline |
| `text` | `#E6F1FF` | primary text |
| `textDim` | `#7C93B3` | secondary text |

Shape: cards `radius 14`, overlay `radius 18`, toast `radius 12`, buttons
`radius 10` (default). Card padding `14`.

## Screens (match layer names to the LVGL objects in ui.cpp)

### 1. Home (`scr_home`)
```
header (h 60)   lbl_clock "07:42" (28) | lbl_wifi (14) | lbl_relay (14)
card "NEXT FEED" (360x250)      card "FEED NOW" (360x250)
  lbl NEXT FEED (14, textDim)     btn_feed 300x120, accent, "FEED NOW" (40)
  lbl_next_time (40)              lbl_feed (16, textDim) status line
  lbl_next_in (16, textDim)
card today (100%x84): lbl_today "Today: 2 feeds" (20)
nav (68): Home | Schedule | Status
```

### 2. Schedule (`scr_sched`)
```
title "Daily feed schedule" (28)
4 x row cards (100%x74, cardHi): slot_sw (switch) | slot_lbl "07:30 - 3s" (20) | "tap to edit" (14, textDim)
editor overlay 560x400 (hidden): title, sw_en, roller_h "00..23", roller_m "00..59",
  spin_dur 1..90 s, Save (accent) / Cancel
toast 300x64 (hidden): "Schedule saved"
```

### 3. Status (`scr_status`)
```
title "System status" (28)
card info (380x250): lbl_ip, lbl_rssi, lbl_uptime, lbl_fw
card relay (380x250): lbl_relay_mac, lbl_reen
"Recent feeds" (20) + hist_list (100%x140): 12 newest entries
```

## Export conventions (LVGL Flow)

* Frame names = screen names: `Home`, `Schedule`, `Status`.
* Layer names use the object names above — the Flow annotations map them to
  LVGL widgets (`lvgl:screen`, `lvgl:button`, `lvgl:label`, `lvgl:switch`,
  `lvgl:roller`, `lvgl:spinbox`).
* Colors must reference the named paint styles (or Variables) listed above so
  the CI token sync (`tools/figma_tokens.py`) can regenerate
  `include/figma_tokens.h`.
* Exports land in `figma/export/` as `project.xml` + `globals.xml` + one
  folder per screen; preview them at <https://viewer.lvgl.io/>.
