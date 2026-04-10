extends Control
class_name CompanionStage

const CatSpriteView = preload("res://scripts/cat_sprite_view.gd")
const PetState = preload("res://scripts/pet_state.gd")

const VIRTUAL_SIZE := Vector2(240, 135)
const GROUND_Y := 107.0
const SKY_DAY := Color8(60, 120, 200)
const SKY_SUNSET := Color8(180, 80, 60)
const SKY_NIGHT := Color8(10, 10, 30)
const GROUND_DAY := Color8(80, 140, 60)
const GROUND_DAY_TOP := Color8(100, 170, 70)
const GROUND_NIGHT := Color8(50, 92, 44)
const GROUND_NIGHT_TOP := Color8(74, 122, 58)
const SUN_COLOR := Color8(255, 220, 60)
const SUNSET_SUN := Color8(255, 160, 40)
const MOON_COLOR := Color8(220, 220, 200)
const CLOUD_COLOR := Color8(220, 230, 240)
const STAR_COLOR := Color8(255, 255, 220)
const STATUS_DIM := Color8(184, 192, 210)

const WEATHER_NONE := -1
const WEATHER_CLEAR := 0
const WEATHER_PARTLY_CLOUDY := 1
const WEATHER_OVERCAST := 2
const WEATHER_FOG := 3
const WEATHER_DRIZZLE := 4
const WEATHER_RAIN := 5
const WEATHER_SNOW := 6
const WEATHER_THUNDER := 7

var pet: PetState
var cat_view: CatSpriteView
var stars: Array[Vector2] = []
var weather_type: int = WEATHER_NONE
var temperature_text: String = ""
var humidity_text: String = ""
var status_text: String = ""
var rain_offset: int = 0
var snow_offset: int = 0
var focus_mode_active: bool = false
var focus_mode_activity: String = ""

func _ready() -> void:
	clip_contents = true
	focus_mode = Control.FOCUS_ALL
	custom_minimum_size = Vector2(0, 220)
	resized.connect(_refresh_cat)
	for i in 12:
		stars.append(Vector2(10 + (i * 17) % 210, 8 + (i * 11) % 38))
	cat_view = CatSpriteView.new()
	cat_view.size = Vector2(PetState.CHAR_DRAW_W, PetState.CHAR_DRAW_H)
	add_child(cat_view)

func set_pet(new_pet: PetState) -> void:
	pet = new_pet
	_refresh_cat()

func trigger_happy() -> void:
	cat_view.trigger_happy()

func trigger_talk(duration_ms: int = 1200) -> void:
	cat_view.trigger_talk(duration_ms)

func trigger_idle() -> void:
	cat_view.trigger_idle()

func move_pet(dx: int, dy: int) -> void:
	if pet == null:
		return
	pet.move(dx, dy)
	_refresh_cat()

func refresh_from_pet() -> void:
	_refresh_cat()
	queue_redraw()

func set_weather_info(weather: int, temperature: String, humidity: String) -> void:
	weather_type = weather
	temperature_text = temperature
	humidity_text = humidity
	queue_redraw()

func set_status_text(text: String) -> void:
	status_text = text
	queue_redraw()

func set_focus_scene(active: bool, activity: String = "") -> void:
	focus_mode_active = active
	focus_mode_activity = activity
	queue_redraw()

func _refresh_cat() -> void:
	if pet == null or cat_view == null:
		return
	cat_view.set_outing_active(pet.is_outing)
	cat_view.set_sleeping_active(pet.is_sleeping)
	cat_view.set_cat_kind(pet.pet_kind)
	cat_view.facing_left = pet.facing_left
	cat_view.update_stats(pet.fullness, pet.mood, pet.energy, pet.cleanliness)
	var scale_factor: float = _stage_scale()
	var offset := (size - VIRTUAL_SIZE * scale_factor) * 0.5
	cat_view.position = offset + Vector2(pet.pos_x, pet.pos_y) * scale_factor
	cat_view.size = Vector2(PetState.CHAR_DRAW_W, PetState.CHAR_DRAW_H) * scale_factor

