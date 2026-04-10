from pathlib import Path
import re

ROOT = Path(r"d:\clawputer")
SCRIPT_DIR = ROOT / "desktop" / "CatputerGodot" / "scripts"

STATE_GROUPS = {
    "idle": ["idle1", "idle2", "idle1", "idle3"],
    "happy": ["happy1", "happy2"],
    "sleep": ["sleep1"],
    "talk": ["talk1", "talk2"],
}

SPRITE_SETS = [
    {
        "source": ROOT / "src" / "sprites.h",
        "out": SCRIPT_DIR / "handheld_sprites.gd",
        "class_name": "HandheldSprites",
        "prefix": "sprite_",
        "palette": {
            "_": (0.0, 0.0, 0.0, 0.0),
            "K": (48 / 255.0, 48 / 255.0, 47 / 255.0, 1.0),
            "B": (18 / 255.0, 22 / 255.0, 26 / 255.0, 1.0),
            "W": (1.0, 1.0, 1.0, 1.0),
            "D": (208 / 255.0, 71 / 255.0, 2 / 255.0, 1.0),
            "O": (210 / 255.0, 93 / 255.0, 31 / 255.0, 1.0),
            "L": (229 / 255.0, 130 / 255.0, 61 / 255.0, 1.0),
            "C": (253 / 255.0, 226 / 255.0, 197 / 255.0, 1.0),
        },
    },
    {
        "source": ROOT / "src" / "sprites_purple.h",
        "out": SCRIPT_DIR / "handheld_sprites_purple.gd",
        "class_name": "HandheldSpritesPurple",
        "prefix": "purple_sprite_",
        "palette": {
            "P0": (0.0, 0.0, 0.0, 0.0),
            "P1": (84 / 255.0, 56 / 255.0, 169 / 255.0, 1.0),
            "P2": (199 / 255.0, 141 / 255.0, 226 / 255.0, 1.0),
            "P3": (226 / 255.0, 198 / 255.0, 255 / 255.0, 1.0),
            "P4": (255 / 255.0, 226 / 255.0, 255 / 255.0, 1.0),
            "P5": (235 / 255.0, 235 / 255.0, 235 / 255.0, 1.0),
            "P6": (252 / 255.0, 252 / 255.0, 252 / 255.0, 1.0),
        },
    },
]


def parse_sprites(source: Path, prefix: str) -> dict[str, list[str]]:
    text = source.read_text(encoding="utf-8")
    pattern = re.compile(
        rf"const uint16_t PROGMEM ({re.escape(prefix)}[a-z0-9_]+)\[CHAR_W \* CHAR_H\] = \{{(.*?)\}};",
        re.S,
    )
    found: dict[str, list[str]] = {}
    for name, body in pattern.findall(text):
        tokens = [token.strip() for token in body.replace("\n", " ").split(",") if token.strip()]
        found[name] = tokens
    return found


def write_gd(source: Path, out_path: Path, class_name: str, prefix: str, palette: dict[str, tuple[float, float, float, float]]) -> None:
    sprite_defs = parse_sprites(source, prefix)
    lines: list[str] = []
    lines.append("extends RefCounted")
    lines.append(f"class_name {class_name}")
    lines.append("")
    lines.append("const WIDTH := 16")
    lines.append("const HEIGHT := 16")
    lines.append("const SCALE := 3")
    lines.append("")
    lines.append("const PALETTE := {")
    for key, rgba in palette.items():
        lines.append(
            f'\t"{key}": Color({rgba[0]:.6f}, {rgba[1]:.6f}, {rgba[2]:.6f}, {rgba[3]:.6f}),'
        )
    lines.append("}")
    lines.append("")
    lines.append("const RAW_SPRITES := {")
    for sprite_name, tokens in sprite_defs.items():
        joined = ", ".join(f'"{token}"' for token in tokens)
        lines.append(f'\t"{sprite_name}": [{joined}],')
    lines.append("}")
    lines.append("")
    lines.append("static var _cache: Dictionary = {}")
    lines.append("")
    lines.append("static func get_frames(state: String) -> Array[Texture2D]:")
    lines.append("\t_ensure_cache()")
    lines.append('\treturn _cache.get(state, [])')
    lines.append("")
    lines.append("static func _ensure_cache() -> void:")
    lines.append("\tif not _cache.is_empty():")
    lines.append("\t\treturn")
    for state, suffixes in STATE_GROUPS.items():
        names = [f"{prefix}{suffix}" for suffix in suffixes]
        names_joined = ", ".join(f'"{name}"' for name in names)
        lines.append(f'\t_cache["{state}"] = _build_group([{names_joined}])')
    lines.append("")
    lines.append("static func _build_group(names: Array[String]) -> Array[Texture2D]:")
    lines.append("\tvar frames: Array[Texture2D] = []")
    lines.append("\tfor name in names:")
    lines.append("\t\tframes.append(_build_texture(name))")
    lines.append("\treturn frames")
    lines.append("")
    lines.append("static func _build_texture(name: String) -> Texture2D:")
    lines.append("\tvar pixels: Array = RAW_SPRITES.get(name, [])")
    lines.append("\tvar image := Image.create(WIDTH, HEIGHT, false, Image.FORMAT_RGBA8)")
    lines.append("\tfor y in range(HEIGHT):")
    lines.append("\t\tfor x in range(WIDTH):")
    lines.append("\t\t\tvar token: String = pixels[y * WIDTH + x]")
    lines.append('\t\t\timage.set_pixel(x, y, PALETTE.get(token, Color(1, 0, 1, 1)))')
    lines.append("\timage.resize(WIDTH * SCALE, HEIGHT * SCALE, Image.INTERPOLATE_NEAREST)")
    lines.append("\treturn ImageTexture.create_from_image(image)")
    lines.append("")
    out_path.write_text("\n".join(lines) + "\n", encoding="utf-8")


def main() -> None:
    for sprite_set in SPRITE_SETS:
        write_gd(
            sprite_set["source"],
            sprite_set["out"],
            sprite_set["class_name"],
            sprite_set["prefix"],
            sprite_set["palette"],
        )


if __name__ == "__main__":
    main()
