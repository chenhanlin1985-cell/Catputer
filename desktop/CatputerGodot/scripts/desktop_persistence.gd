extends RefCounted
class_name DesktopPersistence

const SAVE_PATH := "user://catputer_desktop_save.json"
const MEMORY_DIR := "user://memories"

static func save_data(data: Dictionary) -> bool:
	var file: FileAccess = FileAccess.open(SAVE_PATH, FileAccess.WRITE)
	if file == null:
		return false
	file.store_string(JSON.stringify(data, "\t"))
	file.close()
	return true

static func load_data() -> Dictionary:
	if not FileAccess.file_exists(SAVE_PATH):
		return {}
	var file: FileAccess = FileAccess.open(SAVE_PATH, FileAccess.READ)
	if file == null:
		return {}
	var text: String = file.get_as_text()
	file.close()
	var parsed: Variant = JSON.parse_string(text)
	if parsed is Dictionary:
		return parsed
	return {}

static func _ensure_memory_dir() -> void:
	DirAccess.make_dir_recursive_absolute(MEMORY_DIR)

static func pet_memory_path(pet_id: String) -> String:
	return "%s/%s.json" % [MEMORY_DIR, pet_id]

static func save_pet_memory(pet_id: String, data: Dictionary) -> bool:
	if pet_id.strip_edges().is_empty():
		return false
	_ensure_memory_dir()
	var file: FileAccess = FileAccess.open(pet_memory_path(pet_id), FileAccess.WRITE)
	if file == null:
		return false
	file.store_string(JSON.stringify(data, "\t"))
	file.close()
	return true

static func load_pet_memory(pet_id: String) -> Dictionary:
	if pet_id.strip_edges().is_empty():
		return {}
	var path := pet_memory_path(pet_id)
	if not FileAccess.file_exists(path):
		return {}
	var file: FileAccess = FileAccess.open(path, FileAccess.READ)
	if file == null:
		return {}
	var text := file.get_as_text()
	file.close()
	var parsed: Variant = JSON.parse_string(text)
	if parsed is Dictionary:
		return parsed
	return {}

static func ensure_pet_memory_file(pet_id: String, data: Dictionary = {}) -> void:
	if pet_id.strip_edges().is_empty():
		return
	var path := pet_memory_path(pet_id)
	if FileAccess.file_exists(path):
		return
	save_pet_memory(pet_id, data)

static func delete_pet_memory(pet_id: String) -> void:
	if pet_id.strip_edges().is_empty():
		return
	var path := pet_memory_path(pet_id)
	if FileAccess.file_exists(path):
		DirAccess.remove_absolute(path)

static func rename_pet_memory(old_pet_id: String, new_pet_id: String, fallback_data: Dictionary = {}) -> void:
	if old_pet_id.strip_edges().is_empty() or new_pet_id.strip_edges().is_empty() or old_pet_id == new_pet_id:
		return
	_ensure_memory_dir()
	var old_path := pet_memory_path(old_pet_id)
	var new_path := pet_memory_path(new_pet_id)
	if FileAccess.file_exists(old_path):
		if FileAccess.file_exists(new_path):
			if not fallback_data.is_empty():
				save_pet_memory(new_pet_id, fallback_data)
			DirAccess.remove_absolute(old_path)
			return
		DirAccess.rename_absolute(old_path, new_path)
		return
	if not fallback_data.is_empty() and not FileAccess.file_exists(new_path):
		save_pet_memory(new_pet_id, fallback_data)
