# Figma -> LVGL design flow

This project uses LVGL's official design-to-code pipeline. The design tokens
in `figma/design-spec.md` are already applied in `src/ui_node/ui.cpp`, so you
can either design in Figma and regenerate, or keep editing code directly.

## 1. Install the Figma plugin

1. Open the **Figma desktop app** (required — the plugin talks to the LVGL
   service over localhost).
2. Go to **Community** and search for **"LVGL Flow for Figma"**, click **Add**.
3. Open your design file and run it from **Plugins > LVGL Flow**.

Docs: <https://lvgl.io/docs/pro/figma/get-started>

## 2. Set up the LVGL side

1. Download **LVGL Pro Editor** (v2.0.0+) from lvgl.io and open this project.
2. Click the **Figma button** in the toolbar -> **Start Service**.
   The Figma plugin connects to this local service automatically.
3. In the plugin's **Project view**, set the display size to **800 x 480**.

## 3. Design / import flow

* Design each screen inside a top-level frame named after the screen
  (`Home`, `Schedule`, `Status`).
* Name layers to match the widget map in `figma/design-spec.md`
  (`card`, `btn_feed`, `lbl_next_time`, ...). Annotate layers as widgets in
  the plugin so they export as LVGL widgets, not plain rectangles.
* Colors: define the tokens from `figma/design-spec.md` as **paint styles**
  (or Variables) with exactly these names: `bg`, `card`, `cardHi`, `accent`,
  `accentDim`, `warn`, `danger`, `text`, `textDim`.
* **Export mode** -> select screens -> *Selected Screens*.
  The export lands in `figma/export/` as LVGL XML
  (`project.xml`, `globals.xml`, one folder per screen).

## 4. Preview without the editor

Drop any exported file onto <https://viewer.lvgl.io/> to render it in the
browser — handy for quick design reviews without flashing hardware.

## 5. Keep tokens in sync (CI)

* Repo secrets: `FIGMA_TOKEN` (a Figma personal access token — **treat it like
  a password, rotate if leaked**) and `FIGMA_FILE_KEY` (the key in the file
  URL `figma.com/design/<FILE_KEY>/...`).
* The **Figma design sync** workflow (daily + manual) pulls the named
  styles/variables and regenerates `include/figma_tokens.h`
  (see `tools/figma_tokens.py`).

## 6. LVGL AI

LVGL AI is an MCP server that grounds your AI assistant in the official LVGL
docs (no more guessed APIs):

* Endpoint: `https://lvgl.mcp.kapa.ai/` (read-only, already wired in
  `.mcp.json` as `lvgl-docs`).
* Works with Claude Code, Cursor, VS Code Copilot, Windsurf, Gemini CLI.
* Example prompts: *"Create a new LVGL 9 screen with a header, a big button
  and a roller"* or *"How does lv_display_set_buffers work with partial
  rendering?"*

Docs: <https://lvgl.io/docs/pro/ai>
