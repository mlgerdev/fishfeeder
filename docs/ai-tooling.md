# AI tooling — plugins, skills, connectors, MCP

Everything an AI assistant needs to work productively on this repo.

## MCP servers (already configured in `.mcp.json`)

| Server | Type | Purpose |
|--------|------|---------|
| `lvgl-docs` | remote HTTP (`https://lvgl.mcp.kapa.ai/`) | **LVGL AI** — official LVGL / LVGL Pro docs, forum and XML widget examples. Read-only. |
| `context7` | `npx @upstash/context7-mcp` | Up-to-date docs for **LVGL, ESP-IDF, PlatformIO, ESPHome** — resolves library IDs and returns version-correct API snippets. |
| `github` | `npx @modelcontextprotocol/server-github` | Issues / PRs / Actions / Releases. Export `GITHUB_MCP_PAT` in your shell before launching. |

### Enable per tool

* **Claude Code** — nothing to do: repo-level `.mcp.json` is picked up
  automatically; approve the servers on first use.
* **Cursor / Windsurf** — copy the entries from `.mcp.json` into the tool's
  MCP settings (`~/.cursor/mcp.json`).
* **VS Code (Copilot)** — add them under `github.copilot.chat.mcp.servers` in
  settings.json, or use the MCP extension.

## Suggested "skills" (project conventions the AI should follow)

* `include/shared/protocol.h` is the **contract** between the two firmware
  nodes — never change it on one side only.
* Design tokens live in `figma/design-spec.md` and are mirrored at the top of
  `src/ui_node/ui.cpp` (and generated into `include/figma_tokens.h`).
* LVGL code must target **LVGL 9.x APIs** (`lv_display_*`, `lv_screen_active`,
  `lv_obj_remove_flag`) — not the 8.x names.
* All user-facing strings are ASCII (Montserrat built-in font has no emoji).

## Figma connector

The **LVGL Flow for Figma** plugin + **LVGL Pro Editor** local service form
the connector pair that moves designs into code — full walkthrough in
[figma-lvgl.md](figma-lvgl.md).

## Useful bookmarks

* LVGL Pro docs: <https://lvgl.io/docs/pro>
* LVGL viewer: <https://viewer.lvgl.io/>
* LVGL Pro XML examples: <https://github.com/lvgl/lvgl_pro>
* ESP-IDF (ESP32-S3): <https://docs.espressif.com/projects/esp-idf/en/stable/esp32/index.html>
* Waveshare 4.3B wiki: <https://docs.waveshare.com/ESP32-S3-Touch-LCD-4.3B>
