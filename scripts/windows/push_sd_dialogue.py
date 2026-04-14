import argparse
import json
import pathlib
import time

import serial


def read_json_line(port: serial.Serial):
    deadline = time.time() + 15
    while time.time() < deadline:
        line = port.readline()
        if not line:
            continue
        try:
            return json.loads(line.decode("utf-8", errors="replace").strip())
        except json.JSONDecodeError:
            continue
    raise TimeoutError("Timed out waiting for device response")


def send_cmd(port: serial.Serial, payload: dict):
    port.write((json.dumps(payload, ensure_ascii=False) + "\n").encode("utf-8"))
    port.flush()
    obj = read_json_line(port)
    if not obj.get("ok", False):
        raise RuntimeError(f"Device error: {obj.get('error', 'unknown')}")
    return obj


def push_file(port: serial.Serial, source: pathlib.Path, target_path: str):
    data = source.read_bytes()
    send_cmd(port, {"cmd": "sd_put_begin", "path": target_path, "size": len(data)})
    chunk_size = 96
    for offset in range(0, len(data), chunk_size):
        chunk = data[offset: offset + chunk_size]
        send_cmd(port, {
            "cmd": "sd_put_chunk",
            "data": chunk.hex().upper(),
        })
    send_cmd(port, {"cmd": "sd_put_end"})


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--port", required=True)
    parser.add_argument("--source", default=r"d:\clawputer\sdcard\pet\dialogue")
    args = parser.parse_args()

    source = pathlib.Path(args.source)
    if not source.exists():
        raise FileNotFoundError(source)

    with serial.Serial(args.port, 115200, timeout=10, write_timeout=10) as port:
        time.sleep(1.2)
        port.reset_input_buffer()
        port.reset_output_buffer()
        send_cmd(port, {"cmd": "ping"})
        send_cmd(port, {"cmd": "sd_mkdir", "path": "/pet"})
        send_cmd(port, {"cmd": "sd_mkdir", "path": "/pet/dialogue"})
        if source.is_dir():
            for file_name in ("dialogue.txt", "prompts.txt"):
                file_path = source / file_name
                if file_path.exists():
                    push_file(port, file_path, f"/pet/dialogue/{file_name}")
                    print(f"Pushed {file_name} to SD card.")
        else:
            push_file(port, source, f"/pet/dialogue/{source.name}")
            print(f"Pushed {source.name} to SD card.")


if __name__ == "__main__":
    main()
