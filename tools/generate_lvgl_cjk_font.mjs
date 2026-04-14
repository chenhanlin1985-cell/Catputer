import { spawnSync } from "node:child_process";
import fs from "node:fs";
import path from "node:path";
import { fileURLToPath } from "node:url";

const root = path.resolve(path.dirname(fileURLToPath(import.meta.url)), "..");
const fontPath = path.join(
  root,
  "_vendor",
  "ESP32-S3-Touch-AMOLED-1.8",
  "examples",
  "Arduino-v3.3.5",
  "libraries",
  "lvgl",
  "scripts",
  "built_in_font",
  "SimSun.woff",
);
const chinese4List = path.join(
  root,
  "lib",
  "waveshare",
  "GFX_Library_for_Arduino",
  "src",
  "font",
  "chinese4.list",
);
const outputPath = path.join(root, "src", "lv_font_cat_cn_16.c");
const converter = path.join(
  process.env.LOCALAPPDATA ?? "",
  "npm-cache",
  "_npx",
  "b62fd1a864044392",
  "node_modules",
  ".bin",
  process.platform === "win32" ? "lv_font_conv.cmd" : "lv_font_conv",
);

function readChinese4Symbols() {
  const codes = new Set();
  const text = fs.readFileSync(chinese4List, "utf8");
  for (const raw of text.split(",")) {
    const item = raw.trim();
    if (!item) continue;
    const hex = item.match(/^\$([0-9a-fA-F]+)$/);
    const range = item.match(/^([0-9]+)-([0-9]+)$/);
    const plainHex = item.match(/^([0-9a-fA-F]+)$/);
    if (hex) {
      codes.add(Number.parseInt(hex[1], 16));
    } else if (range) {
      const start = Number.parseInt(range[1], 10);
      const end = Number.parseInt(range[2], 10);
      for (let code = start; code <= end; code += 1) codes.add(code);
    } else if (plainHex) {
      codes.add(Number.parseInt(plainHex[1], 16));
    }
  }

  const appSymbols =
    "℃°—–…·≤≥×√✓、。！？：；“”‘’（）《》【】「」『』￥";
  for (const ch of appSymbols) codes.add(ch.codePointAt(0));

  return [...codes]
    .filter((code) => code >= 0x80 && code <= 0xffff)
    .sort((a, b) => a - b)
    .map((code) => String.fromCodePoint(code))
    .join("");
}

const result = spawnSync(
  fs.existsSync(converter) ? converter : "npx.cmd",
  [
    ...(fs.existsSync(converter) ? [] : ["--yes", "lv_font_conv"]),
    "--size",
    "16",
    "--bpp",
    "1",
    "--format",
    "lvgl",
    "--lv-include",
    "lvgl.h",
    "--lv-font-name",
    "lv_font_cat_cn_16",
    "--font",
    fontPath,
    "-r",
    "0x20-0x7F",
    "--symbols",
    readChinese4Symbols(),
    "--no-kerning",
    "-o",
    outputPath,
  ],
  { cwd: root, stdio: "inherit", shell: process.platform === "win32" },
);

process.exit(result.status ?? 1);
