extends Control
class_name CatHouseView

signal pet_selected(pet_id: String)

const CatSpriteView = preload("res://scripts/cat_sprite_view.gd")
const PetState = preload("res://scripts/pet_state.gd")

const FLOOR_TOP := 248.0
const LIVING_ROOM_BOTTOM := 210.0
const CAT_SIZE := Vector2(112, 112)
const BUBBLE_BG := Color8(255, 246, 238)
const BUBBLE_BORDER := Color8(178, 142, 126)
const INTERACTION_COLOR := Color8(255, 192, 128)

var _entries: Array[Dictionary] = []
var _active_pet_id: String = ""
var _current_device_pet_id: String = ""
var _selected_return_pet_id: String = ""
var _town_sync_active: bool = false
var _room_spots: Array[Dictionary] = []
var _roster_signature: String = ""

func _ready() -> void:
	custom_minimum_size = Vector2(0, 420)
	clip_contents = true
	mouse_filter = Control.MOUSE_FILTER_PASS
	set_process(true)
	_rebuild_room_spots()

func _resized() -> void:
	_rebuild_room_spots()
	queue_redraw()

func set_roster(cats: Array[PetState], active_pet_id: String, current_device_pet_id: String, selected_return_pet_id: String, town_sync_active: bool) -> void:
	_active_pet_id = active_pet_id
	_current_device_pet_id = current_device_pet_id
	_selected_return_pet_id = selected_return_pet_id
	_town_sync_active = town_sync_active

	var signature_parts: Array[String] = []
	for pet in cats:
		if not _current_device_pet_id.is_empty() and pet.pet_id == _current_device_pet_id:
			continue
		signature_parts.append("%s:%s" % [pet.pet_id, pet.pet_kind])
	var next_signature := "|".join(signature_parts)

	if next_signature == _roster_signature and _entries.size() == signature_parts.size():
		for entry in _entries:
			var pet: PetState = entry["pet"]
			var sprite: CatSpriteView = entry["sprite"]
			sprite.set_cat_kind(pet.pet_kind)
			sprite.set_sleeping_active(pet.is_sleeping)
			sprite.set_outing_active(pet.is_outing)
			sprite.facing_left = pet.facing_left
			sprite.update_stats(pet.fullness, pet.mood, pet.energy, pet.cleanliness)
		queue_redraw()
		return

	_roster_signature = next_signature

	for child in get_children():
		child.queue_free()
	_entries.clear()

	var columns: int = max(2, int(floor(size.x / 180.0))) if size.x > 0 else 3
	var start_x: float = 42.0
	var gap_x: float = 148.0
	var living_row: int = 0
	var bedroom_row: int = 0

	for i in cats.size():
		var pet: PetState = cats[i]
		if not _current_device_pet_id.is_empty() and pet.pet_id == _current_device_pet_id:
			continue
		var sprite: CatSpriteView = CatSpriteView.new()
		sprite.custom_minimum_size = CAT_SIZE
		sprite.size = CAT_SIZE
		sprite.set_cat_kind(pet.pet_kind)
		sprite.set_sleeping_active(pet.is_sleeping)
		sprite.set_outing_active(pet.is_outing)
		sprite.facing_left = pet.facing_left
		sprite.update_stats(pet.fullness, pet.mood, pet.energy, pet.cleanliness)
		sprite.mouse_filter = Control.MOUSE_FILTER_STOP
		sprite.gui_input.connect(func(event: InputEvent) -> void:
			if event is InputEventMouseButton and event.pressed and event.button_index == MOUSE_BUTTON_LEFT:
				pet_selected.emit(pet.pet_id)
		)
		add_child(sprite)

		var room := _preferred_room_for(pet)
		var row_index := living_row if room == "living" else bedroom_row
		var col: int = row_index % columns
		var row: int = row_index / columns
		var base_y: float = 118.0 if room == "living" else FLOOR_TOP + 12.0
		var row_gap: float = 84.0 if room == "living" else 72.0
		var base_pos: Vector2 = Vector2(start_x + col * gap_x, base_y + row * row_gap)
		if room == "living":
			living_row += 1
		else:
			bedroom_row += 1
		var jitter: Vector2 = Vector2(randf_range(-18.0, 18.0), randf_range(-10.0, 10.0))
		var entry: Dictionary = {
			"pet": pet,
			"sprite": sprite,
			"position": base_pos + jitter,
			"target": base_pos + jitter,
			"room": _preferred_room_for(pet),
			"rest_at": Time.get_ticks_msec() + randi_range(800, 2200),
			"next_living_line_at": Time.get_ticks_msec() + randi_range(2600, 6400),
			"hop_phase": randf_range(0.0, TAU),
			"spot_hint": "",
			"spot_id": "",
			"next_spot_effect_at": Time.get_ticks_msec() + randi_range(2200, 4800),
			"next_social_at": Time.get_ticks_msec() + randi_range(2600, 5200),
		}
		_entries.append(entry)
		_apply_entry_visual(entry)

	queue_redraw()

