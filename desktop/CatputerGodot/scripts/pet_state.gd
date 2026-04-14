extends RefCounted
class_name PetState

const MAX_STAT := 100
const ACTION_COOLDOWN := 6.0
const OUTING_DURATION := 18.0
const MAX_SOUVENIRS := 24
const CHAR_DRAW_W := 48
const CHAR_DRAW_H := 48
const SCREEN_W := 240
const SCREEN_H := 135
const GROUND_Y := SCREEN_H - 28
const MOVE_STEP := 2
const MOVE_MIN_X := 0
const MOVE_MAX_X := SCREEN_W - CHAR_DRAW_W
const MOVE_MIN_Y := 16
const MOVE_MAX_Y := GROUND_Y - CHAR_DRAW_H - 2
const PROMPT_INTERVAL := 28.0
const PROMPT_DURATION := 120.0
const MEMORY_NOTE_LIMIT := 8

const PERSONALITY_CLINGY := "clingy"
const PERSONALITY_CALM := "calm"
const PERSONALITY_LIVELY := "lively"
const PERSONALITY_ALOOF := "aloof"
const CARE_TENDENCY_EAGER := "eager"
const CARE_TENDENCY_BALANCED := "balanced"
const CARE_TENDENCY_GENTLE := "gentle"

const PERSONALITY_ORDER: Array[String] = [
	PERSONALITY_CLINGY,
	PERSONALITY_CALM,
	PERSONALITY_LIVELY,
	PERSONALITY_ALOOF,
]

const PERSONALITY_DIALOGUE_BANK := {
	PERSONALITY_CLINGY: {
		"hello": [
			"\u4f60\u6765\u5566\uff0c\u6211\u521a\u521a\u8fd8\u5728\u60f3\u4f60\u3002",
			"\u4f60\u4e00\u51fa\u73b0\uff0c\u6211\u5c31\u4e0d\u60f3\u770b\u522b\u7684\u5730\u65b9\u4e86\u3002",
		],
		"weather": [
			"\u8981\u662f\u5916\u9762\u8d77\u98ce\uff0c\u4f60\u8981\u79bb\u6211\u8fd1\u4e00\u70b9\u54e6\u3002",
			"\u6211\u60f3\u8ddf\u4f60\u4e00\u8d77\u770b\u4eca\u5929\u7684\u5929\u6c14\u3002",
		],
		"news": [
			"\u4f60\u8bb2\u7ed9\u6211\u542c\u5427\uff0c\u6211\u4f1a\u8ba4\u771f\u8bb0\u4f4f\u7684\u3002",
			"\u53ea\u8981\u662f\u4f60\u8bf4\u7684\u65b0\u9c9c\u4e8b\uff0c\u6211\u90fd\u60f3\u542c\u3002",
		],
		"food": [
			"\u4e00\u63d0\u5230\u5403\u7684\uff0c\u6211\u7684\u8033\u6735\u5c31\u7acb\u8d77\u6765\u4e86\u3002",
			"\u6211\u53ef\u4ee5\u4e00\u8fb9\u5403\uff0c\u4e00\u8fb9\u542c\u4f60\u8bf4\u8bdd\u3002",
		],
		"sleep": [
			"\u4f60\u5728\u65c1\u8fb9\u7684\u8bdd\uff0c\u6211\u7761\u89c9\u4f1a\u66f4\u9999\u3002",
			"\u6211\u6709\u70b9\u60f3\u7f29\u8fdb\u4f60\u7684\u58f0\u97f3\u91cc\u6253\u76f9\u3002",
		],
		"default": [
			"\u6211\u5728\u542c\uff0c\u4f60\u53ef\u4ee5\u518d\u8bf4\u4e00\u53e5\u3002",
			"\u4eca\u5929\u7684\u5fc3\u60c5\uff0c\u60f3\u62ff\u6765\u8ddf\u4f60\u8d34\u4e00\u8d34\u3002",
			"\u6211\u521a\u624d\u8fd8\u5728\u60f3\uff0c\u4f60\u4f1a\u4e0d\u4f1a\u6b63\u597d\u6765\u627e\u6211\u3002",
		],
	},
	PERSONALITY_CALM: {
		"hello": [
			"\u4f60\u6765\u4e86\uff0c\u8fd9\u4e00\u523b\u5c31\u53d8\u5f97\u5f88\u5b89\u9759\u3002",
			"\u4f60\u5750\u4e0b\u6765\u5427\uff0c\u6211\u4eec\u53ef\u4ee5\u6162\u6162\u804a\u3002",
		],
		"weather": [
			"\u4eca\u5929\u7684\u5929\u6c14\u50cf\u4e00\u5c01\u5f88\u6162\u7684\u4fe1\u3002",
			"\u6211\u89c9\u5f97\u7a97\u5916\u7684\u98ce\uff0c\u6b63\u597d\u9002\u5408\u53d1\u4e00\u4f1a\u5446\u3002",
		],
		"news": [
			"\u4f60\u8bf4\u7ed9\u6211\u542c\u5427\uff0c\u6211\u60f3\u628a\u5b83\u653e\u5728\u5fc3\u91cc\u6162\u6162\u60f3\u3002",
			"\u65b0\u9c9c\u4e8b\u4e0d\u4e00\u5b9a\u8981\u5feb\uff0c\u4f60\u6162\u6162\u8bf4\u5c31\u597d\u3002",
		],
		"food": [
			"\u5403\u70b9\u4e1c\u897f\u4e5f\u633a\u597d\uff0c\u80c3\u91cc\u6696\u4e86\uff0c\u5fc3\u4e5f\u4f1a\u8ddf\u7740\u7a33\u4e0b\u6765\u3002",
			"\u6211\u60f3\u628a\u4eca\u5929\u5403\u5230\u7684\u9999\u5473\u6162\u6162\u8bb0\u4f4f\u3002",
		],
		"sleep": [
			"\u6211\u60f3\u5148\u95ed\u4e00\u4e0b\u773c\u776b\uff0c\u8ba9\u65f6\u95f4\u8d70\u5f97\u518d\u8f7b\u4e00\u70b9\u3002",
			"\u8981\u662f\u80fd\u5728\u9633\u5149\u91cc\u6253\u4e2a\u76f9\uff0c\u5c31\u518d\u597d\u4e0d\u8fc7\u4e86\u3002",
		],
		"default": [
			"\u6211\u89c9\u5f97\u4f60\u8fd9\u53e5\u8bdd\uff0c\u5f88\u9002\u5408\u653e\u5728\u4eca\u5929\u3002",
			"\u521a\u624d\u6709\u4e00\u9635\u98ce\u7ecf\u8fc7\uff0c\u6211\u7a81\u7136\u5c31\u60f3\u8ddf\u4f60\u8bf4\u8bdd\u3002",
			"\u6709\u4e9b\u8bdd\u53ea\u8981\u8f7b\u8f7b\u8bf4\u51fa\u6765\uff0c\u5c31\u5f88\u597d\u3002",
		],
	},
	PERSONALITY_LIVELY: {
		"hello": [
			"\u4f60\u6765\u5f97\u6b63\u597d\uff0c\u6211\u521a\u60f3\u627e\u4eba\u804a\u5929\u3002",
			"\u54ce\uff0c\u4f60\u6765\u5566\uff01\u6211\u4eca\u5929\u6709\u597d\u591a\u5c0f\u4e8b\u60f3\u8bf4\u3002",
		],
		"weather": [
			"\u4eca\u5929\u7684\u5929\u6c14\u5f88\u9002\u5408\u8dd1\u4e24\u5708\uff0c\u6211\u5c3e\u5df4\u90fd\u7acb\u8d77\u6765\u4e86\u3002",
			"\u8981\u662f\u5916\u9762\u6709\u98ce\uff0c\u6211\u611f\u89c9\u5f71\u5b50\u90fd\u4f1a\u8ddf\u7740\u8df3\u3002",
		],
		"news": [
			"\u5feb\u8bf4\u5feb\u8bf4\uff01\u6211\u5df2\u7ecf\u51c6\u5907\u597d\u60ca\u8bb6\u4e86\u3002",
			"\u65b0\u9c9c\u4e8b\u6700\u597d\u73b0\u5728\u5c31\u544a\u8bc9\u6211\uff0c\u6211\u80af\u5b9a\u4f1a\u5439\u5f97\u5f88\u9ad8\u5174\u3002",
		],
		"food": [
			"\u8bf4\u5230\u5403\u7684\uff0c\u6211\u89c9\u5f97\u81ea\u5df1\u8fd8\u80fd\u518d\u8dd1\u4e09\u5708\u3002",
			"\u5403\u5b8c\u518d\u73a9\u4e5f\u884c\uff0c\u73a9\u5b8c\u518d\u5403\u4e5f\u884c\uff0c\u53cd\u6b63\u90fd\u5f00\u5fc3\u3002",
		],
		"sleep": [
			"\u6211\u53ef\u80fd\u53ea\u7761\u4e00\u5c0f\u4f1a\u513f\uff0c\u9192\u4e86\u8fd8\u8981\u7ee7\u7eed\u95f9\u817e\u3002",
			"\u5148\u6253\u4e2a\u5c0f\u76f9\uff0c\u7b49\u4f1a\u513f\u6211\u8fd8\u60f3\u518d\u8dd1\u53bb\u7a97\u8fb9\u770b\u770b\u3002",
		],
		"default": [
			"\u6211\u611f\u89c9\u4eca\u5929\u4f1a\u6709\u597d\u4e8b\u53d1\u751f\uff0c\u771f\u7684\u3002",
			"\u6211\u521a\u624d\u8ffd\u7740\u4e00\u70b9\u5149\u8dd1\u4e86\u597d\u51e0\u6b65\uff0c\u73b0\u5728\u8fd8\u60f3\u8ddf\u4f60\u804a\u804a\u3002",
			"\u8981\u662f\u4f60\u613f\u610f\uff0c\u6211\u53ef\u4ee5\u7ee7\u7eed\u628a\u4eca\u5929\u53d8\u5f97\u70ed\u95f9\u4e00\u70b9\u3002",
		],
	},
	PERSONALITY_ALOOF: {
		"hello": [
			"\u4f60\u6765\u5f97\u8fd8\u884c\uff0c\u6211\u6b63\u597d\u6ca1\u5728\u5fd9\u522b\u7684\u4e8b\u3002",
			"\u65e2\u7136\u4f60\u6765\u4e86\uff0c\u90a3\u6211\u4e5f\u4e0d\u662f\u4e0d\u80fd\u8ddf\u4f60\u8bf4\u4e24\u53e5\u3002",
		],
		"weather": [
			"\u5916\u9762\u7684\u5929\u6c14\u4e5f\u5c31\u90a3\u6837\uff0c\u4e0d\u8fc7\u8fd8\u633a\u9002\u5408\u770b\u4e00\u773c\u3002",
			"\u6211\u53ea\u662f\u987a\u4fbf\u6ce8\u610f\u5230\u4eca\u5929\u7684\u98ce\u58f0\u6302\u597d\u542c\u3002",
		],
		"news": [
			"\u4f60\u8bf4\u5427\uff0c\u6211\u542c\u542c\u770b\u6709\u6ca1\u6709\u70b9\u610f\u601d\u3002",
			"\u8981\u662f\u771f\u6709\u65b0\u9c9c\u4e8b\uff0c\u90a3\u5012\u4e5f\u503c\u5f97\u544a\u8bc9\u6211\u3002",
		],
		"food": [
			"\u6211\u53ea\u662f\u987a\u4fbf\u60f3\u5230\u98df\u76c6\u597d\u50cf\u8fd8\u633a\u7a7a\u7684\u3002",
			"\u8981\u5403\u4e5f\u53ef\u4ee5\uff0c\u4f46\u6211\u4e0d\u662f\u5728\u50ac\u4f60\u3002",
		],
		"sleep": [
			"\u6211\u4e0d\u662f\u7d2f\uff0c\u53ea\u662f\u60f3\u5148\u95ed\u4e00\u4e0b\u773c\u776b\u3002",
			"\u8981\u662f\u6211\u7a81\u7136\u4e0d\u8bf4\u8bdd\u4e86\uff0c\u90a3\u591a\u534a\u662f\u5728\u6253\u76f9\u3002",
		],
		"default": [
			"\u6211\u53ea\u662f\u987a\u4fbf\u60f3\u5230\u4f60\u4e86\uff0c\u6240\u4ee5\u8bf4\u4e00\u53e5\u3002",
			"\u8fd9\u53e5\u8bdd\u5012\u662f\u8fd8\u4e0d\u9519\uff0c\u6211\u53ef\u4ee5\u591a\u542c\u4e00\u70b9\u3002",
			"\u522b\u8bef\u4f1a\uff0c\u6211\u53ea\u662f\u6b63\u597d\u613f\u610f\u8ddf\u4f60\u804a\u804a\u3002",
		],
	},
}

