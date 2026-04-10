import argparse
from pathlib import Path


def bucket_name(key: str) -> str:
    if not key:
        return "misc"
    first = key[0].lower()
    if "a" <= first <= "z":
        return first
    return "misc"


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--source", default=r"d:\clawputer\sdcard\pet\ime\pinyin.txt")
    parser.add_argument("--target-dir", default=r"d:\clawputer\sdcard\pet\ime")
    args = parser.parse_args()

    source_path = Path(args.source)
    target_dir = Path(args.target_dir)
    target_dir.mkdir(parents=True, exist_ok=True)

    buckets: dict[str, list[str]] = {}
    lines = source_path.read_text(encoding="utf-8").splitlines()
    header = ["# Catputer IME bucket"]

    for raw in lines:
        line = raw.strip()
        if not line or line.startswith("#"):
            continue
        if "=" not in line:
            continue
        key = line.split("=", 1)[0].strip()
        bucket = bucket_name(key)
        buckets.setdefault(bucket, []).append(line)

    for old_file in target_dir.glob("*.txt"):
        if old_file.name == "pinyin.txt":
            continue
        old_file.unlink()

    for bucket, entries in sorted(buckets.items()):
        output = target_dir / f"{bucket}.txt"
        output.write_text("\n".join(header + entries) + "\n", encoding="utf-8")
        print(f"Wrote {output.name} ({len(entries)} entries)")


if __name__ == "__main__":
    main()