func _process(delta: float) -> void:
	if _entries.is_empty():
		return
	var now: int = Time.get_ticks_msec()
	for entry in _entries:
		var pet: PetState = entry["pet"]
		var sprite: CatSpriteView = entry["sprite"]
		if pet.is_outing:
			sprite.visible = false
			continue
		sprite.visible = true

		var pos: Vector2 = entry["position"]
		var target: Vector2 = entry["target"]
		if now >= int(entry["rest_at"]):
			entry["room"] = _preferred_room_for(pet)
			var next_target: Dictionary = _next_target_for(pos, pet)
			entry["spot_hint"] = String(next_target.get("label", ""))
			entry["spot_id"] = String(next_target.get("spot_id", ""))
			entry["target"] = Vector2(next_target.get("position", pos))
			entry["rest_at"] = now + _next_rest_delay_ms(pet)
			target = Vector2(entry["target"])

		var step: float = min(60.0 * delta, pos.distance_to(target))
		if step > 0.01:
			var dir: Vector2 = (target - pos).normalized()
			pos += dir * step
			entry["position"] = pos
			sprite.facing_left = dir.x < 0.0
			_apply_entry_visual(entry)
		elif now >= int(entry.get("next_spot_effect_at", now + 1)):
			_apply_spot_effect(entry)
			entry["next_spot_effect_at"] = now + _next_spot_effect_delay_ms(pet)
		_maybe_emit_living_line(entry, now)
	queue_redraw()

	_process_social_moments(now)

func _draw() -> void:
	var wall := Rect2(0, 0, size.x, FLOOR_TOP)
	var floor := Rect2(0, FLOOR_TOP, size.x, size.y - FLOOR_TOP)
	draw_rect(wall, Color8(244, 233, 220), true)
	draw_rect(floor, Color8(210, 176, 136), true)
	draw_rect(Rect2(0, LIVING_ROOM_BOTTOM, size.x, 2), Color8(224, 208, 190), true)

	for y in range(18, int(FLOOR_TOP) - 8, 28):
		draw_line(Vector2(0, y), Vector2(size.x, y), Color8(236, 224, 210), 1.0)

	_draw_window(Rect2(36, 34, 128, 94))
	_draw_shelf(Rect2(size.x - 174, 52, 116, 12))
	_draw_bed(Rect2(size.x - 214, FLOOR_TOP - 76, 168, 56))
	_draw_rug(Rect2(size.x * 0.5 - 110, FLOOR_TOP + 18, 220, 86))
	_draw_food_bowls()
	_draw_room_labels()
	if _entries.is_empty():
		_draw_empty_hint()
		return
	_draw_cat_overlays()
	_draw_cat_interactions()

func _draw_window(rect: Rect2) -> void:
	draw_rect(rect, Color8(141, 108, 84), true)
	draw_rect(rect.grow(-6), Color8(186, 224, 250), true)
	draw_line(Vector2(rect.position.x + rect.size.x * 0.5, rect.position.y + 6), Vector2(rect.position.x + rect.size.x * 0.5, rect.position.y + rect.size.y - 6), Color8(141, 108, 84), 2.0)
	draw_line(Vector2(rect.position.x + 6, rect.position.y + rect.size.y * 0.5), Vector2(rect.position.x + rect.size.x - 6, rect.position.y + rect.size.y * 0.5), Color8(141, 108, 84), 2.0)