const PHOTO_ENTRIES: Array[Dictionary] = [
	{"key": "corner_store", "title": "街角小店", "note": "路口的灯很暖。", "type": "photo"},
	{"key": "night_walk", "title": "夜路散步", "note": "晚风把脚步吹轻了。", "type": "photo"},
	{"key": "quiet_alley", "title": "安静小巷", "note": "小巷里藏着安静。", "type": "photo"},
	{"key": "roof_sun", "title": "屋顶晒太阳", "note": "今天的阳光很松弛。", "type": "photo"},
	{"key": "seaside_trip", "title": "海边旅行", "note": "海风像一封慢信。", "type": "photo"},
	{"key": "window_rain", "title": "窗边听雨", "note": "雨声把时间放慢了。", "type": "photo"},
]

const NOTE_ENTRIES: Array[Dictionary] = [
	{"key": "note_wind", "title": "小纸条", "note": "风从耳边经过，我想起了你。", "type": "note"},
	{"key": "note_cloud", "title": "小纸条", "note": "今天的云，像慢慢折好的毛巾。", "type": "note"},
	{"key": "note_road", "title": "小纸条", "note": "有些路不用赶，走到就很好。", "type": "note"},
	{"key": "note_sunset", "title": "小纸条", "note": "太阳下山的时候，影子也很温柔。", "type": "note"},
	{"key": "note_pocket", "title": "小纸条", "note": "我把一点安静，装进了口袋里。", "type": "note"},
]

const ITEM_ENTRIES: Array[Dictionary] = [
	{"key": "bell", "title": "小铃铛", "note": "叮当一声，今天算平安。", "type": "item"},
	{"key": "feather", "title": "羽毛", "note": "一根轻轻的羽毛。", "type": "item"},
	{"key": "bead", "title": "玻璃珠", "note": "像把一点天光捧回来了。", "type": "item"},
	{"key": "paper_star", "title": "纸星星", "note": "折得不太整齐，但很认真。", "type": "item"},
]

var pet_id: String = ""
var pet_name: String = "小橘"
var pet_kind: String = "orange"
var personality: String = PERSONALITY_LIVELY
var care_tendency: String = CARE_TENDENCY_BALANCED
var assigned_device_id: String = ""
var runtime_location: String = "pc"

var fullness: int = 76
var mood: int = 72
var energy: int = 81
var cleanliness: int = 79
var bond: int = 38
var pos_x: int = (SCREEN_W - CHAR_DRAW_W) / 2
var pos_y: int = MOVE_MAX_Y
var facing_left: bool = false

var is_sleeping: bool = false
var is_outing: bool = false
var action_status: String = "呼噜..."
var last_action_at: Dictionary = {}
var outing_finish_at: float = 0.0
var outing_counter: int = 0
var last_prompt_at: float = 0.0
var active_prompt: Dictionary = {}
var ambient_line: String = ""
var ambient_expires_at: float = 0.0
var memory_tags: Array[String] = []
var memory_notes: Array[String] = []
var device_prompt_memories: Array[Dictionary] = []
var device_prompt_followups: Array[String] = []
var device_memory_events: Array[String] = []
var last_seen_at: int = 0
var last_interaction_at: int = 0
var care_scene_marks: Dictionary = {}
var last_weather_hint_seen: String = ""
var session_reunion_done: bool = false

var souvenirs: Array[Dictionary] = []
var event_history: Array[String] = []
var chat_entries: Array[Dictionary] = []