func _draw() -> void:
	var scale_factor: float = _stage_scale()
	var draw_size := VIRTUAL_SIZE * scale_factor
	var offset := (size - draw_size) * 0.5

	draw_set_transform(offset, 0.0, Vector2(scale_factor, scale_factor))
	_draw_background()
	_draw_status_line()
	_draw_focus_scene()
	_draw_sleep_z()
	_draw_outing_bubble()
	_draw_clock_bar()
	draw_set_transform(Vector2.ZERO, 0.0, Vector2.ONE)

func _draw_background() -> void:
	var hour: int = Time.get_datetime_dict_from_system()["hour"]
	var sky_color: Color
	var ground_color: Color
	var ground_top: Color

	if hour >= 6 and hour < 17:
		sky_color = SKY_DAY
		ground_color = GROUND_DAY
		ground_top = GROUND_DAY_TOP
	elif hour >= 17 and hour < 19:
		sky_color = SKY_SUNSET
		ground_color = GROUND_DAY
		ground_top = GROUND_DAY_TOP
	else:
		sky_color = SKY_NIGHT
		ground_color = GROUND_NIGHT
		ground_top = GROUND_NIGHT_TOP

	draw_rect(Rect2(Vector2.ZERO, VIRTUAL_SIZE), sky_color, true)
	_apply_weather_tint()

	if hour >= 6 and hour < 17:
		_draw_day_elements()
	elif hour >= 17 and hour < 19:
		draw_circle(Vector2(200, GROUND_Y - 10.0), 12.0, SUN_COLOR)
		draw_circle(Vector2(200, GROUND_Y - 10.0), 10.0, SUNSET_SUN)
	else:
		for star: Vector2 in stars:
			draw_rect(Rect2(star, Vector2(1, 1)), STAR_COLOR, true)
		draw_circle(Vector2(30, 20), 10.0, MOON_COLOR)
		draw_circle(Vector2(34, 17), 9.0, sky_color)

	_draw_weather_effects()
	draw_rect(Rect2(0, GROUND_Y, VIRTUAL_SIZE.x, VIRTUAL_SIZE.y - GROUND_Y), ground_color, true)
	draw_line(Vector2(0, GROUND_Y), Vector2(VIRTUAL_SIZE.x, GROUND_Y), ground_top, 1.0)
	for i: int in 8:
		var gx := float((i * 31 + 10) % int(VIRTUAL_SIZE.x))
		draw_rect(Rect2(Vector2(gx, GROUND_Y + 4), Vector2(1, 1)), ground_top, true)
		draw_rect(Rect2(Vector2(gx + 15, GROUND_Y + 8), Vector2(1, 1)), ground_top, true)

func _draw_day_elements() -> void:
	draw_circle(Vector2(200, 18), 10.0, SUN_COLOR)
	for i: int in 8:
		var angle := i * 0.785
		var x1 := 200 + cos(angle) * 13.0
		var y1 := 18 + sin(angle) * 13.0
		var x2 := 200 + cos(angle) * 16.0
		var y2 := 18 + sin(angle) * 16.0
		draw_line(Vector2(x1, y1), Vector2(x2, y2), SUN_COLOR, 1.0)

	draw_rect(Rect2(40, 12, 24, 8), CLOUD_COLOR, true)
	draw_rect(Rect2(48, 8, 16, 8), CLOUD_COLOR, true)
	draw_rect(Rect2(130, 18, 20, 6), CLOUD_COLOR, true)
	draw_rect(Rect2(136, 14, 14, 6), CLOUD_COLOR, true)

