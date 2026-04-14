extends RefCounted
class_name HandheldSpritesQ

const ASSET_ROOT := "res://assets/pets/q"
const SCALE := 1

static var _cache: Dictionary = {}

static func get_frames(state: String) -> Array[Texture2D]:
	_ensure_cache()
	return _cache.get(state, [])

static func _ensure_cache() -> void:
	if not _cache.is_empty():
		return
	_cache["idle"] = _load_group("idle", ["idle_1.png", "idle_2.png", "idle_3.png", "idle_4.png"])
	_cache["happy"] = _load_group("happy", ["happy_1.png", "happy_2.png"])
	_cache["sleep"] = _load_group("sleep", ["sleep_1.png"])
	_cache["talk"] = _load_group("talk", ["talk_1.png", "talk_1.png"])

static func _load_group(group: String, files: Array[String]) -> Array[Texture2D]:
	var out: Array[Texture2D] = []
	for file_name in files:
		var tex := _load_one("%s/%s/%s" % [ASSET_ROOT, group, file_name])
		if tex != null:
			out.append(tex)
	return out

static func _load_one(path: String) -> Texture2D:
	if not ResourceLoader.exists(path):
		return null
	var img := Image.new()
	var err := img.load(path)
	if err != OK:
		return null
	if SCALE > 1:
		img.resize(img.get_width() * SCALE, img.get_height() * SCALE, Image.INTERPOLATE_NEAREST)
	return ImageTexture.create_from_image(img)