static func create_named(name: String, kind: String, new_personality: String = "", new_care_tendency: String = "") -> PetState:
	var pet := PetState.new()
	pet.pet_name = name
	pet.pet_kind = kind
	pet.personality = pet._normalize_personality(new_personality if not new_personality.is_empty() else pet._default_personality_for_kind(kind))
	pet.care_tendency = pet._normalize_care_tendency(new_care_tendency)
	pet.ensure_identity()
	pet._push_event("新的猫咪档案已建立")
	return pet

func _init() -> void:
	ensure_identity()
	if event_history.is_empty():
		_push_event("电脑端镜像已启动")

func ensure_identity() -> void:
	if pet_id.is_empty():
		pet_id = "cat_%d_%d" % [Time.get_unix_time_from_system(), randi() % 100000]
	if pet_name.is_empty():
		pet_name = "小猫"
	if pet_kind.is_empty():
		pet_kind = "orange"
	personality = _normalize_personality(personality)
	care_tendency = _normalize_care_tendency(care_tendency)

func tick(delta: float, now: float) -> void:
	if is_outing and now >= outing_finish_at:
		_finish_outing()

	_apply_decay(delta)
	_update_active_prompt(now)
	if ambient_expires_at > 0.0 and now >= ambient_expires_at:
		ambient_line = ""
		ambient_expires_at = 0.0

	if is_sleeping:
		energy = min(MAX_STAT, energy + int(round(delta * 1.5)))
		if energy >= 92:
			is_sleeping = false
			action_status = "睡醒啦。"
			_push_event("%s 睡醒了" % pet_name)

func feed(now: float) -> bool:
	if fullness > 90:
		action_status = "现在还不饿。"
		return false
	if not _can_act("feed", now):
		action_status = "再等一会儿吧。"
		return false
	fullness = min(MAX_STAT, fullness + 22)
	mood = min(MAX_STAT, mood + 4)
	bond = min(MAX_STAT, bond + 2)
	is_sleeping = false
	action_status = "咔嚓咔嚓。"
	_clear_prompt()
	_remember("feed", "上次你喂我吃了点好吃的")
	_push_event("你喂了 %s 一顿" % pet_name)
	return true

func play(now: float) -> bool:
	if energy < 20:
		action_status = "太困了，不想玩。"
		return false
	if mood > 92:
		action_status = "已经很开心了。"
		return false
	if not _can_act("play", now):
		action_status = "先歇一会儿。"
		return false
	mood = min(MAX_STAT, mood + 18)
	energy = max(0, energy - 10)
	fullness = max(0, fullness - 4)
	cleanliness = max(0, cleanliness - 3)
	bond = min(MAX_STAT, bond + 4)
	is_sleeping = false
	action_status = "蹦蹦跳跳。"
	_clear_prompt()
	_remember("play", "上次你陪我玩了好一会儿")
	_push_event("%s 跟你玩了一会儿" % pet_name)
	return true

func nap(now: float) -> bool:
	if energy > 82:
		action_status = "还不太困。"
		return false
	if not _can_act("nap", now):
		action_status = "刚睡醒，再等等。"
		return false
	is_sleeping = true
	energy = min(MAX_STAT, energy + 16)
	bond = min(MAX_STAT, bond + 1)
	action_status = "打个小盹。"
	_clear_prompt()
	_remember("nap", "上次你陪我安静地打了个盹")
	_push_event("%s 缩成一团打盹" % pet_name)
	return true

func clean_up(now: float) -> bool:
	if cleanliness > 90:
		action_status = "已经很干净啦。"
		return false
	if not _can_act("clean", now):
		action_status = "先让毛毛落一落。"
		return false
	cleanliness = min(MAX_STAT, cleanliness + 26)
	mood = min(MAX_STAT, mood + 5)
	bond = min(MAX_STAT, bond + 3)
	action_status = "毛毛顺起来了。"
	_clear_prompt()
	_remember("clean", "上次你把我收拾得香香的")
	_push_event("你给 %s 整理干净了" % pet_name)
	return true

func game(now: float) -> bool:
	if energy < 16:
		action_status = "有点累，等会儿再玩。"
		return false
	if not _can_act("game", now):
		action_status = "玩具还没收回来。"
		return false
	mood = min(MAX_STAT, mood + 14)
	bond = min(MAX_STAT, bond + 5)
	energy = max(0, energy - 8)
	action_status = "抓到玩具啦。"
	_clear_prompt()
	_remember("game", "上次你陪我追着玩具跑了几圈")
	_push_event("%s 追着玩具跑了几圈" % pet_name)
	return true

func start_outing(now: float) -> bool:
	if is_outing:
		action_status = "还在外面散步。"
		return false
	if energy < 18:
		action_status = "今天不想出门。"
		return false
	if not _can_act("outing", now):
		action_status = "等尾巴歇好再出发。"
		return false
	is_outing = true
	outing_finish_at = now + OUTING_DURATION
	energy = max(0, energy - 10)
	fullness = max(0, fullness - 4)
	action_status = "出门逛逛。"
	_clear_prompt()
	_remember("outing", "上次你答应让我出门散了散步")
	_push_event("%s 出门了" % pet_name)
	return true

func move(dx: int, dy: int) -> void:
	if is_outing:
		return
	var speed_mult := 1.0
	if bond >= 80 and mood >= 70:
		speed_mult *= 1.1
	if energy < 20:
		speed_mult *= 0.6
	elif energy < 35:
		speed_mult *= 0.8
	if mood < 20:
		speed_mult *= 0.85
	var step: int = max(1, int(MOVE_STEP * speed_mult))

	pos_x = clamp(pos_x + dx * step, MOVE_MIN_X, MOVE_MAX_X)
	pos_y = clamp(pos_y + dy * step, MOVE_MIN_Y, MOVE_MAX_Y)
	if dx < 0:
		facing_left = true
	elif dx > 0:
		facing_left = false

func chat_reply(text: String) -> String:
	var cleaned := text.strip_edges()
	var lower := cleaned.to_lower()
	if lower.is_empty():
		return _pick_personality_line("default")
	if cleaned.contains("天气"):
		return _pick_personality_line("weather")
	if cleaned.contains("新闻"):
		return _pick_personality_line("news")
	if cleaned.contains("你好") or lower.contains("hi") or lower.contains("hello"):
		return _pick_personality_line("hello")
	if cleaned.contains("晚安"):
		return _pick_personality_line("sleep")
	if cleaned.contains("吃") or cleaned.contains("饭"):
		return _pick_personality_line("food")
	return _pick_personality_line("default")

func status_summary() -> String:
	if has_active_prompt():
		return String(active_prompt.get("body", action_status))
	if is_outing:
		return "外出中"
	if is_sleeping:
		return "打盹中"
	if fullness < 28:
		return "想吃点东西"
	if cleanliness < 35:
		return "想洗洗脸"
	if energy < 25:
		return "眼皮有点沉"
	if mood > 75 and bond > 60:
		return "今天很黏人"
	return action_status

func affection_level() -> int:
	if bond >= 75:
		return 3
	if bond >= 50:
		return 2
	if bond >= 25:
		return 1
	return 0

func affection_label() -> String:
	match affection_level():
		3:
			return "依赖"
		2:
			return "亲近"
		1:
			return "熟悉"
		_:
			return "陌生"

func personality_label() -> String:
	match personality:
		PERSONALITY_CLINGY:
			return "黏人"
		PERSONALITY_CALM:
			return "安静"
		PERSONALITY_ALOOF:
			return "别扭"
		_:
			return "活泼"

func care_tendency_label() -> String:
	match care_tendency:
		CARE_TENDENCY_EAGER:
			return "更主动"
		CARE_TENDENCY_GENTLE:
			return "更安静"
		_:
			return "适中"

func care_tendency_scale() -> float:
	match care_tendency:
		CARE_TENDENCY_EAGER:
			return 0.72
		CARE_TENDENCY_GENTLE:
			return 1.35
		_:
			return 1.0

func care_tendency_bonus() -> int:
	match care_tendency:
		CARE_TENDENCY_EAGER:
			return 8
		CARE_TENDENCY_GENTLE:
			return -4
		_:
			return 0

func has_active_prompt() -> bool:
	return not active_prompt.is_empty()

