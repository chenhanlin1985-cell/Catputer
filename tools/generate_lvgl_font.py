from pathlib import Path
import shutil
import subprocess
import sys


def collect_chars(text: str, seen: set[str], ordered: list[str]) -> None:
    for ch in text:
        code = ord(ch)
        if ch in seen:
            continue
        if (
            code >= 0x80
            or ch in "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789"
            or ch in " .,:;!?+-/%()[]<>℃·|_"
        ):
            seen.add(ch)
            ordered.append(ch)


def collect_unicode_escapes(text: str, seen: set[str], ordered: list[str]) -> None:
    import re

    for match in re.finditer(r"\\u([0-9a-fA-F]{4})|\\U([0-9a-fA-F]{8})", text):
        raw = match.group(1) or match.group(2)
        ch = chr(int(raw, 16))
        collect_chars(ch, seen, ordered)


def unique_text(paths: list[Path]) -> str:
    seen: set[str] = set()
    ordered: list[str] = []

    for path in paths:
        if not path.exists():
            continue
        text = path.read_text(encoding="utf-8", errors="ignore")
        collect_chars(text, seen, ordered)
        collect_unicode_escapes(text, seen, ordered)

    return "".join(ordered)


def source_paths(root: Path) -> list[Path]:
    paths = [root / "tools" / "lvgl_font_chars.txt"]
    for folder in (root / "src", root / "sdcard" / "pet" / "dialogue"):
        if not folder.exists():
            continue
        for path in folder.rglob("*"):
            if path.name == "lv_font_cat_cn_18.c":
                continue
            if path.suffix.lower() in {".c", ".cpp", ".h", ".hpp", ".txt", ".json"}:
                paths.append(path)
    return paths


def find_font(root: Path) -> Path:
    candidates = [
        root
        / "_vendor"
        / "ESP32-S3-Touch-AMOLED-1.8"
        / "examples"
        / "Arduino-v3.3.5"
        / "libraries"
        / "lvgl"
        / "scripts"
        / "built_in_font"
        / "SimSun.woff",
        Path("C:/Windows/Fonts/simhei.ttf"),
        Path("C:/Windows/Fonts/simsun.ttc"),
    ]
    for path in candidates:
        if path.exists():
            return path
    raise FileNotFoundError("No usable Chinese font found for LVGL font generation")


def main() -> int:
    script_file = globals().get("__file__")
    root = Path(script_file).resolve().parents[1] if script_file else Path.cwd().resolve()
    output_path = root / "src" / "lv_font_cat_cn_18.c"
    font_path = find_font(root)
    npx = shutil.which("npx.cmd") or shutil.which("npx") or "npx.cmd"

    symbols = unique_text(source_paths(root))
    cmd = [
        npx,
        "--yes",
        "lv_font_conv",
        "--size",
        "18",
        "--bpp",
        "4",
        "--format",
        "lvgl",
        "--lv-font-name",
        "lv_font_cat_cn_18",
        "--font",
        str(font_path),
        "-r",
        "0x20-0x7F,0x2103,0x3000-0x303F,0xFF01-0xFF5E",
        "--symbols",
        symbols,
        "-o",
        str(output_path),
    ]
    result = subprocess.run(cmd, cwd=root, check=False)
    if result.returncode == 0 and output_path.exists():
        content = output_path.read_text(encoding="utf-8")
        content = content.replace(
            '#ifdef LV_LVGL_H_INCLUDE_SIMPLE\n#include "lvgl.h"\n#else\n#include "lvgl/lvgl.h"\n#endif',
            '#include "lvgl.h"',
        )
        output_path.write_text(content, encoding="utf-8")
    return result.returncode


if __name__ == "__main__":
    sys.exit(main())
else:
    code = main()
    if code != 0:
        raise SystemExit(code)
