#!/usr/bin/env python3
"""FishFeeder — Figma -> LVGL design token sync.

Reads colors from the Figma file (secrets.FIGMA_FILE_KEY) and writes
include/figma_tokens.h with C macros the LVGL UI can use.

Token lookup order:
  1. Figma *variables* (if the file uses the Variables API)
  2. Paint styles (named color styles)

Expected style/variable names (case-insensitive):
  bg / card / cardHi / accent / accentDim / warn / danger / text / textDim

Env:
  FIGMA_TOKEN     — Figma personal access token (required)
  FIGMA_FILE_KEY  — key from the file URL (required)
  FIGMA_TOKEN_OUT — output header path (default include/figma_tokens.h)
"""
import json
import os
import sys
import urllib.request

API = "https://api.figma.com/v1"

NAME_MAP = {
    "bg": "FF_COLOR_BG",
    "card": "FF_COLOR_CARD",
    "cardhi": "FF_COLOR_CARD_HI",
    "accent": "FF_COLOR_ACCENT",
    "accentdim": "FF_COLOR_ACCENT_DIM",
    "warn": "FF_COLOR_WARN",
    "danger": "FF_COLOR_DANGER",
    "text": "FF_COLOR_TEXT",
    "textdim": "FF_COLOR_TEXT_DIM",
}

DEFAULTS = {
    "FF_COLOR_BG": "0x0B1220",
    "FF_COLOR_CARD": "0x13223D",
    "FF_COLOR_CARD_HI": "0x1B3054",
    "FF_COLOR_ACCENT": "0x00C2A8",
    "FF_COLOR_ACCENT_DIM": "0x0A5F55",
    "FF_COLOR_WARN": "0xFFB454",
    "FF_COLOR_DANGER": "0xFF6B6B",
    "FF_COLOR_TEXT": "0xE6F1FF",
    "FF_COLOR_TEXT_DIM": "0x7C93B3",
}


def api(path, token):
    req = urllib.request.Request(
        f"{API}{path}", headers={"X-Figma-Token": token})
    with urllib.request.urlopen(req, timeout=30) as r:
        return json.load(r)


def paint_to_hex(paint):
    if paint.get("type") not in ("SOLID",) or not paint.get("visible", True):
        return None
    c = paint.get("color")
    if not c:
        return None
    r = int(round(c.get("r", 0) * 255))
    g = int(round(c.get("g", 0) * 255))
    b = int(round(c.get("b", 0) * 255))
    return f"0x{r:02X}{g:02X}{b:02X}"


def main():
    token = os.environ.get("FIGMA_TOKEN", "").strip()
    file_key = os.environ.get("FIGMA_FILE_KEY", "").strip()
    out_path = os.environ.get("FIGMA_TOKEN_OUT", "include/figma_tokens.h")

    tokens = dict(DEFAULTS)  # start from defaults so the header always builds

    if not token or not file_key:
        print("FIGMA_TOKEN / FIGMA_FILE_KEY not set — writing defaults only")
    else:
        try:
            file = api(f"/files/{file_key}?depth=1", token)
            styles = file.get("styles", {})
            # depth=1 does not include style paints; fetch style nodes lazily
            for sid, meta in styles.items():
                key = NAME_MAP.get(
                    meta.get("name", "").replace(" ", "").replace("_", "").lower())
                if not key:
                    continue
                node = api(f"/files/{file_key}/nodes?ids={sid}", token)
                nodes = node.get("nodes", {}).get(sid, {}).get("document", {})
                for child in nodes.get("children", []):
                    for paint in child.get("paints", []) or []:
                        hx = paint_to_hex(paint)
                        if hx:
                            tokens[key] = hx
            print(f"Fetched {len(styles)} styles from Figma")
        except Exception as e:  # noqa: BLE001 — never break CI on Figma hiccups
            print(f"WARN: Figma fetch failed ({e}) — using defaults")

    lines = [
        "/**",
        " * FishFeeder — design tokens generated from Figma",
        " * (tools/figma_tokens.py — do not edit by hand)",
        " */",
        "#pragma once",
        "",
    ]
    for k in sorted(DEFAULTS):
        lines.append(f"#define {k} lv_color_hex({tokens[k]})")
    lines.append("")
    with open(out_path, "w") as f:
        f.write("\n".join(lines))
    print(f"Wrote {out_path}")


if __name__ == "__main__":
    sys.exit(main())