func add_chat(role: String, text: String) -> void:
	var cleaned := text.strip_edges()
	if cleaned.is_empty():
		return
	chat_entries.append({"role": role, "text": cleaned})
	if chat_entries.size() > 60:
		chat_entries.pop_front()
	if role == "user":
		_remember("chat", "上次你专门停下来陪我说了会儿话")

func replace_chat(entries: Array) -> void:
	chat_entries.clear()
	for item: Variant in entries:
		if item is Dictionary:
			var row := item as Dictionary
			var text := String(row.get("text", "")).strip_edges()
			if text.is_empty():
				continue
			chat_entries.append({
				"role": String(row.get("role", "ai")),
				"text": text,
			})

func to_dict() -> Dictionary:
	ensure_identity()
	var saved_souvenirs: Array[Dictionary] = []
	for item: Dictionary in souvenirs:
		saved_souvenirs.append(item.duplicate(true))
	var events: Array[String] = []
	for line: String in event_history:
		events.append(line)
	var chats: Array[Dictionary] = []
	for row: Dictionary in chat_entries:
		chats.append(row.duplicate(true))

	return {
		"pet_id": pet_id,
		"pet_name": pet_name,
		"pet_kind": pet_kind,
		"personality": personality,
		"care_tendency": care_tendency,
		"assigned_device_id": assigned_device_id,
		"runtime_location": runtime_location,
		"fullness": fullness,
		"mood": mood,
		"energy": energy,
		"cleanliness": cleanliness,
		"bond": bond,
		"pos_x": pos_x,
		"pos_y": pos_y,
		"facing_left": facing_left,
		"is_sleeping": is_sleeping,
		"is_outing": false,
		"action_status": action_status,
		"outing_finish_at": 0.0,
		"outing_counter": outing_counter,
		"last_prompt_at": last_prompt_at,
		"active_prompt": active_prompt.duplicate(true),
		"memory_tags": memory_tags.duplicate(),
		"memory_notes": memory_notes.duplicate(),
		"souvenirs": saved_souvenirs,
		"events": events,
		"chat_entries": chats,
	}

func to_core_dict() -> Dictionary:
	ensure_identity()
	return {
		"pet_id": pet_id,
		"pet_name": pet_name,
		"pet_kind": pet_kind,
		"personality": personality,
		"care_tendency": care_tendency,
		"fullness": fullness,
		"mood": mood,
		"energy": energy,
		"cleanliness": cleanliness,
		"bond": bond,
		"pos_x": pos_x,
		"pos_y": pos_y,
		"facing_left": facing_left,
		"is_sleeping": is_sleeping,
		"is_outing": false,
		"action_status": action_status,
		"outing_finish_at": 0.0,
		"outing_counter": outing_counter,
		"last_prompt_at": last_prompt_at,
		"active_prompt": active_prompt.duplicate(true),
		"souvenirs": souvenirs.duplicate(true),
	}

func to_memory_dict() -> Dictionary:
	return {
		"memory_tags": memory_tags.duplicate(),
		"memory_notes": memory_notes.duplicate(),
		"events": event_history.duplicate(),
		"chat_entries": chat_entries.duplicate(true),
		"device_prompt_memories": device_prompt_memories.duplicate(true),
		"device_memory_events": device_memory_events.duplicate(),
		"last_seen_at": last_seen_at,
		"last_interaction_at": last_interaction_at,
		"care_scene_marks": care_scene_marks.duplicate(true),
		"last_weather_hint_seen": last_weather_hint_seen,
		"care_tendency": care_tendency,
	}

func load_from_dict(data: Dictionary) -> void:
	pet_id = String(data.get("pet_id", pet_id))
	pet_name = String(data.get("pet_name", pet_name))
	pet_kind = String(data.get("pet_kind", pet_kind))
	personality = _normalize_personality(String(data.get("personality", personality)))
	care_tendency = _normalize_care_tendency(String(data.get("care_tendency", care_tendency)))
	assigned_device_id = String(data.get("assigned_device_id", assigned_device_id))
	runtime_location = String(data.get("runtime_location", runtime_location))
	fullness = int(data.get("fullness", fullness))
	mood = int(data.get("mood", mood))
	energy = int(data.get("energy", energy))
	cleanliness = int(data.get("cleanliness", cleanliness))
	bond = int(data.get("bond", bond))
	pos_x = clamp(int(data.get("pos_x", pos_x)), MOVE_MIN_X, MOVE_MAX_X)
	pos_y = clamp(int(data.get("pos_y", pos_y)), MOVE_MIN_Y, MOVE_MAX_Y)
	facing_left = bool(data.get("facing_left", facing_left))
	is_sleeping = bool(data.get("is_sleeping", is_sleeping))
	is_outing = false
	action_status = String(data.get("action_status", action_status))
	outing_finish_at = 0.0
	outing_counter = int(data.get("outing_counter", outing_counter))
	last_prompt_at = float(data.get("last_prompt_at", last_prompt_at))
	var session_now := Time.get_ticks_msec() / 1000.0
	if last_prompt_at > session_now:
		last_prompt_at = 0.0
	ensure_identity()

	active_prompt = {}
	var loaded_prompt: Variant = data.get("active_prompt", {})
	if loaded_prompt is Dictionary:
		active_prompt = (loaded_prompt as Dictionary).duplicate(true)

	memory_tags.clear()
	for tag: Variant in data.get("memory_tags", []):
		memory_tags.append(String(tag))
	memory_notes.clear()
	for note: Variant in data.get("memory_notes", []):
		memory_notes.append(String(note))

	souvenirs.clear()
	for item: Variant in data.get("souvenirs", []):
		if item is Dictionary:
			souvenirs.append((item as Dictionary).duplicate(true))

	event_history.clear()
	for line: Variant in data.get("events", []):
		event_history.append(String(line))

	replace_chat(data.get("chat_entries", []))

func load_core_dict(data: Dictionary) -> void:
	pet_id = String(data.get("pet_id", pet_id))
	pet_name = String(data.get("pet_name", pet_name))
	pet_kind = String(data.get("pet_kind", pet_kind))
	personality = _normalize_personality(String(data.get("personality", personality)))
	care_tendency = _normalize_care_tendency(String(data.get("care_tendency", care_tendency)))
	assigned_device_id = String(data.get("assigned_device_id", assigned_device_id))
	runtime_location = String(data.get("runtime_location", runtime_location))
	fullness = int(data.get("fullness", fullness))
	mood = int(data.get("mood", mood))
	energy = int(data.get("energy", energy))
	cleanliness = int(data.get("cleanliness", cleanliness))
	bond = int(data.get("bond", bond))
	pos_x = clamp(int(data.get("pos_x", pos_x)), MOVE_MIN_X, MOVE_MAX_X)
	pos_y = clamp(int(data.get("pos_y", pos_y)), MOVE_MIN_Y, MOVE_MAX_Y)
	facing_left = bool(data.get("facing_left", facing_left))
	is_sleeping = bool(data.get("is_sleeping", is_sleeping))
	is_outing = false
	action_status = String(data.get("action_status", action_status))
	outing_finish_at = 0.0
	outing_counter = int(data.get("outing_counter", outing_counter))
	last_prompt_at = float(data.get("last_prompt_at", last_prompt_at))
	var session_now := Time.get_ticks_msec() / 1000.0
	if last_prompt_at > session_now:
		last_prompt_at = 0.0
	ensure_identity()

	active_prompt = {}
	var loaded_prompt: Variant = data.get("active_prompt", {})
	if loaded_prompt is Dictionary:
		active_prompt = (loaded_prompt as Dictionary).duplicate(true)

	souvenirs.clear()
	for item: Variant in data.get("souvenirs", []):
		if item is Dictionary:
			souvenirs.append((item as Dictionary).duplicate(true))