func _draw_shelf(rect: Rect2) -> void:
	draw_rect(rect, Color8(155, 113, 84), true)
	draw_rect(Rect2(rect.position.x + 10, rect.position.y - 26, 16, 26), Color8(243, 188, 124), true)
	draw_rect(Rect2(rect.position.x + 40, rect.position.y - 22, 14, 22), Color8(189, 155, 210), true)
	draw_rect(Rect2(rect.position.x + 70, rect.position.y - 18, 18, 18), Color8(120, 162, 108), true)

func _draw_bed(rect: Rect2) -> void:
	draw_rect(rect, Color8(198, 157, 122), true)
	draw_rect(Rect2(rect.position.x + 8, rect.position.y + 8, rect.size.x - 16, rect.size.y - 14), Color8(248, 228, 208), true)
	draw_rect(Rect2(rect.position.x + 20, rect.position.y + 12, 42, 18), Color8(255, 208, 198), true)
	draw_rect(Rect2(rect.position.x + 68, rect.position.y + 12, 42, 18), Color8(255, 208, 198), true)

func _draw_rug(rect: Rect2) -> void:
	draw_rect(rect, Color8(220, 143, 124), true)
	draw_rect(rect.grow(-8), Color8(245, 188, 168), false)

func _draw_food_bowls() -> void:
	draw_circle(Vector2(72, FLOOR_TOP + 42), 12.0, Color8(184, 204, 214))
	draw_circle(Vector2(72, FLOOR_TOP + 42), 8.0, Color8(248, 232, 196))
	draw_circle(Vector2(112, FLOOR_TOP + 42), 12.0, Color8(184, 204, 214))
	draw_circle(Vector2(112, FLOOR_TOP + 42), 8.0, Color8(156, 196, 208))

func _draw_room_labels() -> void:
	var font: Font = ThemeDB.fallback_font
	draw_string(font, Vector2(24, 20), "客厅", HORIZONTAL_ALIGNMENT_LEFT, -1, 12, Color8(142, 112, 92))
	draw_string(font, Vector2(24, LIVING_ROOM_BOTTOM + 18), "卧室", HORIZONTAL_ALIGNMENT_LEFT, -1, 12, Color8(142, 112, 92))

func _draw_empty_hint() -> void:
	var font: Font = ThemeDB.fallback_font
	var title := "猫屋里现在很安静"
	var body := "当前这只猫在端侧活动，电脑里的其他猫暂时没有待在这里。"
	var title_size := font.get_string_size(title, HORIZONTAL_ALIGNMENT_LEFT, -1, 16)
	var cx := size.x * 0.5
	var top := FLOOR_TOP - 78.0
	var panel := Rect2(cx - 170.0, top, 340.0, 58.0)
	draw_rect(panel, Color8(252, 245, 236, 220), true)
	draw_rect(panel, Color8(190, 168, 146), false)
	draw_string(font, Vector2(cx - title_size.x * 0.5, top + 20.0), title, HORIZONTAL_ALIGNMENT_LEFT, -1, 16, Color8(94, 78, 62))
	draw_string(font, Vector2(cx - 140.0, top + 42.0), body, HORIZONTAL_ALIGNMENT_LEFT, 280, 12, Color8(120, 102, 88))

