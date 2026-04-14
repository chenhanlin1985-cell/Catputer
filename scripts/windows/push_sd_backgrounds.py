import argparse
import json
import sys
import time
import zlib
from pathlib import Path

import serial
from serial.tools import list_ports


def read_json_line(port: serial.Serial, timeout=15):
    deadline = time.time() + timeout
    while True:
        if time.time() > deadline:
            raise TimeoutError("Timed out waiting for device response")
        raw = port.readline()
        if not raw:
            continue
        line = raw.decode("utf-8", errors="ignore").strip()
        if not line or not line.startswith("{"):
            continue
        obj = json.loads(line)
        if not obj.get("ok") and obj.get("error") == "bad json":
            continue
        if not obj.get("ok"):
            raise RuntimeError(f"Device error: {obj.get('error', 'unknown')}")
        return obj


def send_cmd(port: serial.Serial, payload: dict):
    port.write((json.dumps(payload, ensure_ascii=False, separators=(",", ":")) + "\n").encode("utf-8"))
    port.flush()
    return read_json_line(port)


def candidate_ports(preferred: str | None):
    seen = set()
    if preferred:
        seen.add(preferred.upper())
        yield preferred
    for info in list_ports.comports():
        port = info.device
        if port.upper() in seen:
            continue
        seen.add(port.upper())
        if "VID:PID=303A:1001" in (info.hwid or "").upper() or "303A" in (info.hwid or "").upper():
            yield port


def open_runtime_port(preferred: str | None):
    last_error = None
    for name in candidate_ports(preferred):
        print(f"Trying runtime serial port {name} ...", flush=True)
        try:
            port = serial.Serial(name, 115200, timeout=3, write_timeout=3)
            time.sleep(1.2)
            port.reset_input_buffer()
            port.reset_output_buffer()
            send_cmd(port, {"cmd": "ping"})
            print(f"Runtime serial ready: {name}", flush=True)
            return port
        except Exception as exc:
            last_error = exc
            try:
                port.close()
            except Exception:
                pass
            print(f"  {name} did not answer Catputer ping: {exc}", flush=True)
    raise RuntimeError(f"Could not find Catputer runtime serial port. Last error: {last_error}")


def push_file(port: serial.Serial, local_path: Path, remote_path: str):
    data = local_path.read_bytes()
    print(f"Uploading {local_path.name} -> {remote_path} ({len(data)} bytes)", flush=True)
    send_cmd(port, {"cmd": "sd_put_begin", "path": remote_path, "size": len(data)})
    time.sleep(1.5)
    chunk_size = 64
    next_report = 0
    for offset in range(0, len(data), chunk_size):
        chunk = data[offset:offset + chunk_size]
        send_cmd(port, {"cmd": "sd_put_chunk", "data": chunk.hex().upper()})
        done = min(offset + len(chunk), len(data))
        percent = int((done * 100) / max(1, len(data)))
        if percent >= next_report or done == len(data):
            print(f"  {local_path.name}: {done}/{len(data)} bytes ({percent}%)", flush=True)
            next_report += 10
        time.sleep(0.001)
    send_cmd(port, {"cmd": "sd_put_end"})
    expected_crc = f"{zlib.crc32(data) & 0xFFFFFFFF:08X}"
    stat = send_cmd(port, {"cmd": "sd_crc32", "path": remote_path})
    actual_crc = str(stat.get("crc32", "")).upper()
    actual_size = int(stat.get("size", -1))
    if actual_size != len(data) or actual_crc != expected_crc:
        raise RuntimeError(
            f"Verify failed for {remote_path}: device size={actual_size} crc={actual_crc}, "
            f"local size={len(data)} crc={expected_crc}"
        )
    print(f"  verified crc32={actual_crc}", flush=True)


def remove_file(port: serial.Serial, remote_path: str):
    send_cmd(port, {"cmd": "sd_remove", "path": remote_path})


def main():
    parser = argparse.ArgumentParser(description="Push Catputer SD background assets.")
    parser.add_argument("--port", default=None, help="Runtime serial port. If omitted, probe VID_303A ports with ping.")
    parser.add_argument("--source", default=r"d:\clawputer\sdcard\pet\backgrounds")
    parser.add_argument("--restart", action="store_true", help="Restart the device after uploading, so cached backgrounds are reloaded.")
    args = parser.parse_args()

    source_dir = Path(args.source)
    if not source_dir.exists():
        raise FileNotFoundError(source_dir)

    expected = ["home_morning.rgb565", "home_noon.rgb565", "home_dusk.rgb565", "home_night.rgb565"]
    with open_runtime_port(args.port) as port:
        send_cmd(port, {"cmd": "sd_mkdir", "path": "/pet"})
        send_cmd(port, {"cmd": "sd_mkdir", "path": "/pet/backgrounds"})

        for name in expected:
            local_path = source_dir / name
            if not local_path.exists():
                raise FileNotFoundError(local_path)
            remote_path = f"/pet/backgrounds/{name}"
            ready_path = remote_path.replace(".rgb565", ".ok")
            remove_file(port, ready_path)
            push_file(port, local_path, remote_path)
            marker = source_dir / (Path(name).stem + ".ok")
            marker.write_text("ok\n", encoding="utf-8")
            push_file(port, marker, ready_path)
            print(f"Pushed {name}")

        if args.restart:
            print("Restarting device so the background cache reloads from SD ...", flush=True)
            send_cmd(port, {"cmd": "restart"})

    print("SD background sync complete.")


if __name__ == "__main__":
    try:
        main()
    except Exception as exc:
        print(str(exc), file=sys.stderr)
        sys.exit(1)