func load_memory_dict(data: Dictionary) -> void:
	memory_tags.clear()
	for tag: Variant in data.get("memory_tags", []):
		memory_tags.append(String(tag))
	memory_notes.clear()
	for note: Variant in data.get("memory_notes", []):
		memory_notes.append(String(note))
	event_history.clear()
	for line: Variant in data.get("events", []):
		event_history.append(String(line))
	replace_chat(data.get("chat_entries", []))
	device_prompt_memories.clear()
	for row: Variant in data.get("device_prompt_memories", []):
		if row is Dictionary:
			device_prompt_memories.append((row as Dictionary).duplicate(true))
	device_memory_events.clear()
	for line: Variant in data.get("device_memory_events", []):
		device_memory_events.append(String(line))
	last_seen_at = int(data.get("last_seen_at", last_seen_at))
	last_interaction_at = int(data.get("last_interaction_at", last_interaction_at))
	care_scene_marks = {}
	var loaded_marks: Variant = data.get("care_scene_marks", {})
	if loaded_marks is Dictionary:
		care_scene_marks = (loaded_marks as Dictionary).duplicate(true)
	last_weather_hint_seen = String(data.get("last_weather_hint_seen", last_weather_hint_seen))
	care_tendency = _normalize_care_tendency(String(data.get("care_tendency", care_tendency)))

func to_sync_dict() -> Dictionary:
	ensure_identity()
	var souvenir_entries: Array[Dictionary] = []
	for item: Dictionary in souvenirs:
		souvenir_entries.append(_souvenir_to_sync_entry(item))
	return {
		"pet": {
			"id": pet_id,
			"name": pet_name,
			"kind": pet_kind,
			"personality": personality,
			"assigned_device_id": assigned_device_id,
			"runtime_location": runtime_location,
			"fullness": fullness,
			"mood": mood,
			"energy": energy,
			"cleanliness": cleanliness,
			"bond": bond,
			"x": pos_x,
			"y": pos_y,
			"facing_left": facing_left,
			"sleeping": is_sleeping,
			"status": status_summary(),
		},
		"souvenirs": souvenir_entries,
		"chat": chat_entries.duplicate(true),
		"memory": {
			"prompts": device_prompt_memories.duplicate(true),
			"followups": device_prompt_followups.duplicate(),
			"events": device_memory_events.duplicate(),
		},
	}

func load_from_sync_dict(data: Dictionary) -> void:
	var pet_data: Dictionary = data.get("pet", {})
	pet_id = String(pet_data.get("id", pet_id))
	pet_name = String(pet_data.get("name", pet_name))
	pet_kind = String(pet_data.get("kind", pet_kind))
	personality = _normalize_personality(String(pet_data.get("personality", personality)))
	assigned_device_id = String(pet_data.get("assigned_device_id", assigned_device_id))
	runtime_location = String(pet_data.get("runtime_location", "pc"))
	fullness = int(pet_data.get("fullness", fullness))
	mood = int(pet_data.get("mood", mood))
	energy = int(pet_data.get("energy", energy))
	cleanliness = int(pet_data.get("cleanliness", cleanliness))
	bond = int(pet_data.get("bond", bond))
	pos_x = clamp(int(pet_data.get("x", pos_x)), MOVE_MIN_X, MOVE_MAX_X)
	pos_y = clamp(int(pet_data.get("y", pos_y)), MOVE_MIN_Y, MOVE_MAX_Y)
	facing_left = bool(pet_data.get("facing_left", facing_left))
	is_sleeping = bool(pet_data.get("sleeping", false))
	is_outing = false
	action_status = String(pet_data.get("status", action_status))
	ensure_identity()
	_clear_prompt()

	souvenirs.clear()
	for entry: Variant in data.get("souvenirs", []):
		if entry is Dictionary:
			souvenirs.append(_sync_entry_to_souvenir(entry as Dictionary))
	replace_chat(data.get("chat", []))
	var memory_data: Dictionary = data.get("memory", {})
	device_prompt_memories.clear()
	for row: Variant in memory_data.get("prompts", []):
		if row is Dictionary:
			device_prompt_memories.append((row as Dictionary).duplicate(true))
	device_prompt_followups.clear()
	for row: Variant in memory_data.get("followups", []):
		device_prompt_followups.append(String(row))
	device_memory_events.clear()
	for row: Variant in memory_data.get("events", []):
		device_memory_events.append(String(row))
	last_seen_at = int(Time.get_unix_time_from_system())

func _apply_decay(delta: float) -> void:
	fullness = max(0, fullness - int(round(delta * 0.35)))
	mood = max(0, mood - int(round(delta * 0.18)))
	energy = max(0, energy - int(round(delta * (0.12 if is_sleeping else 0.2))))
	cleanliness = max(0, cleanliness - int(round(delta * 0.16)))
	if fullness < 30:
		mood = max(0, mood - int(round(delta * 0.25)))
	if cleanliness < 35:
		mood = max(0, mood - int(round(delta * 0.18)))
	if fullness < 22 and mood < 35 and energy < 35:
		bond = max(0, bond - int(round(delta * 0.08)))

func _finish_outing() -> void:
	is_outing = false
	outing_counter += 1

	var entry: Dictionary
	var roll := outing_counter % 3
	if roll == 0:
		entry = PHOTO_ENTRIES[randi() % PHOTO_ENTRIES.size()].duplicate()
	elif roll == 1:
		entry = NOTE_ENTRIES[randi() % NOTE_ENTRIES.size()].duplicate()
	else:
		entry = ITEM_ENTRIES[randi() % ITEM_ENTRIES.size()].duplicate()

	souvenirs.push_front(entry)
	if souvenirs.size() > MAX_SOUVENIRS:
		souvenirs.resize(MAX_SOUVENIRS)

	bond = min(MAX_STAT, bond + 3)
	mood = min(MAX_STAT, mood + 6)
	action_status = "带了新纪念回来。"
	_clear_prompt()
	_remember("souvenir")
	_push_event("%s 带回了“%s”" % [pet_name, entry.get("title", "小纪念")])

func _can_act(action: String, now: float) -> bool:
	var last: float = last_action_at.get(action, -1000.0)
	if now - last < ACTION_COOLDOWN:
		return false
	last_action_at[action] = now
	return true

func _push_event(text: String) -> void:
	event_history.push_front(text)
	if event_history.size() > 12:
		event_history.resize(12)

func _default_personality_for_kind(kind: String) -> String:
	match kind:
		"q":
			return PERSONALITY_CALM
		"purple":
			return PERSONALITY_CALM
		_:
			return PERSONALITY_LIVELY

func _normalize_personality(value: String) -> String:
	return value if PERSONALITY_ORDER.has(value) else PERSONALITY_LIVELY

func _normalize_care_tendency(value: String) -> String:
	match value:
		CARE_TENDENCY_EAGER, CARE_TENDENCY_BALANCED, CARE_TENDENCY_GENTLE:
			return value
		_:
			return CARE_TENDENCY_BALANCED

func _remember(tag: String, note: String = "") -> void:
	memory_tags.push_front(tag)
	while memory_tags.size() > 6:
		memory_tags.pop_back()
	var cleaned_note := note.strip_edges()
	if not cleaned_note.is_empty():
		memory_notes.push_front(cleaned_note)
		while memory_notes.size() > MEMORY_NOTE_LIMIT:
			memory_notes.pop_back()

func _clear_prompt() -> void:
	active_prompt.clear()

func clear_active_prompt() -> void:
	_clear_prompt()

func set_device_memories(prompts: Array, followups: Array, events: Array) -> void:
	device_prompt_memories.clear()
	for row: Variant in prompts:
		if row is Dictionary:
			device_prompt_memories.append((row as Dictionary).duplicate(true))
	device_prompt_followups.clear()
	for line: Variant in followups:
		device_prompt_followups.append(String(line))
	device_memory_events.clear()
	for line: Variant in events:
		device_memory_events.append(String(line))
	last_seen_at = int(Time.get_unix_time_from_system())
	last_interaction_at = last_seen_at

func note_player_interaction() -> void:
	var now_unix := int(Time.get_unix_time_from_system())
	last_seen_at = now_unix
	last_interaction_at = now_unix

func _current_day_stamp() -> int:
	var time_info := Time.get_datetime_dict_from_system()
	return int(time_info.get("year", 2000)) * 1000 + int(time_info.get("day_of_year", 1))

func _mark_scene_today(key: String) -> void:
	care_scene_marks[key] = _current_day_stamp()

func _scene_mark_today(key: String) -> bool:
	return int(care_scene_marks.get(key, -1)) == _current_day_stamp()

func _mark_scene_bucket(key: String, bucket: int) -> void:
	care_scene_marks[key] = bucket