func _apply_weather_tint() -> void:
	match weather_type:
		WEATHER_OVERCAST:
			draw_rect(Rect2(Vector2.ZERO, VIRTUAL_SIZE), Color8(130, 140, 160, 80), true)
		WEATHER_RAIN, WEATHER_THUNDER:
			draw_rect(Rect2(Vector2.ZERO, VIRTUAL_SIZE), Color8(60, 60, 75, 140), true)
		WEATHER_DRIZZLE:
			draw_rect(Rect2(Vector2.ZERO, VIRTUAL_SIZE), Color8(90, 90, 105, 110), true)
		WEATHER_SNOW:
			draw_rect(Rect2(Vector2.ZERO, VIRTUAL_SIZE), Color8(120, 120, 135, 110), true)
		WEATHER_FOG:
			draw_rect(Rect2(Vector2.ZERO, VIRTUAL_SIZE), Color8(140, 140, 145, 120), true)

func _draw_weather_effects() -> void:
	match weather_type:
		WEATHER_DRIZZLE, WEATHER_RAIN, WEATHER_THUNDER:
			var count := 8 if weather_type == WEATHER_DRIZZLE else 15
			var speed := 3 if weather_type == WEATHER_DRIZZLE else 5
			var length := 3 if weather_type == WEATHER_DRIZZLE else 5
			rain_offset = (rain_offset + speed) % 18
			var rain_color := Color8(140, 160, 200)
			for i in count:
				var x := float((i * 17 + rain_offset * 3) % int(VIRTUAL_SIZE.x))
				var y := float((i * 11 + rain_offset * speed) % int(GROUND_Y))
				draw_line(Vector2(x, y), Vector2(x, min(y + length, GROUND_Y)), rain_color, 1.0)
		WEATHER_SNOW:
			snow_offset = (snow_offset + 1) % 32
			var snow_color := Color8(220, 220, 230)
			for i in 15:
				var x := float((i * 19 + snow_offset + (i % 3) * 7) % int(VIRTUAL_SIZE.x))
				var y := float((i * 9 + snow_offset) % int(GROUND_Y))
				draw_rect(Rect2(Vector2(x, y), Vector2(1, 1)), snow_color, true)
				if i % 3 == 0:
					draw_rect(Rect2(Vector2(x + 1, y), Vector2(1, 1)), snow_color, true)
		WEATHER_FOG:
			var fog_color := Color8(160, 160, 165, 110)
			for y in range(20, int(GROUND_Y), 4):
				for x in range(y % 6, int(VIRTUAL_SIZE.x), 6):
					draw_rect(Rect2(Vector2(x, y), Vector2(1, 1)), fog_color, true)

func _draw_status_line() -> void:
	var line := status_text
	if line.is_empty():
		line = pet.status_summary() if pet != null else "\u547c\u565c\u547c\u565c"
	draw_rect(Rect2(4, 4, 168, 14), Color8(12, 16, 24, 120), true)
	draw_string(ThemeDB.fallback_font, Vector2(8, 14), line, HORIZONTAL_ALIGNMENT_LEFT, 156, 12, STATUS_DIM)
	_draw_bond_hearts(180, 4)

func _draw_focus_scene() -> void:
	if pet == null or not focus_mode_active or pet.is_outing:
		return
	var text := "专注学习中"
	var font: Font = ThemeDB.fallback_font
	var text_size := font.get_string_size(text, HORIZONTAL_ALIGNMENT_LEFT, -1, 12)
	var cat_x := float(pet.pos_x + PetState.CHAR_DRAW_W * 0.5)
	var cat_y := float(pet.pos_y - 6.0)
	var rect := Rect2(cat_x - text_size.x * 0.5 - 10.0, cat_y - 18.0, text_size.x + 20.0, 18.0)
	draw_rect(rect, Color8(247, 242, 228), true)
	draw_rect(rect, Color8(174, 152, 126), false)
	draw_string(font, Vector2(rect.position.x + 10.0, rect.position.y + 13.0), text, HORIZONTAL_ALIGNMENT_LEFT, -1, 12, Color8(94, 78, 62))
	var tail := PackedVector2Array([
		Vector2(cat_x - 4.0, rect.position.y + rect.size.y),
		Vector2(cat_x + 4.0, rect.position.y + rect.size.y),
		Vector2(cat_x, rect.position.y + rect.size.y + 5.0),
	])
	draw_colored_polygon(tail, Color8(247, 242, 228))
	draw_polyline(tail + PackedVector2Array([tail[0]]), Color8(174, 152, 126), 1.0)

