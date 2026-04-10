extends Control
class_name CatSpriteView

const HandheldSprites = preload("res://scripts/handheld_sprites.gd")
const HandheldSpritesPurple = preload("res://scripts/handheld_sprites_purple.gd")

const STATE_IDLE := "idle"
const STATE_HAPPY := "happy"
const STATE_SLEEP := "sleep"
const STATE_TALK := "talk"
const STATE_LOOK := "look"
const STATE_STRETCH := "stretch"

var fullness: int = 75
var mood: int = 70
var energy: int = 80
var cleanliness: int = 78
var cat_kind: String = "orange"
var facing_left: bool = false
var outing_active: bool = false
var sleeping_active: bool = false

var _state: String = STATE_IDLE
var _frame_index: int = 0
var _state_started_at: int = 0
var _frame_started_at: int = 0
var _talk_hold_until: int = 0

func _ready() -> void:
	custom_minimum_size = Vector2(180, 180)
	_state_started_at = Time.get_ticks_msec()
	_frame_started_at = _state_started_at

func _process(_delta: float) -> void:
	var now: int = Time.get_ticks_msec()
	var target_state: String = _resolve_target_state(now)
	if target_state != _state:
		_set_state(target_state)

	var frame_interval: int = _frame_interval_ms(_state)
	if now - _frame_started_at >= frame_interval:
		_frame_started_at = now
		_frame_index += 1
		var frames: Array[Texture2D] = _frames_for_state(_state)
		if not frames.is_empty():
			_frame_index %= frames.size()
		queue_redraw()

	if _state == STATE_HAPPY and now - _state_started_at > 1200:
		_set_state(STATE_IDLE)
	elif _state == STATE_STRETCH and now - _state_started_at > 1600:
		_set_state(STATE_IDLE)
	elif _state == STATE_LOOK and now - _state_started_at > 2400:
		_set_state(STATE_IDLE)
	elif _state == STATE_TALK and now > _talk_hold_until:
		_set_state(STATE_IDLE)

func trigger_happy() -> void:
	_set_state(STATE_HAPPY)

func trigger_talk(duration_ms: int = 1200) -> void:
	_talk_hold_until = Time.get_ticks_msec() + duration_ms
	_set_state(STATE_TALK)

func trigger_idle() -> void:
	_set_state(STATE_IDLE)

func update_stats(new_fullness: int, new_mood: int, new_energy: int, new_cleanliness: int) -> void:
	fullness = new_fullness
	mood = new_mood
	energy = new_energy
	cleanliness = new_cleanliness
	queue_redraw()

func set_cat_kind(value: String) -> void:
	cat_kind = value if not value.is_empty() else "orange"
	queue_redraw()

func set_outing_active(active: bool) -> void:
	outing_active = active
	queue_redraw()

func set_sleeping_active(active: bool) -> void:
	sleeping_active = active
	queue_redraw()

func _draw() -> void:
	var texture: Texture2D = _current_texture()
	if texture == null:
		if outing_active:
			var text := "\u5916\u51fa\u4e2d"
			var font := ThemeDB.fallback_font
			var font_size := 28
			var text_size := font.get_string_size(text, HORIZONTAL_ALIGNMENT_LEFT, -1, font_size)
			draw_string(font, (size - text_size) * 0.5, text, HORIZONTAL_ALIGNMENT_LEFT, -1, font_size, Color(0.2, 0.2, 0.24))
		return

	var draw_size: Vector2 = texture.get_size()
	var pos := (size - draw_size) * 0.5
	pos += _offset_for_state()

	if facing_left:
		draw_set_transform(pos + Vector2(draw_size.x, 0), 0.0, Vector2(-1, 1))
		draw_texture(texture, Vector2.ZERO)
		draw_set_transform(Vector2.ZERO, 0.0, Vector2.ONE)
	else:
		draw_texture(texture, pos)

	_draw_condition_overlays(pos)