func _scene_mark_bucket(key: String, bucket: int) -> bool:
	return int(care_scene_marks.get(key, -1)) == bucket

func memory_preview_lines(limit: int = 6) -> Array[String]:
	var lines: Array[String] = []
	for row: Dictionary in device_prompt_memories:
		var slot_label := String(row.get("label", "提醒"))
		var reply := String(row.get("reply", "")).strip_edges()
		if reply.is_empty():
			continue
		lines.append("%s：%s" % [slot_label, reply])
		if lines.size() >= limit:
			return lines
	for line: String in device_memory_events:
		var cleaned := line.strip_edges()
		if cleaned.is_empty():
			continue
		lines.append(cleaned)
		if lines.size() >= limit:
			return lines
	for line: String in memory_notes:
		var cleaned := line.strip_edges()
		if cleaned.is_empty():
			continue
		lines.append(cleaned)
		if lines.size() >= limit:
			return lines
	return lines

func device_prompt_summary_lines(limit: int = 4) -> Array[String]:
	var lines: Array[String] = []
	for row: Dictionary in device_prompt_memories:
		var slot_label := String(row.get("label", "提醒")).strip_edges()
		var reply := String(row.get("reply", "")).strip_edges()
		if reply.is_empty():
			continue
		lines.append("%s：%s" % [slot_label, reply])
		if lines.size() >= limit:
			return lines
	return lines

func device_followup_lines(limit: int = 3) -> Array[String]:
	var lines: Array[String] = []
	for line: String in device_prompt_followups:
		var cleaned := line.strip_edges()
		if cleaned.is_empty():
			continue
		lines.append(cleaned)
		if lines.size() >= limit:
			return lines
	return lines

func trigger_care_scene(scene_type: String, weather_hint: String = "") -> String:
	var now_unix := int(Time.get_unix_time_from_system())
	var line := ""
	match scene_type:
		"reunion":
			var hours_away := 0.0
			if last_seen_at > 0:
				hours_away = max(0.0, float(now_unix - last_seen_at) / 3600.0)
			if hours_away >= 24.0:
				line = "你回来啦，我把这段时间都给你留着。"
			elif hours_away >= 6.0:
				line = "你回来得刚刚好，我正想把今天讲给你听。"
			elif hours_away >= 1.0:
				line = "又见到你了，屋子一下就有点暖。"
			else:
				line = "你刚走开一会儿，我就又等到你啦。"
			_remember("reunion", "上次重逢时，你回来陪我了")
		"ritual":
			var hour: int = int(Time.get_datetime_dict_from_system().get("hour", 12))
			if hour < 11:
				line = ["早安，今天也慢慢开始吧。", "早上见到你，我就觉得今天有着落了。"][randi() % 2]
				_remember("ritual_morning", "今天早上，你和我打了招呼")
			else:
				line = ["晚一点也没关系，我陪你把今天收一收。", "要是准备休息了，我可以陪你安静一下。"][randi() % 2]
				_remember("ritual_night", "今天睡前，你还记得来看看我")
		"meal":
			var meal_hour: int = int(Time.get_datetime_dict_from_system().get("hour", 12))
			if meal_hour < 11:
				line = "如果你还没吃早饭，我想提醒你先照顾好自己。"
			elif meal_hour < 16:
				line = "到吃点东西的时候了，肚子暖一点，心也会稳一点。"
			else:
				line = "晚一点也别忘了吃饭，我会因此放心很多。"
			_remember("meal_scene", "我提醒过你今天要好好吃饭")
		"weather":
			var weather_text := weather_hint.strip_edges()
			if weather_text.is_empty():
				weather_text = "今天的天气"
			line = "%s有自己的心情，我也想知道你今天过得怎么样。" % weather_text
			_remember("weather_scene", "我把天气轻轻讲给你听")
		"absence":
			line = ["你就算忙一点也没关系，我还会在这里等你。", "今天要是累了，也可以先把心放过来一会儿。"][randi() % 2]
			_remember("absence", "有一次你不在的时候，我还是轻轻惦记着你")
		"anniversary":
			var preview := memory_preview_lines(1)
			if not preview.is_empty():
				line = "我还记得：%s" % preview[0]
			else:
				line = "我把以前的小事情都好好放着，等你哪天想翻一翻。"
			_remember("anniversary", "我把以前的小事重新翻出来给你看")
		_:
			line = "我刚刚想到你了，就想把这句话先留给你。"
	if not line.is_empty():
		say_ambient(line, 12.0)
		_push_event("轻提醒：%s" % line)
		last_seen_at = now_unix
		last_interaction_at = now_unix
	return line

func maybe_auto_care_scene(config: Dictionary = {}, weather_hint: String = "") -> String:
	var now_unix := int(Time.get_unix_time_from_system())
	if not bool(config.get("enabled", true)):
		return ""
	if has_active_prompt() or is_outing:
		return ""
	if not current_ambient_line().is_empty():
		return ""
	var time_info := Time.get_datetime_dict_from_system()
	var hour := int(time_info.get("hour", 12))
	var reunion_seconds := int(config.get("reunion_hours", 1)) * 3600
	var weather_seconds := int(config.get("weather_cooldown_minutes", 3)) * 60
	var absence_seconds := int(config.get("absence_minutes", 30)) * 60
	reunion_seconds = max(300, reunion_seconds)
	weather_seconds = max(60, weather_seconds)
	absence_seconds = max(300, absence_seconds)
	if bool(config.get("reunion", true)) and not session_reunion_done and last_seen_at > 0 and now_unix - last_seen_at >= reunion_seconds:
		session_reunion_done = true
		return trigger_care_scene("reunion", weather_hint)
	if bool(config.get("ritual", true)) and hour >= 7 and hour < 11 and not _scene_mark_today("ritual_morning"):
		_mark_scene_today("ritual_morning")
		return trigger_care_scene("ritual", weather_hint)
	if bool(config.get("ritual", true)) and hour >= 22 and not _scene_mark_today("ritual_night"):
		_mark_scene_today("ritual_night")
		return trigger_care_scene("ritual", weather_hint)
	var cleaned_weather := weather_hint.strip_edges()
	if bool(config.get("weather", true)) and not cleaned_weather.is_empty() and cleaned_weather != last_weather_hint_seen:
		last_weather_hint_seen = cleaned_weather
		if now_unix - last_interaction_at >= weather_seconds:
			return trigger_care_scene("weather", cleaned_weather)
	var absence_bucket := int(now_unix / 21600)
	if bool(config.get("absence", true)) and now_unix - last_interaction_at >= absence_seconds and not _scene_mark_bucket("absence", absence_bucket):
		_mark_scene_bucket("absence", absence_bucket)
		return trigger_care_scene("absence", weather_hint)
	if bool(config.get("anniversary", true)) and hour >= 20 and not _scene_mark_today("anniversary") and memory_preview_lines(1).size() > 0 and randi() % 100 < 12:
		_mark_scene_today("anniversary")
		return trigger_care_scene("anniversary", weather_hint)
	return ""

func say_ambient(text: String, duration: float = 8.0) -> void:
	var cleaned := text.strip_edges()
	if cleaned.is_empty():
		return
	ambient_line = cleaned
	ambient_expires_at = Time.get_ticks_msec() / 1000.0 + duration

func current_ambient_line() -> String:
	if ambient_expires_at > 0.0 and Time.get_ticks_msec() / 1000.0 >= ambient_expires_at:
		ambient_line = ""
		ambient_expires_at = 0.0
	return ambient_line

func _update_active_prompt(now: float) -> void:
	if last_prompt_at > now:
		last_prompt_at = 0.0
	if has_active_prompt():
		if now >= float(active_prompt.get("expires_at", now + 1.0)):
			_clear_prompt()
		else:
			return
	if now - last_prompt_at < PROMPT_INTERVAL:
		return
	last_prompt_at = now
	var generated := _build_prompt(now)
	if generated.is_empty():
		return
	active_prompt = generated
	var title := String(active_prompt.get("title", "")).strip_edges()
	if not title.is_empty():
		_push_event(title)