func _draw_bond_hearts(start_x: int, y: int) -> void:
	if pet == null:
		return
	var heart_count := 0
	if pet.bond >= 80:
		heart_count = 4
	elif pet.bond >= 60:
		heart_count = 3
	elif pet.bond >= 40:
		heart_count = 2
	elif pet.bond >= 20:
		heart_count = 1

	var filled := Color8(255, 105, 150) if pet.mood >= 55 else Color8(190, 120, 150)
	var dim := Color8(80, 70, 90)
	for i in 4:
		var hx := start_x + i * 7
		var color := filled if i < heart_count else dim
		draw_rect(Rect2(Vector2(hx + 1, y + 1), Vector2(1, 1)), color, true)
		draw_rect(Rect2(Vector2(hx + 3, y + 1), Vector2(1, 1)), color, true)
		draw_rect(Rect2(Vector2(hx, y + 2), Vector2(5, 1)), color, true)
		draw_rect(Rect2(Vector2(hx, y + 3), Vector2(5, 1)), color, true)
		draw_rect(Rect2(Vector2(hx + 1, y + 4), Vector2(3, 1)), color, true)
		draw_rect(Rect2(Vector2(hx + 2, y + 5), Vector2(1, 1)), color, true)

func _draw_sleep_z() -> void:
	if pet == null or not pet.is_sleeping:
		return
	var cat_x := float(pet.pos_x + PetState.CHAR_DRAW_W)
	var cat_y := float(pet.pos_y)
	draw_string(ThemeDB.fallback_font, Vector2(cat_x + 4, cat_y + 18), "z", HORIZONTAL_ALIGNMENT_LEFT, -1, 12, STATUS_DIM)
	draw_string(ThemeDB.fallback_font, Vector2(cat_x + 10, cat_y + 8), "Z", HORIZONTAL_ALIGNMENT_LEFT, -1, 14, STATUS_DIM)
	draw_string(ThemeDB.fallback_font, Vector2(cat_x + 16, cat_y - 2), "Z", HORIZONTAL_ALIGNMENT_LEFT, -1, 18, STATUS_DIM)

func _draw_outing_bubble() -> void:
	if pet == null or not pet.is_outing:
		return
	var box := Rect2(82, 48, 76, 18)
	draw_rect(box, Color8(245, 236, 210), true)
	draw_rect(box, Color8(140, 125, 110), false)
	draw_string(ThemeDB.fallback_font, Vector2(box.position.x + 14, box.position.y + 12), "\u5916\u51fa\u4e2d", HORIZONTAL_ALIGNMENT_LEFT, -1, 12, Color.BLACK)

func _draw_clock_bar() -> void:
	var now: Dictionary = Time.get_datetime_dict_from_system()
	var hh := int(now.get("hour", 0))
	var mm := int(now.get("minute", 0))
	var time_str := "%02d:%02d" % [hh, mm]
	var bar := Rect2(74, GROUND_Y + 8, 92, 14)
	draw_rect(bar, Color8(12, 16, 24, 110), true)
	draw_string(ThemeDB.fallback_font, Vector2(bar.position.x + 20, bar.position.y + 11), time_str, HORIZONTAL_ALIGNMENT_LEFT, -1, 12, STATUS_DIM)

func _stage_scale() -> float:
	return min(size.x / VIRTUAL_SIZE.x, size.y / VIRTUAL_SIZE.y)