func _draw_cat_overlays() -> void:
	var font: Font = ThemeDB.fallback_font
	for entry in _entries:
		var pet: PetState = entry["pet"]
		var sprite: CatSpriteView = entry["sprite"]
		if pet.is_outing:
			continue
		var pos: Vector2 = entry["position"]
		var draw_rect := sprite.current_draw_rect()
		var world_rect := Rect2(pos + draw_rect.position, draw_rect.size)
		var name_size := font.get_string_size(pet.pet_name, HORIZONTAL_ALIGNMENT_LEFT, -1, 15)
		var name_pos := Vector2(
			world_rect.position.x + world_rect.size.x * 0.5 - name_size.x * 0.5,
			world_rect.position.y - 8
		)
		var name_color := Color8(78, 60, 54)
		if pet.pet_id == _current_device_pet_id:
			name_color = Color8(78, 116, 186)
		elif pet.pet_id == _selected_return_pet_id:
			name_color = Color8(172, 108, 72)
		draw_string(font, name_pos, pet.pet_name, HORIZONTAL_ALIGNMENT_LEFT, -1, 15, name_color)

		if pet.pet_id == _active_pet_id:
			var glow_size := Vector2(max(32.0, world_rect.size.x - 4.0), 14.0)
			var glow_pos := Vector2(
				world_rect.position.x + world_rect.size.x * 0.5 - glow_size.x * 0.5,
				world_rect.position.y + world_rect.size.y - 2.0
			)
			var glow_center := glow_pos + glow_size * 0.5
			draw_set_transform(glow_center, 0.0, Vector2(glow_size.x / glow_size.y, 1.0))
			draw_circle(Vector2.ZERO, glow_size.y * 0.5, Color(1.0, 0.84, 0.52, 0.22))
			draw_set_transform(Vector2.ZERO, 0.0, Vector2.ONE)

		var bubble_text := _bubble_text_for(entry)
		if not bubble_text.is_empty():
			_draw_prompt_bubble(
				Vector2(world_rect.position.x + world_rect.size.x * 0.5, world_rect.position.y - 16),
				bubble_text,
				pet.has_active_prompt()
			)

func _draw_cat_interactions() -> void:
	for i in range(_entries.size()):
		for j in range(i + 1, _entries.size()):
			var a: Dictionary = _entries[i]
			var b: Dictionary = _entries[j]
			var pet_a: PetState = a["pet"]
			var pet_b: PetState = b["pet"]
			var sprite_a: CatSpriteView = a["sprite"]
			var sprite_b: CatSpriteView = b["sprite"]
			if pet_a.is_outing or pet_b.is_outing:
				continue
			var rect_a := Rect2(Vector2(a["position"]) + sprite_a.current_draw_rect().position, sprite_a.current_draw_rect().size)
			var rect_b := Rect2(Vector2(b["position"]) + sprite_b.current_draw_rect().position, sprite_b.current_draw_rect().size)
			var pos_a: Vector2 = Vector2(rect_a.position.x + rect_a.size.x * 0.5, rect_a.position.y + rect_a.size.y * 0.55)
			var pos_b: Vector2 = Vector2(rect_b.position.x + rect_b.size.x * 0.5, rect_b.position.y + rect_b.size.y * 0.55)
			var distance := pos_a.distance_to(pos_b)
			if distance < 110.0:
				var center := pos_a.lerp(pos_b, 0.5)
				draw_line(pos_a, pos_b, Color8(214, 188, 162, 90), 1.0)
				if distance < 72.0:
					draw_circle(center + Vector2(-5, -8), 2.0, INTERACTION_COLOR)
					draw_circle(center + Vector2(5, -8), 2.0, INTERACTION_COLOR)
					draw_circle(center + Vector2(0, -2), 2.0, INTERACTION_COLOR)

func _process_social_moments(now: int) -> void:
	for i in range(_entries.size()):
		for j in range(i + 1, _entries.size()):
			var a: Dictionary = _entries[i]
			var b: Dictionary = _entries[j]
			var pet_a: PetState = a["pet"]
			var pet_b: PetState = b["pet"]
			var sprite_a: CatSpriteView = a["sprite"]
			var sprite_b: CatSpriteView = b["sprite"]
			if pet_a.is_outing or pet_b.is_outing:
				continue
			if now < int(a.get("next_social_at", 0)) or now < int(b.get("next_social_at", 0)):
				continue
			var rect_a := Rect2(Vector2(a["position"]) + sprite_a.current_draw_rect().position, sprite_a.current_draw_rect().size)
			var rect_b := Rect2(Vector2(b["position"]) + sprite_b.current_draw_rect().position, sprite_b.current_draw_rect().size)
			var pos_a: Vector2 = Vector2(rect_a.position.x + rect_a.size.x * 0.5, rect_a.position.y + rect_a.size.y * 0.55)
			var pos_b: Vector2 = Vector2(rect_b.position.x + rect_b.size.x * 0.5, rect_b.position.y + rect_b.size.y * 0.55)
			if pos_a.distance_to(pos_b) >= 74.0:
				continue
			_apply_social_effect(pet_a, pet_b)
			a["next_social_at"] = now + _next_social_delay_ms(pet_a)
			b["next_social_at"] = now + _next_social_delay_ms(pet_b)