func _build_prompt(now: float) -> Dictionary:
	if is_outing:
		return {}
	var prompt_type := _choose_prompt_type()
	if prompt_type.is_empty():
		return {}
	return {
		"type": prompt_type,
		"title": _prompt_title_for_v2(prompt_type),
		"body": _line_for_type_v2(prompt_type),
		"expires_at": now + PROMPT_DURATION + randf_range(-12.0, 12.0),
	}

func _line_for_type(prompt_type: String) -> String:
	match personality:
		PERSONALITY_CLINGY:
			match prompt_type:
				"care":
					return "你能不能再看看我？"
				"rest":
					return "陪我安静地眯一会儿吧。"
				"outing":
					return "你要不要跟我一起出门？"
				"share":
					return "我想离你近一点。"
				_:
					return "我想先听你讲一句话。"
		PERSONALITY_CALM:
			match prompt_type:
				"care":
					return "收拾好了，心里也会更舒服一点。"
				"rest":
					return "我想把现在过得慢一点。"
				"outing":
					return "今天的风很轻，适合出去走走。"
				"share":
					return "我想把刚才的安静分给你。"
				_:
					return "我想和你慢慢聊一句。"
		PERSONALITY_ALOOF:
			match prompt_type:
				"care":
					return "只是顺便提醒你一下，我有点狼狈了。"
				"rest":
					return "我不是累，只是想闭一下眼。"
				"outing":
					return "要是出门，我也不介意。"
				"share":
					return "我只是碰巧想坐在这里。"
				_:
					return "你要说话的话，我也可以听听。"
		_:
			match prompt_type:
				"care":
					return "快看看我，我想赶紧满血复活。"
				"rest":
					return "先躺一下，等会儿再继续冒险。"
				"outing":
					return "我想出门跑两圈，说不定会有新发现。"
				"share":
					return "我刚才想到你了，就来找你。"
				_:
					return "我有一句话想立刻跟你说。"

func _choose_prompt_type() -> String:
	if fullness < 28 or cleanliness < 35:
		return "care"
	if energy < 24:
		return "rest"

	var weighted: Array[String] = []
	_add_prompt_weight(weighted, "chat", 1)
	_add_prompt_weight(weighted, "share", 1)
	if bond >= 45:
		_add_prompt_weight(weighted, "chat", 1)
	if bond >= 60:
		_add_prompt_weight(weighted, "share", 1)
	if bond >= 35:
		_add_prompt_weight(weighted, "question", 1)
	if mood >= 62 and energy >= 28:
		_add_prompt_weight(weighted, "outing", 1)

	match personality:
		PERSONALITY_CLINGY:
			_add_prompt_weight(weighted, "chat", 2)
			_add_prompt_weight(weighted, "share", 2)
			_add_prompt_weight(weighted, "question", 2)
		PERSONALITY_CALM:
			_add_prompt_weight(weighted, "share", 2)
			_add_prompt_weight(weighted, "rest", 1)
			_add_prompt_weight(weighted, "question", 1)
		PERSONALITY_ALOOF:
			_add_prompt_weight(weighted, "share", 1)
			if bond >= 55:
				_add_prompt_weight(weighted, "chat", 1)
				_add_prompt_weight(weighted, "question", 1)
		_:
			_add_prompt_weight(weighted, "chat", 2)
			_add_prompt_weight(weighted, "question", 1)
			if mood >= 68:
				_add_prompt_weight(weighted, "outing", 2)
	_add_prompt_weight(weighted, "chat", 1)

	if weighted.is_empty():
		return ""
	var blank_chance: int = 8
	if randi() % 100 < blank_chance:
		return ""
	return weighted[randi() % weighted.size()]

func _add_prompt_weight(weighted: Array[String], prompt_type: String, count: int) -> void:
	for _i in count:
		weighted.append(prompt_type)

func _prompt_title_for(prompt_type: String) -> String:
	match personality:
		PERSONALITY_CLINGY:
			match prompt_type:
				"care":
					return "想让你看看我"
				"rest":
					return "想靠着你打盹"
				"outing":
					return "想一起出门"
				"share":
					return "想黏在你身边"
				_:
					return "想和你说话"
		PERSONALITY_CALM:
			match prompt_type:
				"care":
					return "想慢慢整理一下"
				"rest":
					return "想安静眯一会儿"
				"outing":
					return "想出去走一圈"
				"share":
					return "想把心情分给你"
				_:
					return "想和你聊一句"
		PERSONALITY_ALOOF:
			match prompt_type:
				"care":
					return "现在有点不太体面"
				"rest":
					return "想先闭一下眼"
				"outing":
					return "倒也可以出门"
				"share":
					return "只是想待在这里"
				_:
					return "有句话想顺便说"
		_:
			match prompt_type:
				"care":
					return "快来看看我"
				"rest":
					return "想先躺一小会儿"
				"outing":
					return "想出去逛逛"
				"share":
					return "想把开心分给你"
				_:
					return "有句话想现在说"

func _pick_personality_line(category: String) -> String:
	var bank: Dictionary = PERSONALITY_DIALOGUE_BANK.get(personality, PERSONALITY_DIALOGUE_BANK[PERSONALITY_LIVELY])
	var options: Array = bank.get(category, [])
	if options.is_empty():
		options = bank.get("default", [])
	if options.is_empty():
		return "喵。"
	return String(options[randi() % options.size()])

func _latest_memory_note() -> String:
	if memory_notes.is_empty():
		return ""
	return String(memory_notes[0]).strip_edges()

func _line_for_type_v2(prompt_type: String) -> String:
	var memory_hint := _latest_memory_note()
	match personality:
		PERSONALITY_CLINGY:
			match prompt_type:
				"care":
					return "你能不能再看看我？"
				"rest":
					return "陪我安静地眯一会儿，好不好？"
				"outing":
					return "你要不要陪我一起出门？"
				"share":
					return "我刚想到你了，就想黏过来。"
				"question":
					return memory_hint if not memory_hint.is_empty() else "我想问你一句小小的话，你会回我吗？"
				_:
					return "我想先听你讲一句话。"
		PERSONALITY_CALM:
			match prompt_type:
				"care":
					return "收拾一下的话，心里也会更舒服一点。"
				"rest":
					return "我想把现在过得再慢一点。"
				"outing":
					return "今天的风很轻，适合出去走走。"
				"share":
					return "我想把刚才的安静分给你一点。"
				"question":
					return memory_hint if not memory_hint.is_empty() else "我有一句轻轻的话，想问问你。"
				_:
					return "我想和你慢慢聊一句。"
		PERSONALITY_ALOOF:
			match prompt_type:
				"care":
					return "只是顺便提醒你一下，我现在有点乱。"
				"rest":
					return "我不是累，只是想先闭一会儿眼。"
				"outing":
					return "要是出门，我也不是不可以。"
				"share":
					return "我只是刚好想在你旁边待一会儿。"
				"question":
					return memory_hint if not memory_hint.is_empty() else "我只是顺口问一句，你也可以回答。"
				_:
					return "你要说话的话，我也可以听。"
		_:
			match prompt_type:
				"care":
					return "快看看我，我现在想立刻精神起来。"
				"rest":
					return "先歇一小会儿，等会儿再继续热闹。"
				"outing":
					return "我想出去转两圈，说不定会有新发现。"
				"share":
					return "我刚刚想到你了，就想来找你。"
				"question":
					return memory_hint if not memory_hint.is_empty() else "我想现在就问你一句，你会接住吗？"
				_:
					return "我有一句话，想现在就告诉你。"

func _prompt_title_for_v2(prompt_type: String) -> String:
	match personality:
		PERSONALITY_CLINGY:
			match prompt_type:
				"care":
					return "想让你看看我"
				"rest":
					return "想靠着你打个盹"
				"outing":
					return "想一起出门"
				"share":
					return "想黏在你身边"
				"question":
					return "想问你一小句"
				_:
					return "想和你说话"
		PERSONALITY_CALM:
			match prompt_type:
				"care":
					return "想慢慢整理一下"
				"rest":
					return "想安静眯一会儿"
				"outing":
					return "想出去走一圈"
				"share":
					return "想把心情分给你"
				"question":
					return "想轻轻问你一句"
				_:
					return "想和你聊一句"
		PERSONALITY_ALOOF:
			match prompt_type:
				"care":
					return "现在有点不太体面"
				"rest":
					return "想先闭一会儿眼"
				"outing":
					return "倒也可以出门"
				"share":
					return "只是想待在这里"
				"question":
					return "想顺便问问你"
				_:
					return "有句话想顺便说"
		_:
			match prompt_type:
				"care":
					return "快来看看我"
				"rest":
					return "想先歇一小会儿"
				"outing":
					return "想出去逛逛"
				"share":
					return "想把开心分给你"
				"question":
					return "想现在问你一句"
				_:
					return "有句话想现在说"

