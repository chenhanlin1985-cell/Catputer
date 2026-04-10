extends Node
class_name DeviceBridge

signal listen_state_changed(listening: bool, message: String)
signal state_packet_received(data: Dictionary)
signal chat_packet_received(role: String, text: String)
signal raw_packet_received(text: String)

const BROADCAST_PORT := 19820
const COMMAND_TCP_PORT := 19821
const COMMAND_UDP_PORT := 19822

var udp: PacketPeerUDP
var listening: bool = false

func start_listening(port: int = BROADCAST_PORT) -> bool:
	stop_listening()
	udp = PacketPeerUDP.new()
	var result: Error = udp.bind(port, "0.0.0.0")
	if result != OK:
		listening = false
		emit_signal("listen_state_changed", false, "\u76d1\u542c\u5931\u8d25: %s" % error_string(result))
		udp = null
		return false
	listening = true
	emit_signal("listen_state_changed", true, "\u6b63\u5728\u76d1\u542c %d" % port)
	return true

func stop_listening() -> void:
	if udp != null:
		udp.close()
	udp = null
	if listening:
		listening = false
		emit_signal("listen_state_changed", false, "\u5df2\u505c\u6b62\u76d1\u542c")

func poll() -> void:
	if not listening or udp == null:
		return
	while udp.get_available_packet_count() > 0:
		var packet: PackedByteArray = udp.get_packet()
		var text: String = packet.get_string_from_utf8()
		emit_signal("raw_packet_received", text)
		var parsed: Variant = JSON.parse_string(text)
		if not (parsed is Dictionary):
			continue
		var data: Dictionary = parsed
		if data.has("type"):
			var packet_type: String = String(data.get("type", ""))
			if packet_type == "chat":
				emit_signal("chat_packet_received", String(data.get("role", "")), String(data.get("text", "")))
		else:
			emit_signal("state_packet_received", data)

func send_text(host: String, text: String, auto_send: bool = false) -> bool:
	if host.strip_edges().is_empty() or text.strip_edges().is_empty():
		return false
	var sender: PacketPeerUDP = PacketPeerUDP.new()
	var packet: Dictionary = {
		"cmd": ("say" if auto_send else "text"),
		"msg": text,
	}
	sender.set_dest_address(host.strip_edges(), COMMAND_UDP_PORT)
	var bytes: PackedByteArray = JSON.stringify(packet).to_utf8_buffer()
	var result: Error = sender.put_packet(bytes)
	sender.close()
	return result == OK

func request_sync_enter(host: String) -> Dictionary:
	return _send_tcp_command(host, {"cmd": "sync_enter"})

func request_sync_ping(host: String) -> Dictionary:
	return _send_tcp_command(host, {"cmd": "sync_ping"})

func request_sync_leave(host: String, snapshot: Dictionary) -> Dictionary:
	return _send_tcp_command(host, {"cmd": "sync_leave", "snapshot": snapshot}, 3.5)

func _send_tcp_command(host: String, payload: Dictionary, timeout_seconds: float = 2.5) -> Dictionary:
	var clean_host := host.strip_edges()
	if clean_host.is_empty():
		return {"ok": false, "error": "缺少设备IP"}

	var tcp := StreamPeerTCP.new()
	var result: Error = tcp.connect_to_host(clean_host, COMMAND_TCP_PORT)
	if result != OK:
		return {"ok": false, "error": "连接失败: %s" % error_string(result)}

	var deadline := Time.get_ticks_msec() + int(timeout_seconds * 1000.0)
	while tcp.get_status() == StreamPeerTCP.STATUS_CONNECTING and Time.get_ticks_msec() < deadline:
		tcp.poll()
		OS.delay_msec(10)

	if tcp.get_status() != StreamPeerTCP.STATUS_CONNECTED:
		tcp.disconnect_from_host()
		return {"ok": false, "error": "连接设备超时"}

	var packet_text := JSON.stringify(payload) + "\n"
	result = tcp.put_data(packet_text.to_utf8_buffer())
	if result != OK:
		tcp.disconnect_from_host()
		return {"ok": false, "error": "发送失败: %s" % error_string(result)}

	var response := PackedByteArray()
	while Time.get_ticks_msec() < deadline:
		tcp.poll()
		var available := tcp.get_available_bytes()
		if available > 0:
			var chunk := tcp.get_data(available)
			if chunk[0] != OK:
				tcp.disconnect_from_host()
				return {"ok": false, "error": "读取失败: %s" % error_string(chunk[0])}
			response.append_array(chunk[1])
			if response.find(10) >= 0:
				break
		elif tcp.get_status() == StreamPeerTCP.STATUS_NONE:
			break
		OS.delay_msec(10)

	tcp.disconnect_from_host()
	var text := response.get_string_from_utf8().strip_edges()
	if text.is_empty():
		return {"ok": false, "error": "设备没有返回内容"}
	var parsed: Variant = JSON.parse_string(text)
	if parsed is Dictionary:
		return parsed
	return {"ok": false, "error": "返回格式不对", "raw": text}