func _draw_small_badge(at: Vector2, text: String, border: Color, fill: Color) -> void:
	var font: Font = ThemeDB.fallback_font
	var text_size: Vector2 = font.get_string_size(text, HORIZONTAL_ALIGNMENT_LEFT, -1, 12)
	var rect := Rect2(at.x, at.y, text_size.x + 12, 18)
	draw_rect(rect, fill, true)
	draw_rect(rect, border, false)
	draw_string(font, Vector2(rect.position.x + 6, rect.position.y + 13), text, HORIZONTAL_ALIGNMENT_LEFT, -1, 12, border)

func _draw_prompt_bubble(at: Vector2, text: String, emphasized: bool = false) -> void:
	if text.is_empty():
		return
	var font: Font = ThemeDB.fallback_font
	var shown := text.substr(0, 12)
	var text_size: Vector2 = font.get_string_size(shown, HORIZONTAL_ALIGNMENT_LEFT, -1, 13)
	var rect := Rect2(at.x - text_size.x * 0.5 - 10, at.y - 24, text_size.x + 20, 20)
	var bubble_bg := BUBBLE_BG
	var bubble_border := BUBBLE_BORDER
	var text_color := Color8(90, 68, 64)
	if emphasized:
		bubble_bg = Color8(255, 246, 220)
		bubble_border = Color8(206, 144, 82)
		text_color = Color8(110, 72, 36)
	draw_rect(rect, bubble_bg, true)
	draw_rect(rect, bubble_border, false)
	draw_string(font, Vector2(rect.position.x + 10, rect.position.y + 14), shown, HORIZONTAL_ALIGNMENT_LEFT, -1, 13, text_color)
	var tail := PackedVector2Array([
		Vector2(at.x - 5, rect.position.y + rect.size.y),
		Vector2(at.x + 5, rect.position.y + rect.size.y),
		Vector2(at.x, rect.position.y + rect.size.y + 7),
	])
	draw_colored_polygon(tail, bubble_bg)
	draw_polyline(tail + PackedVector2Array([tail[0]]), bubble_border, 1.0)

func _maybe_emit_living_line(entry: Dictionary, now: int) -> void:
	var pet: PetState = entry["pet"]
	if String(entry.get("room", "bedroom")) != "living":
		return
	if pet.is_outing or pet.has_active_prompt():
		return
	if not pet.current_ambient_line().strip_edges().is_empty():
		return
	if now < int(entry.get("next_living_line_at", now + 1)):
		return
	pet.say_ambient(pet.ambient_line_for("idle"), 6.5)
	entry["next_living_line_at"] = now + _next_living_line_delay_ms(pet)

func _bubble_text_for(entry: Dictionary) -> String:
	var pet: PetState = entry["pet"]
	if pet.has_active_prompt():
		return String(pet.active_prompt.get("title", "")).strip_edges()
	var ambient := pet.current_ambient_line().strip_edges()
	if not ambient.is_empty():
		return ambient
	if String(entry.get("room", "bedroom")) == "living":
		var hint := String(entry.get("spot_hint", "")).strip_edges()
		if not hint.is_empty():
			return hint
	return ""

func _apply_entry_visual(entry: Dictionary) -> void:
	var pet: PetState = entry["pet"]
	var sprite: CatSpriteView = entry["sprite"]
	sprite.set_cat_kind(pet.pet_kind)
	sprite.set_sleeping_active(pet.is_sleeping)
	sprite.set_outing_active(pet.is_outing)
	sprite.update_stats(pet.fullness, pet.mood, pet.energy, pet.cleanliness)
	var bounce := sin(Time.get_ticks_msec() / 450.0 + float(entry.get("hop_phase", 0.0))) * 2.0
	sprite.position = Vector2(entry["position"].x, entry["position"].y + bounce)
	sprite.queue_redraw()