func prompt_response_line(prompt_type: String) -> String:
	match personality:
		PERSONALITY_CLINGY:
			match prompt_type:
				"question":
					return "你愿意回我这一句，我就会记很久。"
				"share":
					return "你肯听我说，我就已经很开心了。"
				"chat":
					return "再陪我多说一小会儿吧。"
		PERSONALITY_CALM:
			match prompt_type:
				"question":
					return "嗯，这样慢慢回答我就很好。"
				"share":
					return "谢谢你把这一刻接住了。"
				"chat":
					return "有你在，这句就不算落空。"
		PERSONALITY_ALOOF:
			match prompt_type:
				"question":
					return "我只是随口一问，不过你回我了也不错。"
				"share":
					return "行吧，这句就算是被你好好听到了。"
				"chat":
					return "看来你还挺愿意听我说。"
		_:
			match prompt_type:
				"question":
					return "好耶，你真的接住这句了。"
				"share":
					return "嘿，我就知道你会理我。"
				"chat":
					return "这样一来，今天又热闹一点了。"
	return status_summary()

func remember_prompt_response(prompt_type: String, prompt_title: String = "") -> void:
	var note := ""
	match prompt_type:
		"question":
			note = "上次你认真接住了我问的小问题"
		"share":
			note = "上次你停下来听我分享了一件小事"
		"chat":
			note = "上次你顺着我的话继续陪我聊了下去"
		"rest":
			note = "上次你愿意先陪我安静一会儿"
		"care":
			note = "上次你立刻过来照顾我了"
		"outing":
			note = "上次你答应了我想出门的小心思"
		_:
			note = "上次你回应了我的小心思"
	_remember("prompt_%s" % prompt_type, note)
	if not prompt_title.is_empty():
		_push_event("你回应了：%s" % prompt_title)

func house_event_line(category: String, fallback: String) -> String:
	match personality:
		PERSONALITY_CLINGY:
			match category:
				"window":
					return "趴在窗边看了好久，还想拉你一起看。"
				"bed":
					return "在小床边缩了一会儿，像在等你。"
				"bowls":
					return "在食盆附近闻了闻，然后又想你了。"
				"rug":
					return "在地毯上翻了一圈，心里暖乎乎的。"
				"shelf":
					return "盯着架子发呆的时候，突然又想到你。"
				"social":
					return "和 %s 蹭在一起，忽然觉得今天很满。" % fallback
		PERSONALITY_CALM:
			match category:
				"window":
					return "趴在窗边看了一会儿，光线很轻。"
				"bed":
					return "在小床边眯了一阵，时间也跟着慢了下来。"
				"bowls":
					return "在食盆附近停了停，胃里也跟着安心了一点。"
				"rug":
					return "地毯软软的，躺在上面很像被日子轻轻托住了。"
				"shelf":
					return "朝架子看了一阵，像在想一件很慢的事。"
				"social":
					return "和 %s 安安静静地靠了一会儿。" % fallback
		PERSONALITY_ALOOF:
			match category:
				"window":
					return "我只是在窗边看了看，没别的意思。"
				"bed":
					return "我不是想睡，只是在小床边闭了下眼。"
				"bowls":
					return "我只是路过食盆的时候闻了一下。"
				"rug":
					return "地毯还行，我才不是因为喜欢才待在那里。"
				"shelf":
					return "架子上的东西也就那样，但还挺值得看几眼。"
				"social":
					return "和 %s 靠在一起只是碰巧，但也不算坏。" % fallback
		_:
			match category:
				"window":
					return "趴在窗边看了好一会儿，感觉今天很亮。"
				"bed":
					return "爬到小床边蹲了蹲，元气又慢慢回来了。"
				"bowls":
					return "绕着食盆转了一圈，鼻子都精神了。"
				"rug":
					return "在地毯上打滚的时候，尾巴差点飞起来。"
				"shelf":
					return "盯着架子看了半天，感觉像发现了新冒险。"
				"social":
					return "和 %s 蹭了蹭，心情也跟着跳了一下。" % fallback
	return fallback

func house_prompt_line(category: String, fallback: String) -> String:
	var line := house_event_line(category, fallback)
	return line if not line.is_empty() else fallback

func ambient_line_for(category: String, fallback: String = "") -> String:
	match personality:
		PERSONALITY_CLINGY:
			match category:
				"click":
					return [
						"你点到我啦，我本来就在等你。",
						"你一碰我，我就想靠近一点。",
						"我就知道你会先来找我。",
					][randi() % 3]
				"greet":
					return [
						"你来了就好，我刚刚还在找你。",
						"先陪我一下，好不好？",
						"你在这儿，我就安心了。",
					][randi() % 3]
				"idle":
					return [
						"我想先靠近你一点，再想别的。",
						"只要能待在你附近就很好。",
					][randi() % 2]
		PERSONALITY_CALM:
			match category:
				"click":
					return [
						"嗯，我在这儿，慢慢说就好。",
						"别急，我有在听。",
						"这样轻轻叫我就很好。",
					][randi() % 3]
				"greet":
					return [
						"今天也可以安静地待一会儿。",
						"现在的光线很好，适合慢慢说话。",
						"先一起安静一下，也很好。",
					][randi() % 3]
				"idle":
					return [
						"窗边的光刚刚好，我也刚刚好。",
						"今天适合慢一点。",
					][randi() % 2]
		PERSONALITY_ALOOF:
			match category:
				"click":
					return [
						"我知道你在看我，不用点这么准。",
						"你还真会挑时候碰我。",
						"哼，我也不是不能理你。",
					][randi() % 3]
				"greet":
					return [
						"既然你来了，我也不是不能理你。",
						"我只是刚好有空，才不是在等你。",
						"你要说话的话，我就勉强听一下。",
					][randi() % 3]
				"idle":
					return [
						"我只是随便待在这里，没有别的意思。",
						"这里只是刚好还行。",
					][randi() % 2]
		_:
			match category:
				"click":
					return [
						"哎，你点到我啦，我正想动一动。",
						"来得正好，我刚好有精神。",
						"要不要一起玩点什么？",
					][randi() % 3]
				"greet":
					return [
						"来得正好，我今天还挺想热闹一点。",
						"快来快来，我刚好想说句话。",
						"今天有点开心，想分你一点。",
					][randi() % 3]
				"idle":
					return [
						"我感觉今天会有一点点好事发生。",
						"尾巴有点痒，像是想去冒险。",
					][randi() % 2]
	return fallback if not fallback.is_empty() else _pick_personality_line("default")

func _souvenir_to_sync_entry(item: Dictionary) -> Dictionary:
	var kind := String(item.get("type", "item"))
	if kind == "photo":
		return {
			"item": "photo:%s" % String(item.get("key", "")),
			"note": String(item.get("note", "")),
		}
	return {
		"item": String(item.get("title", "")),
		"note": String(item.get("note", "")),
	}

func _sync_entry_to_souvenir(entry: Dictionary) -> Dictionary:
	var item := String(entry.get("item", ""))
	var note := String(entry.get("note", ""))
	if item.begins_with("photo:"):
		var key := item.trim_prefix("photo:")
		return {
			"type": "photo",
			"key": key,
			"title": _photo_title_for_key(key),
			"note": note,
		}
	if item == "小纸条":
		return {"type": "note", "key": "note", "title": item, "note": note}
	return {"type": "item", "key": item, "title": item, "note": note}

func _photo_title_for_key(key: String) -> String:
	match key:
		"corner_store":
			return "街角小店"
		"night_walk":
			return "夜路散步"
		"quiet_alley":
			return "安静小巷"
		"roof_sun":
			return "屋顶晒太阳"
		"seaside_trip":
			return "海边旅行"
		"window_rain":
			return "窗边听雨"
		_:
			return "外出照片"