func current_draw_rect() -> Rect2:
	var texture: Texture2D = _current_texture()
	if texture == null:
		var fallback_size := Vector2(48, 48)
		return Rect2((size - fallback_size) * 0.5, fallback_size)
	var draw_size: Vector2 = texture.get_size()
	var pos := (size - draw_size) * 0.5
	pos += _offset_for_state()
	return Rect2(pos, draw_size)

func _draw_condition_overlays(base_pos: Vector2) -> void:
	if cleanliness < 35:
		var dust_color := Color8(120, 98, 78)
		draw_rect(Rect2(base_pos + Vector2(12, 18), Vector2(3, 3)), dust_color)
		draw_rect(Rect2(base_pos + Vector2(30, 27), Vector2(3, 3)), dust_color)
		if cleanliness < 20:
			draw_rect(Rect2(base_pos + Vector2(21, 36), Vector2(3, 3)), dust_color)
			draw_rect(Rect2(base_pos + Vector2(39, 42), Vector2(3, 3)), dust_color)

	if energy < 25 and _state != STATE_SLEEP:
		var sleepy := Color8(70, 70, 95)
		draw_rect(Rect2(base_pos + Vector2(12, 19), Vector2(10, 1)), sleepy)
		draw_rect(Rect2(base_pos + Vector2(27, 19), Vector2(10, 1)), sleepy)

	if mood < 25 and _state != STATE_HAPPY:
		var pout := Color8(60, 40, 40)
		var pout_pixels := [
			Vector2(22, 32), Vector2(23, 33), Vector2(24, 33),
			Vector2(25, 33), Vector2(26, 32),
		]
		for point: Vector2 in pout_pixels:
			draw_rect(Rect2(base_pos + point, Vector2.ONE), pout)

	if fullness < 20:
		var weak := Color8(255, 220, 140)
		draw_arc(base_pos + Vector2(6, 8), 3.0, 0.0, TAU, 12, weak, 1.0)
		draw_string(ThemeDB.fallback_font, base_pos + Vector2(4, 8), "!", HORIZONTAL_ALIGNMENT_LEFT, -1, 12, weak)

func _current_texture() -> Texture2D:
	var frames: Array[Texture2D] = _frames_for_state(_state)
	if frames.is_empty():
		return null
	return frames[_frame_index % frames.size()]

func _frames_for_state(state_name: String) -> Array[Texture2D]:
	var sprite_source := HandheldSpritesPurple if cat_kind == "purple" else HandheldSprites
	match state_name:
		STATE_LOOK:
			return sprite_source.get_frames(STATE_IDLE)
		STATE_STRETCH:
			return sprite_source.get_frames(STATE_HAPPY)
		_:
			return sprite_source.get_frames(state_name)

func _frame_interval_ms(state_name: String) -> int:
	match state_name:
		STATE_IDLE:
			return 500
		STATE_HAPPY:
			return 200
		STATE_SLEEP:
			return 1000
		STATE_TALK:
			return 250
		STATE_STRETCH:
			return 400
		STATE_LOOK:
			return 300
		_:
			return 500

func _offset_for_state() -> Vector2:
	match _state:
		STATE_HAPPY:
			return Vector2(0, -6 if _frame_index % 2 == 0 else 0)
		STATE_LOOK:
			return Vector2(-3 if _frame_index % 2 == 0 else 3, 0)
		STATE_STRETCH:
			return Vector2(0, -3 if _frame_index % 2 == 0 else 0)
		_:
			return Vector2.ZERO

func _resolve_target_state(now: int) -> String:
	if _state == STATE_TALK and now <= _talk_hold_until:
		return STATE_TALK
	if outing_active:
		return STATE_IDLE
	if sleeping_active:
		return STATE_SLEEP
	return STATE_SLEEP if energy < 18 else STATE_IDLE

func _set_state(new_state: String) -> void:
	_state = new_state
	_frame_index = 0
	_state_started_at = Time.get_ticks_msec()
	_frame_started_at = _state_started_at
	queue_redraw()