func _apply_spot_effect(entry: Dictionary) -> void:
	var pet: PetState = entry["pet"]
	var spot_id: String = String(entry.get("spot_id", ""))
	if spot_id.is_empty() or pet.is_outing:
		return

	match spot_id:
		"bed":
			pet.energy = min(PetState.MAX_STAT, pet.energy + 3)
			pet.mood = min(PetState.MAX_STAT, pet.mood + 1)
			var line := pet.house_event_line("bed", "在小床边眯了一会儿")
			pet.say_ambient(line, 7.5)
			_push_pet_event(pet, line)
		"bowls":
			pet.fullness = min(PetState.MAX_STAT, pet.fullness + 2)
			var line := pet.house_event_line("bowls", "在食盆附近闻了闻")
			pet.say_ambient(line, 7.5)
			_push_pet_event(pet, line)
		"window":
			pet.mood = min(PetState.MAX_STAT, pet.mood + 2)
			var line := pet.house_event_line("window", "趴在窗边看了一会儿")
			pet.say_ambient(line, 7.5)
			_push_pet_event(pet, line)
		"rug":
			pet.mood = min(PetState.MAX_STAT, pet.mood + 2)
			pet.cleanliness = max(0, pet.cleanliness - 1)
			var line := pet.house_event_line("rug", "在地毯上打了个滚")
			pet.say_ambient(line, 7.5)
			_push_pet_event(pet, line)
		"shelf":
			pet.bond = min(PetState.MAX_STAT, pet.bond + 1)
			pet.mood = min(PetState.MAX_STAT, pet.mood + 1)
			var line := pet.house_event_line("shelf", "盯着架子发了会儿呆")
			pet.say_ambient(line, 7.5)
			_push_pet_event(pet, line)

func _apply_social_effect(pet_a: PetState, pet_b: PetState) -> void:
	pet_a.mood = min(PetState.MAX_STAT, pet_a.mood + 2)
	pet_b.mood = min(PetState.MAX_STAT, pet_b.mood + 2)
	pet_a.bond = min(PetState.MAX_STAT, pet_a.bond + 1)
	pet_b.bond = min(PetState.MAX_STAT, pet_b.bond + 1)
	var line_a := pet_a.house_event_line("social", pet_b.pet_name)
	var line_b := pet_b.house_event_line("social", pet_a.pet_name)
	pet_a.say_ambient(line_a, 7.5)
	pet_b.say_ambient(line_b, 7.5)
	_push_pet_event(pet_a, line_a)
	_push_pet_event(pet_b, line_b)

func _push_pet_event(pet: PetState, text: String) -> void:
	pet.event_history.push_front(text)
	while pet.event_history.size() > 12:
		pet.event_history.pop_back()

func _next_target_for(from_pos: Vector2, pet: PetState) -> Dictionary:
	var left: float = 24.0
	var right: float = max(left, size.x - CAT_SIZE.x - 24.0)
	var room: String = _preferred_room_for(pet)
	var top: float = 104.0 if room == "living" else LIVING_ROOM_BOTTOM + 10.0
	var bottom: float = (LIVING_ROOM_BOTTOM - CAT_SIZE.y + 12.0) if room == "living" else max(top, size.y - CAT_SIZE.y - 24.0)
	var candidate := Vector2(
		clamp(from_pos.x + randf_range(-72.0, 72.0), left, right),
		clamp(from_pos.y + randf_range(-42.0, 42.0), top, bottom)
	)
	var chosen_label := ""
	var chosen_id := ""
	var spot_bias := 0.62
	match pet.personality:
		PetState.PERSONALITY_CLINGY:
			spot_bias = 0.72
		PetState.PERSONALITY_CALM:
			spot_bias = 0.84
		PetState.PERSONALITY_ALOOF:
			spot_bias = 0.68
	if not _room_spots.is_empty() and randf() < spot_bias:
		var preferred: Dictionary = _pick_room_spot(pet)
		candidate = Vector2(preferred.get("position", candidate))
		chosen_label = pet.house_prompt_line(String(preferred.get("id", "")), String(preferred.get("label", "")))
		chosen_id = String(preferred.get("id", ""))
	for other in _entries:
		var other_pos: Vector2 = Vector2(other.get("position", candidate))
		if candidate.distance_to(other_pos) < 72.0:
			candidate.x = clamp(candidate.x + randf_range(54.0, 86.0), left, right)
			candidate.y = clamp(candidate.y + randf_range(-24.0, 24.0), top, bottom)
	return {"position": candidate, "label": chosen_label, "spot_id": chosen_id}

