import argparse
import json
import os
import time

import serial


def read_json_line(port: serial.Serial):
    deadline = time.time() + 10
    while time.time() < deadline:
        line = port.readline()
        if not line:
            continue
        text = line.decode("utf-8", errors="replace").strip()
        if not text:
            continue
        try:
            return json.loads(text)
        except json.JSONDecodeError:
            continue
    raise TimeoutError("Timed out waiting for device response")


def send_cmd(port: serial.Serial, payload: dict):
    port.write((json.dumps(payload, ensure_ascii=False) + "\n").encode("utf-8"))
    port.flush()
    deadline = time.time() + 10
    while time.time() < deadline:
        obj = read_json_line(port)
        if obj.get("ok", False):
            return obj
        if obj.get("error") in ("bad json", "line too long"):
            continue
        raise RuntimeError(f"Device error: {obj.get('error', 'unknown')}")
    raise TimeoutError("Timed out waiting for matching device response")


def drain_port(port: serial.Serial, seconds: float = 1.0):
    deadline = time.time() + seconds
    while time.time() < deadline:
        if port.in_waiting:
            port.readline()
        else:
            time.sleep(0.05)


def optional_arg(value: str | None, env_name: str):
    if value is not None:
        return value
    return os.environ.get(env_name)


def main():
    parser = argparse.ArgumentParser(description="Configure Catputer device over USB serial.")
    parser.add_argument("--port", required=True, help="Runtime serial port, for example COM8.")
    parser.add_argument("--wifi-ssid")
    parser.add_argument("--wifi-pass")
    parser.add_argument("--wifi-ssid2")
    parser.add_argument("--wifi-pass2")
    parser.add_argument("--api-key")
    parser.add_argument("--api-host")
    parser.add_argument("--api-port")
    parser.add_argument("--gateway-token")
    parser.add_argument("--stt-host")
    parser.add_argument("--stt-port")
    parser.add_argument("--api-host2")
    parser.add_argument("--city")
    args = parser.parse_args()

    config_values = {}
    mappings = {
        "wifi_ssid": optional_arg(args.wifi_ssid, "WIFI_SSID"),
        "wifi_pass": optional_arg(args.wifi_pass, "WIFI_PASS"),
        "wifi_ssid2": optional_arg(args.wifi_ssid2, "WIFI_SSID2"),
        "wifi_pass2": optional_arg(args.wifi_pass2, "WIFI_PASS2"),
        "api_key": optional_arg(args.api_key, "API_KEY"),
        "api_host": optional_arg(args.api_host, "API_HOST"),
        "api_port": optional_arg(args.api_port, "API_PORT"),
        "gateway_token": optional_arg(args.gateway_token, "GATEWAY_TOKEN"),
        "stt_host": optional_arg(args.stt_host, "STT_PROXY_HOST"),
        "stt_port": optional_arg(args.stt_port, "STT_PROXY_PORT"),
        "api_host2": optional_arg(args.api_host2, "API_HOST2"),
        "city": optional_arg(args.city, "DEFAULT_CITY"),
    }
    for key, value in mappings.items():
        if value:
            config_values[key] = value

    if not config_values:
        raise ValueError("No config values provided. Set environment variables or pass command-line args.")

    payloads = []
    for keys in (
        ("wifi_ssid", "wifi_pass", "wifi_ssid2", "wifi_pass2"),
        ("api_key", "api_host", "api_port", "gateway_token", "api_host2"),
        ("stt_host", "stt_port", "city"),
    ):
        payload = {"cmd": "set_config"}
        for key in keys:
            if key in config_values:
                payload[key] = config_values[key]
        if len(payload) > 1:
            payloads.append(payload)

    with serial.Serial(args.port, 115200, timeout=10, write_timeout=10) as port:
        time.sleep(1.2)
        port.reset_input_buffer()
        port.reset_output_buffer()
        drain_port(port)
        send_cmd(port, {"cmd": "ping"})
        for payload in payloads:
            send_cmd(port, payload)
        config = send_cmd(port, {"cmd": "get_config"}).get("config", {})

    print("Configured device over serial.")
    print(f"  WiFi: {config.get('wifi_ssid', '')}")
    print(f"  API host: {config.get('api_host', '')}:{config.get('api_port', '')}")
    print(f"  STT host: {config.get('stt_host', '')}:{config.get('stt_port', '')}")
    print(f"  City: {config.get('city', '')}")


if __name__ == "__main__":
    main()
