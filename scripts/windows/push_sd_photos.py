import argparse
import json
import sys
import time
from pathlib import Path

import serial


def read_json_line(port: serial.Serial):
    while True:
        raw = port.readline()
        if not raw:
            raise TimeoutError("Timed out waiting for device response")
        line = raw.decode("utf-8", errors="ignore").strip()
        if not line:
            continue
        if line.startswith("{"):
            obj = json.loads(line)
            if not obj.get("ok"):
                raise RuntimeError(f"Device error: {obj.get('error', 'unknown')}")
            return obj


def send_cmd(port: serial.Serial, payload: dict):
    line = json.dumps(payload, ensure_ascii=False, separators=(",", ":")) + "\n"
    port.write(line.encode("utf-8"))
    port.flush()
    return read_json_line(port)


def push_file(port: serial.Serial, local_path: Path, remote_path: str):
    data = local_path.read_bytes()
    send_cmd(port, {"cmd": "sd_put_begin", "path": remote_path, "size": len(data)})
    for offset in range(0, len(data), 96):
        chunk = data[offset:offset + 96]
        send_cmd(port, {"cmd": "sd_put_chunk", "data": chunk.hex().upper()})
    send_cmd(port, {"cmd": "sd_put_end"})


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--port", default="COM5")
    parser.add_argument("--source", default=r"d:\clawputer\sdcard\pet\photos")
    args = parser.parse_args()

    source_dir = Path(args.source)
    if not source_dir.exists():
        raise FileNotFoundError(f"Source directory not found: {source_dir}")

    with serial.Serial(args.port, 115200, timeout=10, write_timeout=10) as port:
        time.sleep(1.2)
        port.reset_input_buffer()
        port.reset_output_buffer()

        send_cmd(port, {"cmd": "ping"})
        send_cmd(port, {"cmd": "sd_mkdir", "path": "/pet"})
        send_cmd(port, {"cmd": "sd_mkdir", "path": "/pet/photos"})

        for file_path in sorted(source_dir.glob("*.png")):
            push_file(port, file_path, f"/pet/photos/{file_path.name}")
            print(f"Pushed {file_path.name}")

    print("SD photo sync complete.")


if __name__ == "__main__":
    try:
        main()
    except Exception as exc:
        print(str(exc), file=sys.stderr)
        sys.exit(1)