func _preferred_room_for(pet: PetState) -> String:
	if pet.pet_id == _active_pet_id:
		return "living"
	if pet.has_active_prompt():
		return "living"
	if pet.current_ambient_line().strip_edges() != "":
		return "living"
	if pet.pet_id == _current_device_pet_id:
		return "living"
	if pet.pet_id == _selected_return_pet_id:
		return "living"
	return "bedroom"

func _next_rest_delay_ms(pet: PetState) -> int:
	match pet.personality:
		PetState.PERSONALITY_LIVELY:
			return randi_range(1200, 2400)
		PetState.PERSONALITY_CALM:
			return randi_range(2400, 4200)
		PetState.PERSONALITY_ALOOF:
			return randi_range(2100, 3600)
		_:
			return randi_range(1600, 3200)

func _next_spot_effect_delay_ms(pet: PetState) -> int:
	match pet.personality:
		PetState.PERSONALITY_LIVELY:
			return randi_range(3200, 5600)
		PetState.PERSONALITY_CALM:
			return randi_range(5400, 9000)
		PetState.PERSONALITY_ALOOF:
			return randi_range(5000, 8200)
		_:
			return randi_range(4200, 7600)

func _next_social_delay_ms(pet: PetState) -> int:
	match pet.personality:
		PetState.PERSONALITY_CLINGY:
			return randi_range(4200, 7200)
		PetState.PERSONALITY_ALOOF:
			return randi_range(7600, 11400)
		PetState.PERSONALITY_CALM:
			return randi_range(7000, 10800)
		_:
			return randi_range(6200, 9800)

func _next_living_line_delay_ms(pet: PetState) -> int:
	match pet.personality:
		PetState.PERSONALITY_CLINGY:
			return randi_range(5200, 9000)
		PetState.PERSONALITY_CALM:
			return randi_range(9800, 14000)
		PetState.PERSONALITY_ALOOF:
			return randi_range(9000, 13200)
		_:
			return randi_range(7000, 11200)

func _rebuild_room_spots() -> void:
	_room_spots = [
		{"id": "window", "label": "\u5728\u7a97\u8fb9\u53d1\u5446", "position": Vector2(64, 126), "room": "living"},
		{"id": "shelf", "label": "\u76ef\u7740\u67b6\u5b50\u770b", "position": Vector2(size.x - 164, 126), "room": "living"},
		{"id": "bed", "label": "\u722c\u5230\u5c0f\u5e8a\u8fb9", "position": Vector2(size.x - 146, FLOOR_TOP - 34), "room": "bedroom"},
		{"id": "rug", "label": "\u5728\u5730\u6bef\u4e0a\u6253\u6eda", "position": Vector2(size.x * 0.5 - 12, FLOOR_TOP + 34), "room": "bedroom"},
		{"id": "bowls", "label": "\u53bb\u98df\u76c6\u9644\u8fd1", "position": Vector2(62, FLOOR_TOP + 18), "room": "bedroom"},
	]

func _pick_room_spot(pet: PetState) -> Dictionary:
	var room := _preferred_room_for(pet)
	var preferred_id := "rug"
	match pet.personality:
		PetState.PERSONALITY_CLINGY:
			preferred_id = "window"
		PetState.PERSONALITY_CALM:
			preferred_id = "bed"
		PetState.PERSONALITY_ALOOF:
			preferred_id = "shelf"
		_:
			preferred_id = "bowls" if pet.fullness < 40 else "rug"

	var options: Array[Dictionary] = []
	for spot in _room_spots:
		if String(spot.get("room", room)) != room:
			continue
		if String(spot.get("id", "")) == preferred_id or randf() < 0.22:
			options.append(spot)
	if options.is_empty():
		for spot in _room_spots:
			if String(spot.get("room", room)) == room:
				options.append(spot)
	return options[randi() % options.size()]
