from pathlib import Path
import re

try:
    from PIL import Image
except ImportError as exc:
    raise SystemExit("Pillow is required. Install with: pip install pillow") from exc


ROOT = Path(r"d:\clawputer")
ASSET_ROOT = ROOT / "assets" / "pets"

SPRITE_SETS = [
    {
        "source": ROOT / "src" / "sprites.h",
        "prefix": "sprite_",
        "out": ASSET_ROOT / "orange",
        "frames": {
            "idle": ["idle1", "idle2", "idle1", "idle3"],
            "happy": ["happy1", "happy2"],
            "sleep": ["sleep1"],
            "talk": ["talk1", "talk2"],
        },
        "palette": {
            "_": (0, 0, 0, 0),
            "K": (48, 48, 47, 255),
            "B": (18, 22, 26, 255),
            "W": (255, 255, 255, 255),
            "D": (208, 71, 2, 255),
            "O": (210, 93, 31, 255),
            "L": (229, 130, 61, 255),
            "C": (253, 226, 197, 255),
        },
    },
    {
        "source": ROOT / "src" / "sprites_purple.h",
        "prefix": "purple_sprite_",
        "out": ASSET_ROOT / "purple",
        "frames": {
            "idle": ["idle1", "idle2", "idle1", "idle3"],
            "happy": ["happy1", "happy2"],
            "sleep": ["sleep1"],
            "talk": ["talk1", "talk2"],
        },
        "palette": {
            "P0": (0, 0, 0, 0),
            "P1": (84, 56, 169, 255),
            "P2": (199, 141, 226, 255),
            "P3": (226, 198, 255, 255),
            "P4": (255, 226, 255, 255),
            "P5": (235, 235, 235, 255),
            "P6": (252, 252, 252, 255),
        },
    },
]

CHAR_W = 16
CHAR_H = 16


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


def write_png(tokens: list[str], palette: dict[str, tuple[int, int, int, int]], out_path: Path) -> None:
    img = Image.new("RGBA", (CHAR_W, CHAR_H), (0, 0, 0, 0))
    pixels = img.load()
    for y in range(CHAR_H):
        for x in range(CHAR_W):
            token = tokens[y * CHAR_W + x]
            pixels[x, y] = palette.get(token, (255, 0, 255, 255))
    out_path.parent.mkdir(parents=True, exist_ok=True)
    img.save(out_path)


def main() -> None:
    for sprite_set in SPRITE_SETS:
        sprite_defs = parse_sprites(sprite_set["source"], sprite_set["prefix"])
        for group, suffixes in sprite_set["frames"].items():
            for index, suffix in enumerate(suffixes, start=1):
                sprite_name = f'{sprite_set["prefix"]}{suffix}'
                tokens = sprite_defs.get(sprite_name)
                if not tokens:
                    continue
                out_name = f"{group}_{index}.png"
                out_path = sprite_set["out"] / group / out_name
                write_png(tokens, sprite_set["palette"], out_path)
                print(f"Wrote {out_path}")


if __name__ == "__main__":
    main()
