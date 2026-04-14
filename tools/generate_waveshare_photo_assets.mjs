import fs from "node:fs";
import path from "node:path";
import zlib from "node:zlib";

const root = process.cwd();
const inputDir = path.join(root, "sdcard", "pet", "photos");
const outHeader = path.join(root, "src", "waveshare_photo_assets.h");
const outSource = path.join(root, "src", "waveshare_photo_assets.cpp");

const photos = [
  ["roof_sun", "roof_sun.png"],
  ["window_rain", "window_rain.png"],
  ["corner_store", "corner_store.png"],
  ["seaside_trip", "seaside_trip.png"],
  ["night_walk", "night_walk.png"],
  ["quiet_alley", "quiet_alley.png"],
];

function decodePngRgb(filePath) {
  const data = fs.readFileSync(filePath);
  if (data.toString("ascii", 1, 4) !== "PNG") {
    throw new Error(`${filePath} is not a PNG`);
  }

  let offset = 8;
  let width = 0;
  let height = 0;
  let bitDepth = 0;
  let colorType = 0;
  const idat = [];

  while (offset < data.length) {
    const length = data.readUInt32BE(offset);
    const type = data.toString("ascii", offset + 4, offset + 8);
    const chunk = data.subarray(offset + 8, offset + 8 + length);
    if (type === "IHDR") {
      width = chunk.readUInt32BE(0);
      height = chunk.readUInt32BE(4);
      bitDepth = chunk[8];
      colorType = chunk[9];
      const interlace = chunk[12];
      if (bitDepth !== 8 || colorType !== 2 || interlace !== 0) {
        throw new Error(`${filePath} must be 8-bit RGB non-interlaced PNG`);
      }
    } else if (type === "IDAT") {
      idat.push(chunk);
    } else if (type === "IEND") {
      break;
    }
    offset += 12 + length;
  }

  const raw = zlib.inflateSync(Buffer.concat(idat));
  const bpp = 3;
  const stride = width * bpp;
  const pixels = Buffer.alloc(width * height * bpp);
  let src = 0;
  let prev = Buffer.alloc(stride);
  let row = Buffer.alloc(stride);

  for (let y = 0; y < height; y++) {
    const filter = raw[src++];
    const scan = raw.subarray(src, src + stride);
    src += stride;
    for (let x = 0; x < stride; x++) {
      const a = x >= bpp ? row[x - bpp] : 0;
      const b = prev[x];
      const c = x >= bpp ? prev[x - bpp] : 0;
      let predict = 0;
      if (filter === 1) predict = a;
      else if (filter === 2) predict = b;
      else if (filter === 3) predict = Math.floor((a + b) / 2);
      else if (filter === 4) {
        const p = a + b - c;
        const pa = Math.abs(p - a);
        const pb = Math.abs(p - b);
        const pc = Math.abs(p - c);
        predict = pa <= pb && pa <= pc ? a : pb <= pc ? b : c;
      } else if (filter !== 0) {
        throw new Error(`${filePath} uses unsupported PNG filter ${filter}`);
      }
      row[x] = (scan[x] + predict) & 0xff;
    }
    row.copy(pixels, y * stride);
    prev = Buffer.from(row);
    row.fill(0);
  }

  return { width, height, pixels };
}

function rgb565(r, g, b) {
  return ((r & 0xf8) << 8) | ((g & 0xfc) << 3) | (b >> 3);
}

const decoded = photos.map(([key, filename]) => {
  const image = decodePngRgb(path.join(inputDir, filename));
  if (image.width !== 64 || image.height !== 48) {
    throw new Error(`${filename} must be 64x48`);
  }
  const values = [];
  for (let i = 0; i < image.pixels.length; i += 3) {
    values.push(rgb565(image.pixels[i], image.pixels[i + 1], image.pixels[i + 2]));
  }
  return { key, values };
});

let header = `#pragma once

#include "lvgl.h"

static constexpr int WAVESHARE_PHOTO_W = 64;
static constexpr int WAVESHARE_PHOTO_H = 48;

`;
for (const { key } of decoded) {
  header += `extern const lv_img_dsc_t waveshare_photo_${key};\n`;
}

let source = `#include "waveshare_photo_assets.h"\n\n`;
for (const { key, values } of decoded) {
  const bytes = values.flatMap((v) => [v & 0xff, (v >> 8) & 0xff]);
  const mapName = `waveshare_photo_${key}_map`;
  source += `static const uint8_t ${mapName}[WAVESHARE_PHOTO_W * WAVESHARE_PHOTO_H * 2] = {\n`;
  for (let i = 0; i < bytes.length; i += 16) {
    source += `    ${bytes.slice(i, i + 16).map((v) => `0x${v.toString(16).padStart(2, "0")}`).join(", ")},\n`;
  }
  source += `};\n\n`;
  source += `const lv_img_dsc_t waveshare_photo_${key} = {\n`;
  source += `    {LV_IMG_CF_TRUE_COLOR, 0, 0, WAVESHARE_PHOTO_W, WAVESHARE_PHOTO_H},\n`;
  source += `    sizeof(${mapName}),\n`;
  source += `    ${mapName},\n`;
  source += `};\n\n`;
}

fs.writeFileSync(outHeader, header);
fs.writeFileSync(outSource, source);
console.log(`Generated ${decoded.length} photo assets`);
