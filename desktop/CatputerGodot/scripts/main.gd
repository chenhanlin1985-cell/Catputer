extends Control

const PetState = preload("res://scripts/pet_state.gd")
const DesktopPersistence = preload("res://scripts/desktop_persistence.gd")
const DeviceBridge = preload("res://scripts/device_bridge.gd")
const CompanionStage = preload("res://scripts/companion_stage.gd")
const CatHouseView = preload("res://scripts/cat_house_view.gd")

const CAT_VARIANTS: Array[Dictionary] = [
	{"kind": "orange", "label": "\u6a58\u732b"},
	{"kind": "purple", "label": "\u7d2b\u732b"},
]

const PERSONALITY_VARIANTS: Array[Dictionary] = [
	{"key": PetState.PERSONALITY_CLINGY, "label": "\u9ecf\u4eba"},
	{"key": PetState.PERSONALITY_CALM, "label": "\u5b89\u9759"},
	{"key": PetState.PERSONALITY_LIVELY, "label": "\u6d3b\u6cfc"},
	{"key": PetState.PERSONALITY_ALOOF, "label": "\u522b\u626d"},
]

const DEFAULT_CARE_SETTINGS := {
	"enabled": true,
	"reunion": true,
	"ritual": true,
	"weather": true,
	"absence": true,
	"anniversary": true,
	"reunion_hours": 1,
	"weather_cooldown_minutes": 3,
	"absence_minutes": 30,
}

var cats: Array[PetState] = []
var active_pet_id: String = ""
var current_device_pet_id: String = ""
var selected_return_pet_id: String = ""
var saved_sync_host: String = "192.168.212.104"
var town_sync_active: bool = false
var latest_weather_label: String = ""
var care_settings: Dictionary = DEFAULT_CARE_SETTINGS.duplicate(true)

var bridge: DeviceBridge
var stage_view: CompanionStage
var colony_view: CatHouseView
var decay_timer: Timer
var sync_heartbeat_timer: Timer
var top_panel: PanelContainer
var tabs_view: TabContainer

var stat_bars: Dictionary = {}
var stat_labels: Dictionary = {}
var event_log: RichTextLabel
var souvenir_list: ItemList
var souvenir_title: Label
var souvenir_note: RichTextLabel
var souvenir_image: TextureRect
var chat_history: RichTextLabel
var chat_input: LineEdit
var sync_status_label: Label
var sync_state_label: Label
var sync_log: RichTextLabel
var sync_host_input: LineEdit
var sync_push_input: LineEdit
var sync_toggle_button: Button
var sync_enter_button: Button
var sync_leave_button: Button
var colony_selected_label: Label
var colony_prompt_button: Button
var warehouse_list: ItemList
var warehouse_hint_label: Label
var warehouse_name_label: Label
var warehouse_meta_label: RichTextLabel
var warehouse_return_button: Button
var warehouse_stat_bars: Dictionary = {}
var warehouse_stat_labels: Dictionary = {}
var warehouse_thought_title_label: Label
var warehouse_thought_label: RichTextLabel
var warehouse_memory_title_label: Label
var warehouse_memory_label: RichTextLabel
var warehouse_name_input: LineEdit
var warehouse_kind_option: OptionButton
var warehouse_personality_option: OptionButton
var warehouse_prompt_button: Button
var warehouse_scene_buttons: Dictionary = {}
var care_enabled_checkbox: CheckBox
var care_scene_checkboxes: Dictionary = {}
var care_reunion_spin: SpinBox
var care_weather_spin: SpinBox
var care_absence_spin: SpinBox
var focus_toggle_button: Button
var focus_status_label: Label

const FOCUS_MODE_DURATION := 25 * 60.0
const FOCUS_WARN_COOLDOWN := 8.0
const FOCUS_ACTIVITIES: Array[String] = ["读书", "踩缝纫机"]

var focus_mode_active: bool = false
var focus_mode_started_at: float = 0.0
var focus_mode_ends_at: float = 0.0
var focus_mode_last_warn_at: float = -1000.0
var focus_mode_pet_id: String = ""
var focus_mode_activity: String = ""

const TAB_ACTIONS := 0
const TAB_CHAT := 1
const TAB_SOUVENIRS := 2
const TAB_CAT_HOUSE := 3
const TAB_WAREHOUSE := 4
const TAB_SYNC := 5

func _ready() -> void:
	randomize()
	_load_state()
	_ensure_default_roster()
	_build_ui()
	_refresh_all()
	bridge = DeviceBridge.new()
	add_child(bridge)
	bridge.listen_state_changed.connect(_on_bridge_state_changed)
	bridge.raw_packet_received.connect(_on_bridge_raw_packet)
	bridge.state_packet_received.connect(_on_bridge_state_packet)
	bridge.chat_packet_received.connect(_on_bridge_chat_packet)
	bridge.start_listening()
	decay_timer = Timer.new()
	decay_timer.wait_time = 1.0
	decay_timer.autostart = true
	decay_timer.timeout.connect(_on_tick)
	add_child(decay_timer)
	sync_heartbeat_timer = Timer.new()
	sync_heartbeat_timer.wait_time = 5.0
	sync_heartbeat_timer.timeout.connect(_on_sync_heartbeat)
	add_child(sync_heartbeat_timer)

func _process(_delta: float) -> void:
	if bridge != null:
		bridge.poll()

func _notification(what: int) -> void:
	if what == NOTIFICATION_WM_CLOSE_REQUEST or what == NOTIFICATION_EXIT_TREE:
		_save_state()

func _unhandled_input(event: InputEvent) -> void:
	if event is InputEventKey and event.is_pressed() and not event.echo:
		var key_event := event as InputEventKey
		if key_event.ctrl_pressed and key_event.shift_pressed and key_event.keycode == KEY_F:
			_toggle_focus_mode()
			get_viewport().set_input_as_handled()
			return
		if focus_mode_active and _is_focus_interrupting_key(key_event):
			_handle_focus_interrupt()
			get_viewport().set_input_as_handled()
			return
	if top_panel != null and not top_panel.visible:
		return
	if chat_input != null and chat_input.has_focus():
		return
	var pet := _current_pet()
	if pet == null:
		return
	if event is InputEventKey and event.is_pressed() and not event.echo:
		if event.keycode == KEY_LEFT or event.keycode == KEY_A:
			stage_view.move_pet(-1, 0)
		elif event.keycode == KEY_RIGHT or event.keycode == KEY_D:
			stage_view.move_pet(1, 0)
		elif event.keycode == KEY_UP or event.keycode == KEY_W:
			stage_view.move_pet(0, -1)
		elif event.keycode == KEY_DOWN or event.keycode == KEY_S:
			stage_view.move_pet(0, 1)
		else:
			return
		_refresh_all()

func _build_ui() -> void:
	var root := VBoxContainer.new()
	root.set_anchors_and_offsets_preset(Control.PRESET_FULL_RECT)
	root.add_theme_constant_override("separation", 14)
	root.offset_left = 18
	root.offset_top = 18
	root.offset_right = -18
	root.offset_bottom = -18
	add_child(root)

	top_panel = PanelContainer.new()
	top_panel.size_flags_vertical = Control.SIZE_EXPAND_FILL
	top_panel.custom_minimum_size = Vector2(0, 360)
	root.add_child(top_panel)

	var top_margin := MarginContainer.new()
	for side in ["left", "top", "right", "bottom"]:
		top_margin.add_theme_constant_override("margin_%s" % side, 12)
	top_panel.add_child(top_margin)

	stage_view = CompanionStage.new()
	stage_view.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	stage_view.size_flags_vertical = Control.SIZE_EXPAND_FILL
	top_margin.add_child(stage_view)

	tabs_view = TabContainer.new()
	tabs_view.size_flags_vertical = Control.SIZE_EXPAND_FILL
	tabs_view.custom_minimum_size = Vector2(0, 420)
	tabs_view.focus_mode = Control.FOCUS_NONE
	tabs_view.tab_changed.connect(_on_tab_changed)
	root.add_child(tabs_view)

	_build_actions_tab(tabs_view)
	_build_chat_tab(tabs_view)
	_build_souvenir_tab(tabs_view)
	_build_colony_tab(tabs_view)
	_build_warehouse_tab(tabs_view)
	_build_sync_tab(tabs_view)
	sync_host_input.text = saved_sync_host
	_update_stage_visibility()

func _build_actions_tab(parent: TabContainer) -> void:
	var tab := VBoxContainer.new()
	tab.name = "\u4e92\u52a8"
	tab.add_theme_constant_override("separation", 10)
	parent.add_child(tab)
	var panel := PanelContainer.new()
	tab.add_child(panel)
	var margin := MarginContainer.new()
	for side in ["left", "top", "right", "bottom"]:
		margin.add_theme_constant_override("margin_%s" % side, 12)
	panel.add_child(margin)
	var box := VBoxContainer.new()
	box.add_theme_constant_override("separation", 8)
	margin.add_child(box)
	for def in [
		{"label": "\u9971\u8179", "key": "fullness"},
		{"label": "\u5fc3\u60c5", "key": "mood"},
		{"label": "\u4f53\u529b", "key": "energy"},
		{"label": "\u6e05\u6d01", "key": "cleanliness"},
		{"label": "\u4eb2\u5bc6", "key": "bond"},
	]:
		var row := VBoxContainer.new()
		var top := HBoxContainer.new()
		var name_label := Label.new()
		name_label.text = String(def["label"])
		var value_label := Label.new()
		value_label.text = "0"
		value_label.size_flags_horizontal = Control.SIZE_EXPAND_FILL
		value_label.horizontal_alignment = HORIZONTAL_ALIGNMENT_RIGHT
		top.add_child(name_label)
		top.add_child(value_label)
		var bar := ProgressBar.new()
		bar.max_value = 100
		bar.show_percentage = false
		bar.custom_minimum_size = Vector2(0, 18)
		row.add_child(top)
		row.add_child(bar)
		box.add_child(row)
		stat_bars[String(def["key"])] = bar
		stat_labels[String(def["key"])] = value_label
	var actions := GridContainer.new()
	actions.columns = 3
	actions.add_theme_constant_override("h_separation", 8)
	actions.add_theme_constant_override("v_separation", 8)
	tab.add_child(actions)
	_add_action_button(actions, "\u5582\u98df", func() -> void: _run_action("\u5582\u98df", _stage_pet().feed(_now())))
	_add_action_button(actions, "\u73a9\u4e50", func() -> void: _run_action("\u73a9\u4e50", _stage_pet().play(_now())))
	_add_action_button(actions, "\u5c0f\u7761", func() -> void: _run_action("\u5c0f\u7761", _stage_pet().nap(_now())))
	_add_action_button(actions, "\u6e05\u7406", func() -> void: _run_action("\u6e05\u7406", _stage_pet().clean_up(_now())))
	_add_action_button(actions, "\u5c0f\u6e38\u620f", func() -> void: _run_action("\u5c0f\u6e38\u620f", _stage_pet().game(_now())))
	_add_action_button(actions, "\u5916\u51fa", func() -> void: _run_action("\u5916\u51fa", _stage_pet().start_outing(_now())))
	var focus_panel := PanelContainer.new()
	tab.add_child(focus_panel)
	var focus_margin := MarginContainer.new()
	for side in ["left", "top", "right", "bottom"]:
		focus_margin.add_theme_constant_override("margin_%s" % side, 10)
	focus_panel.add_child(focus_margin)
	var focus_box := HBoxContainer.new()
	focus_box.add_theme_constant_override("separation", 10)
	focus_margin.add_child(focus_box)
	focus_status_label = Label.new()
	focus_status_label.text = "未进入专注模式"
	focus_status_label.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	focus_box.add_child(focus_status_label)
	focus_toggle_button = Button.new()
	focus_toggle_button.text = "进入专注模式"
	focus_toggle_button.focus_mode = Control.FOCUS_NONE
	focus_toggle_button.pressed.connect(_toggle_focus_mode)
	focus_box.add_child(focus_toggle_button)
	event_log = RichTextLabel.new()
	event_log.fit_content = true
	event_log.scroll_active = true
	event_log.custom_minimum_size = Vector2(0, 160)
	tab.add_child(event_log)

func _build_chat_tab(parent: TabContainer) -> void:
	var tab := VBoxContainer.new()
	tab.name = "\u804a\u5929"
	tab.add_theme_constant_override("separation", 10)
	parent.add_child(tab)
	chat_history = RichTextLabel.new()
	chat_history.fit_content = true
	chat_history.scroll_following = true
	chat_history.size_flags_vertical = Control.SIZE_EXPAND_FILL
	tab.add_child(chat_history)
	var row := HBoxContainer.new()
	tab.add_child(row)
	chat_input = LineEdit.new()
	chat_input.placeholder_text = "\u5728\u8fd9\u91cc\u8f93\u5165\u60f3\u8bf4\u7684\u8bdd..."
	chat_input.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	chat_input.text_submitted.connect(_on_chat_submitted)
	row.add_child(chat_input)
	var button := Button.new()
	button.text = "\u53d1\u9001"
	button.focus_mode = Control.FOCUS_NONE
	button.pressed.connect(func() -> void: _on_chat_submitted(chat_input.text))
	row.add_child(button)

func _build_souvenir_tab(parent: TabContainer) -> void:
	var tab := VBoxContainer.new()
	tab.name = "\u7eaa\u5ff5\u76d2"
	tab.add_theme_constant_override("separation", 10)
	parent.add_child(tab)
	var split := HSplitContainer.new()
	split.size_flags_vertical = Control.SIZE_EXPAND_FILL
	tab.add_child(split)
	souvenir_list = ItemList.new()
	souvenir_list.custom_minimum_size = Vector2(220, 0)
	souvenir_list.focus_mode = Control.FOCUS_NONE
	souvenir_list.item_selected.connect(_on_souvenir_selected)
	split.add_child(souvenir_list)
	var detail := VBoxContainer.new()
	detail.add_theme_constant_override("separation", 12)
	split.add_child(detail)
	souvenir_title = Label.new()
	souvenir_title.text = "\u8fd8\u6ca1\u6709\u7eaa\u5ff5"
	souvenir_title.add_theme_font_size_override("font_size", 24)
	detail.add_child(souvenir_title)
	souvenir_image = TextureRect.new()
	souvenir_image.expand_mode = TextureRect.EXPAND_FIT_WIDTH_PROPORTIONAL
	souvenir_image.stretch_mode = TextureRect.STRETCH_KEEP_ASPECT_CENTERED
	souvenir_image.custom_minimum_size = Vector2(320, 180)
	detail.add_child(souvenir_image)
	souvenir_note = RichTextLabel.new()
	souvenir_note.fit_content = true
	souvenir_note.custom_minimum_size = Vector2(0, 140)
	detail.add_child(souvenir_note)

func _build_colony_tab(parent: TabContainer) -> void:
	var tab := VBoxContainer.new()
	tab.name = "\u732b\u5c4b"
	tab.add_theme_constant_override("separation", 10)
	parent.add_child(tab)

	var colony_toolbar := HBoxContainer.new()
	colony_toolbar.add_theme_constant_override("separation", 8)
	tab.add_child(colony_toolbar)
	colony_selected_label = Label.new()
	colony_selected_label.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	colony_selected_label.text = "\u5148\u70b9\u4e00\u53ea\u732b"
	colony_toolbar.add_child(colony_selected_label)
	colony_prompt_button = Button.new()
	colony_prompt_button.text = "\u56de\u5e94\u5b83"
	colony_prompt_button.focus_mode = Control.FOCUS_NONE
	colony_prompt_button.visible = false
	colony_prompt_button.pressed.connect(_respond_to_active_prompt)
	colony_toolbar.add_child(colony_prompt_button)
	_add_action_button(colony_toolbar, "\u6253\u4e2a\u62db\u547c", func() -> void: _greet_active_pet())
	_add_action_button(colony_toolbar, "\u770b\u5b83\u6863\u6848", func() -> void: _open_active_pet_profile())

	colony_view = CatHouseView.new()
	colony_view.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	colony_view.size_flags_vertical = Control.SIZE_EXPAND_FILL
	colony_view.pet_selected.connect(_on_colony_pet_selected)
	tab.add_child(colony_view)

func _build_warehouse_tab(parent: TabContainer) -> void:
	var tab := VBoxContainer.new()
	tab.name = "\u732b\u4ed3"
	tab.add_theme_constant_override("separation", 10)
	parent.add_child(tab)
	warehouse_hint_label = Label.new()
	warehouse_hint_label.autowrap_mode = TextServer.AUTOWRAP_WORD_SMART
	tab.add_child(warehouse_hint_label)
	var warehouse_top_actions := HBoxContainer.new()
	warehouse_top_actions.add_theme_constant_override("separation", 8)
	tab.add_child(warehouse_top_actions)
	warehouse_return_button = Button.new()
	warehouse_return_button.text = "\u8bbe\u4e3a\u56de\u7aef\u5bf9\u8c61"
	warehouse_return_button.custom_minimum_size = Vector2(0, 38)
	warehouse_return_button.focus_mode = Control.FOCUS_NONE
	warehouse_return_button.pressed.connect(_set_selected_as_return_target)
	warehouse_top_actions.add_child(warehouse_return_button)
	var split := HSplitContainer.new()
	split.size_flags_vertical = Control.SIZE_EXPAND_FILL
	split.split_offset = 280
	tab.add_child(split)
	warehouse_list = ItemList.new()
	warehouse_list.custom_minimum_size = Vector2(260, 0)
	warehouse_list.item_selected.connect(_on_warehouse_selected)
	warehouse_list.focus_mode = Control.FOCUS_NONE
	split.add_child(warehouse_list)
	var right := VBoxContainer.new()
	right.add_theme_constant_override("separation", 10)
	right.custom_minimum_size = Vector2(320, 0)
	split.add_child(right)
	var detail_panel := PanelContainer.new()
	right.add_child(detail_panel)
	var detail_margin := MarginContainer.new()
	for side in ["left", "top", "right", "bottom"]:
		detail_margin.add_theme_constant_override("margin_%s" % side, 12)
	detail_panel.add_child(detail_margin)
	var detail_box := VBoxContainer.new()
	detail_box.add_theme_constant_override("separation", 8)
	detail_margin.add_child(detail_box)
	warehouse_name_label = Label.new()
	warehouse_name_label.add_theme_font_size_override("font_size", 24)
	warehouse_name_label.autowrap_mode = TextServer.AUTOWRAP_WORD_SMART
	detail_box.add_child(warehouse_name_label)
	warehouse_meta_label = RichTextLabel.new()
	warehouse_meta_label.fit_content = true
	warehouse_meta_label.scroll_active = false
	warehouse_meta_label.custom_minimum_size = Vector2(0, 110)
	detail_box.add_child(warehouse_meta_label)
	var stat_grid := GridContainer.new()
	stat_grid.columns = 1
	stat_grid.add_theme_constant_override("h_separation", 6)
	stat_grid.add_theme_constant_override("v_separation", 6)
	stat_grid.custom_minimum_size = Vector2(0, 140)
	detail_box.add_child(stat_grid)
	for def in [
		{"label": "\u9971\u8179", "key": "fullness"},
		{"label": "\u5fc3\u60c5", "key": "mood"},
		{"label": "\u4f53\u529b", "key": "energy"},
		{"label": "\u6e05\u6d01", "key": "cleanliness"},
		{"label": "\u4eb2\u5bc6", "key": "bond"},
	]:
		var stat_row := VBoxContainer.new()
		stat_row.add_theme_constant_override("separation", 3)
		var stat_top := HBoxContainer.new()
		var stat_name := Label.new()
		stat_name.text = String(def["label"])
		var stat_value := Label.new()
		stat_value.text = "0"
		stat_value.size_flags_horizontal = Control.SIZE_EXPAND_FILL
		stat_value.horizontal_alignment = HORIZONTAL_ALIGNMENT_RIGHT
		stat_top.add_child(stat_name)
		stat_top.add_child(stat_value)
		var stat_bar := ProgressBar.new()
		stat_bar.max_value = 100
		stat_bar.show_percentage = false
		stat_bar.custom_minimum_size = Vector2(0, 14)
		stat_row.add_child(stat_top)
		stat_row.add_child(stat_bar)
		stat_grid.add_child(stat_row)
		warehouse_stat_bars[String(def["key"])] = stat_bar
		warehouse_stat_labels[String(def["key"])] = stat_value
	warehouse_thought_title_label = Label.new()
	warehouse_thought_title_label.text = "\u5c0f\u5fc3\u601d"
	warehouse_thought_title_label.visible = false
	detail_box.add_child(warehouse_thought_title_label)
	warehouse_thought_label = RichTextLabel.new()
	warehouse_thought_label.fit_content = true
	warehouse_thought_label.scroll_active = false
	warehouse_thought_label.custom_minimum_size = Vector2(0, 88)
	warehouse_thought_label.visible = false
	detail_box.add_child(warehouse_thought_label)
	warehouse_prompt_button = Button.new()
	warehouse_prompt_button.focus_mode = Control.FOCUS_NONE
	warehouse_prompt_button.pressed.connect(_respond_to_active_prompt)
	detail_box.add_child(warehouse_prompt_button)
	warehouse_memory_title_label = Label.new()
	warehouse_memory_title_label.text = "记得的事"
	detail_box.add_child(warehouse_memory_title_label)
	warehouse_memory_label = RichTextLabel.new()
	warehouse_memory_label.fit_content = true
	warehouse_memory_label.scroll_active = true
	warehouse_memory_label.custom_minimum_size = Vector2(0, 120)
	detail_box.add_child(warehouse_memory_label)
	var scene_panel := PanelContainer.new()
	right.add_child(scene_panel)
	var scene_margin := MarginContainer.new()
	for side in ["left", "top", "right", "bottom"]:
		scene_margin.add_theme_constant_override("margin_%s" % side, 12)
	scene_panel.add_child(scene_margin)
	var scene_box := VBoxContainer.new()
	scene_box.add_theme_constant_override("separation", 8)
	scene_margin.add_child(scene_box)
	var scene_title := Label.new()
	scene_title.text = "轻提醒"
	scene_box.add_child(scene_title)
	var scene_grid := GridContainer.new()
	scene_grid.columns = 2
	scene_grid.add_theme_constant_override("h_separation", 8)
	scene_grid.add_theme_constant_override("v_separation", 8)
	scene_box.add_child(scene_grid)
	for def in [
		{"id": "reunion", "label": "开机重逢"},
		{"id": "ritual", "label": "早晚问候"},
		{"id": "weather", "label": "天气关怀"},
		{"id": "absence", "label": "缺席关怀"},
		{"id": "anniversary", "label": "记忆回放"},
	]:
		var scene_button := Button.new()
		scene_button.text = String(def["label"])
		scene_button.focus_mode = Control.FOCUS_NONE
		var scene_id := String(def["id"])
		scene_button.pressed.connect(func() -> void: _trigger_memory_scene(scene_id))
		scene_grid.add_child(scene_button)
		warehouse_scene_buttons[scene_id] = scene_button
	var care_panel := PanelContainer.new()
	right.add_child(care_panel)
	var care_margin := MarginContainer.new()
	for side in ["left", "top", "right", "bottom"]:
		care_margin.add_theme_constant_override("margin_%s" % side, 12)
	care_panel.add_child(care_margin)
	var care_box := VBoxContainer.new()
	care_box.add_theme_constant_override("separation", 8)
	care_margin.add_child(care_box)
	var care_title := Label.new()
	care_title.text = "提醒设置"
	care_box.add_child(care_title)
	care_enabled_checkbox = CheckBox.new()
	care_enabled_checkbox.text = "启用自动提醒"
	care_enabled_checkbox.toggled.connect(_on_care_settings_changed)
	care_box.add_child(care_enabled_checkbox)
	for def in [
		{"id": "reunion", "label": "开机重逢"},
		{"id": "ritual", "label": "早晚问候"},
		{"id": "weather", "label": "天气关怀"},
		{"id": "absence", "label": "缺席关怀"},
		{"id": "anniversary", "label": "记忆回放"},
	]:
		var checkbox := CheckBox.new()
		checkbox.text = String(def["label"])
		checkbox.toggled.connect(_on_care_settings_changed)
		care_box.add_child(checkbox)
		care_scene_checkboxes[String(def["id"])] = checkbox
	care_reunion_spin = _add_care_spin(care_box, "重逢阈值(小时)", 1, 24, 1)
	care_weather_spin = _add_care_spin(care_box, "天气间隔(分钟)", 1, 60, 1)
	care_absence_spin = _add_care_spin(care_box, "缺席关怀(分钟)", 5, 240, 5)
	var form_panel := PanelContainer.new()
	right.add_child(form_panel)
	var form_margin := MarginContainer.new()
	for side in ["left", "top", "right", "bottom"]:
		form_margin.add_theme_constant_override("margin_%s" % side, 12)
	form_panel.add_child(form_margin)
	var form_box := VBoxContainer.new()
	form_box.add_theme_constant_override("separation", 8)
	form_margin.add_child(form_box)
	var name_label := Label.new()
	name_label.text = "\u65b0\u589e\u732b\u54aa"
	form_box.add_child(name_label)
	warehouse_name_input = LineEdit.new()
	warehouse_name_input.placeholder_text = "\u7ed9\u65b0\u732b\u8d77\u4e2a\u540d\u5b57"
	form_box.add_child(warehouse_name_input)
	warehouse_kind_option = OptionButton.new()
	for variant in CAT_VARIANTS:
		warehouse_kind_option.add_item(String(variant["label"]))
	form_box.add_child(warehouse_kind_option)
	warehouse_personality_option = OptionButton.new()
	for variant in PERSONALITY_VARIANTS:
		warehouse_personality_option.add_item(String(variant["label"]))
	form_box.add_child(warehouse_personality_option)
	var actions := VBoxContainer.new()
	actions.add_theme_constant_override("separation", 8)
	right.add_child(actions)
	_add_action_button(actions, "\u67e5\u770b\u8fd9\u53ea\u732b", func() -> void: _select_highlighted_cat())
	_add_action_button(actions, "\u65b0\u589e\u732b\u54aa", func() -> void: _add_new_cat_from_form())
	_add_action_button(actions, "\u5220\u9664\u8fd9\u53ea\u732b", func() -> void: _delete_selected_cat())

func _build_sync_tab(parent: TabContainer) -> void:
	var tab := VBoxContainer.new()
	tab.name = "\u8054\u52a8"
	tab.add_theme_constant_override("separation", 10)
	parent.add_child(tab)
	sync_status_label = Label.new()
	sync_status_label.text = "\u8fd8\u6ca1\u8fdb\u57ce\u540c\u6b65"
	tab.add_child(sync_status_label)
	sync_state_label = Label.new()
	sync_state_label.text = "\u5148\u5728\u7aef\u4fa7\u6309 T \u8ba9\u5c0f\u732b\u8fdb\u57ce"
	tab.add_child(sync_state_label)
	var host_row := HBoxContainer.new()
	tab.add_child(host_row)
	sync_host_input = LineEdit.new()
	sync_host_input.placeholder_text = "\u8bbe\u5907 IP\uff0c\u4f8b\u5982 192.168.212.104"
	sync_host_input.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	host_row.add_child(sync_host_input)
	sync_toggle_button = Button.new()
	sync_toggle_button.text = "\u505c\u6b62\u76d1\u542c"
	sync_toggle_button.focus_mode = Control.FOCUS_NONE
	sync_toggle_button.pressed.connect(_toggle_bridge)
	host_row.add_child(sync_toggle_button)
	var sync_mode_row := HBoxContainer.new()
	tab.add_child(sync_mode_row)
	sync_enter_button = Button.new()
	sync_enter_button.text = "\u8fdb\u57ce\u540c\u6b65"
	sync_enter_button.focus_mode = Control.FOCUS_NONE
	sync_enter_button.pressed.connect(_on_sync_enter_pressed)
	sync_mode_row.add_child(sync_enter_button)
	sync_leave_button = Button.new()
	sync_leave_button.text = "\u51fa\u57ce\u56de\u5bb6"
	sync_leave_button.focus_mode = Control.FOCUS_NONE
	sync_leave_button.disabled = true
	sync_leave_button.pressed.connect(_on_sync_leave_pressed)
	sync_mode_row.add_child(sync_leave_button)
	var send_row := HBoxContainer.new()
	tab.add_child(send_row)
	sync_push_input = LineEdit.new()
	sync_push_input.placeholder_text = "\u628a\u4e00\u53e5\u8bdd\u63a8\u9001\u5230\u8bbe\u5907\u804a\u5929\u6846"
	sync_push_input.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	sync_push_input.text_submitted.connect(_on_push_text_submitted)
	send_row.add_child(sync_push_input)
	var push_button := Button.new()
	push_button.text = "\u63a8\u9001"
	push_button.focus_mode = Control.FOCUS_NONE
	push_button.pressed.connect(func() -> void: _on_push_text_submitted(sync_push_input.text))
	send_row.add_child(push_button)
	sync_log = RichTextLabel.new()
	sync_log.fit_content = true
	sync_log.scroll_active = true
	sync_log.size_flags_vertical = Control.SIZE_EXPAND_FILL
	tab.add_child(sync_log)

func _add_action_button(parent: Control, text: String, callable: Callable) -> void:
	var button := Button.new()
	button.text = text
	button.custom_minimum_size = Vector2(0, 42)
	button.focus_mode = Control.FOCUS_NONE
	button.pressed.connect(callable)
	parent.add_child(button)

func _add_care_spin(parent: Control, label_text: String, min_value: int, max_value: int, step_value: int) -> SpinBox:
	var row := HBoxContainer.new()
	row.add_theme_constant_override("separation", 8)
	parent.add_child(row)
	var label := Label.new()
	label.text = label_text
	label.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	row.add_child(label)
	var spin := SpinBox.new()
	spin.min_value = min_value
	spin.max_value = max_value
	spin.step = step_value
	spin.custom_minimum_size = Vector2(90, 0)
	spin.value_changed.connect(_on_care_settings_value_changed)
	row.add_child(spin)
	return spin

func _now() -> float:
	return Time.get_ticks_msec() / 1000.0

func _current_pet() -> PetState:
	for pet in cats:
		if pet.pet_id == active_pet_id:
			return pet
	return cats[0] if not cats.is_empty() else null

func _stage_pet() -> PetState:
	var device_pet := _find_pet_by_id(current_device_pet_id)
	if device_pet != null:
		return device_pet
	return _current_pet()

func _scene_panel_pet() -> PetState:
	if tabs_view != null and tabs_view.current_tab in [TAB_ACTIONS, TAB_SOUVENIRS]:
		return _stage_pet()
	return _current_pet()

func _find_pet_by_id(pet_id: String) -> PetState:
	for pet in cats:
		if pet.pet_id == pet_id:
			return pet
	return null

func _pet_matches_snapshot_identity(pet: PetState, pet_obj: Dictionary) -> bool:
	if pet == null:
		return false
	var incoming_name := String(pet_obj.get("name", "")).strip_edges()
	var incoming_kind := String(pet_obj.get("kind", "")).strip_edges()
	var incoming_personality := String(pet_obj.get("personality", "")).strip_edges()
	if incoming_name.is_empty():
		return false
	return pet.pet_name == incoming_name and pet.pet_kind == incoming_kind and pet.personality == incoming_personality

func _find_sync_merge_candidate(pet_obj: Dictionary) -> PetState:
	var incoming_id := String(pet_obj.get("id", "")).strip_edges()
	if not incoming_id.is_empty():
		var by_id := _find_pet_by_id(incoming_id)
		if by_id != null:
			return by_id

	var current_device := _find_pet_by_id(current_device_pet_id)
	if _pet_matches_snapshot_identity(current_device, pet_obj):
		return current_device

	var selected_return := _find_pet_by_id(selected_return_pet_id)
	if _pet_matches_snapshot_identity(selected_return, pet_obj):
		return selected_return

	var exact_matches: Array[PetState] = []
	for pet in cats:
		if _pet_matches_snapshot_identity(pet, pet_obj):
			exact_matches.append(pet)
	if exact_matches.size() == 1:
		return exact_matches[0]

	var incoming_name := String(pet_obj.get("name", "")).strip_edges()
	var incoming_kind := String(pet_obj.get("kind", "")).strip_edges()
	if not incoming_name.is_empty() and not incoming_kind.is_empty():
		var loose_matches: Array[PetState] = []
		for pet in cats:
			if pet.pet_name == incoming_name and pet.pet_kind == incoming_kind:
				loose_matches.append(pet)
		if loose_matches.size() == 1:
			return loose_matches[0]

	return null

func _collapse_duplicate_identity_cats(target: PetState) -> void:
	if target == null:
		return
	var to_remove: Array[int] = []
	for i in cats.size():
		var pet := cats[i]
		if pet == target:
			continue
		if pet.pet_name == target.pet_name and pet.pet_kind == target.pet_kind and pet.personality == target.personality:
			to_remove.append(i)
	if to_remove.is_empty():
		return
	to_remove.sort()
	to_remove.reverse()
	for index in to_remove:
		var duplicate := cats[index]
		if active_pet_id == duplicate.pet_id:
			active_pet_id = target.pet_id
		if current_device_pet_id == duplicate.pet_id:
			current_device_pet_id = target.pet_id
		if selected_return_pet_id == duplicate.pet_id:
			selected_return_pet_id = target.pet_id
		DesktopPersistence.delete_pet_memory(duplicate.pet_id)
		cats.remove_at(index)

func _ensure_default_roster() -> void:
	if cats.is_empty():
		var orange := PetState.create_named("\u5c0f\u6a58", "orange", PetState.PERSONALITY_LIVELY)
		var purple := PetState.create_named("\u5c0f\u8461\u8404", "purple", PetState.PERSONALITY_CALM)
		cats = [orange, purple]
		current_device_pet_id = orange.pet_id
		selected_return_pet_id = orange.pet_id
		active_pet_id = orange.pet_id
	if active_pet_id.is_empty():
		active_pet_id = cats[0].pet_id
	if current_device_pet_id.is_empty():
		current_device_pet_id = cats[0].pet_id
	if selected_return_pet_id.is_empty():
		selected_return_pet_id = current_device_pet_id

func _add_new_cat_from_form() -> void:
	var kind_index: int = clamp(warehouse_kind_option.selected, 0, CAT_VARIANTS.size() - 1)
	var personality_index: int = clamp(warehouse_personality_option.selected, 0, PERSONALITY_VARIANTS.size() - 1)
	var kind: String = String(CAT_VARIANTS[kind_index]["kind"])
	var personality: String = String(PERSONALITY_VARIANTS[personality_index]["key"])
	var chosen_name: String = warehouse_name_input.text.strip_edges()
	if chosen_name.is_empty():
		chosen_name = "\u65b0\u732b\u54aa%d" % [cats.size() + 1]
	var pet := PetState.create_named(chosen_name, kind, personality)
	cats.append(pet)
	DesktopPersistence.ensure_pet_memory_file(pet.pet_id, pet.to_memory_dict())
	active_pet_id = pet.pet_id
	warehouse_name_input.clear()
	warehouse_kind_option.selected = 0
	warehouse_personality_option.selected = 0
	_append_sync_log("\u5df2\u65b0\u589e %s\uff0c%s\uff0c%s" % [pet.pet_name, _kind_label(pet.pet_kind), pet.personality_label()])
	_refresh_all()

func _delete_selected_cat() -> void:
	var selected := warehouse_list.get_selected_items()
	if selected.is_empty():
		return
	var pet := cats[selected[0]]
	if cats.size() <= 1:
		_append_sync_log("\u81f3\u5c11\u8981\u4fdd\u7559\u4e00\u53ea\u732b")
		return
	if pet.pet_id == current_device_pet_id:
		_append_sync_log("\u5f53\u524d\u5728\u7aef\u4fa7\u7684\u732b\u4e0d\u80fd\u5220\u9664")
		return
	DesktopPersistence.delete_pet_memory(pet.pet_id)
	cats.remove_at(selected[0])
	if active_pet_id == pet.pet_id:
		active_pet_id = cats[0].pet_id
	if selected_return_pet_id == pet.pet_id:
		selected_return_pet_id = current_device_pet_id if not current_device_pet_id.is_empty() else cats[0].pet_id
	_append_sync_log("\u5df2\u5220\u9664 %s" % pet.pet_name)
	_refresh_all()

func _refresh_all() -> void:
	var pet := _scene_panel_pet()
	if pet == null:
		return
	stage_view.set_pet(pet)
	stage_view.refresh_from_pet()
	stage_view.set_focus_scene(focus_mode_active and pet.pet_id == focus_mode_pet_id, focus_mode_activity)
	stage_view.set_status_text(_stage_status_line(pet))
	_set_stat("fullness", pet.fullness)
	_set_stat("mood", pet.mood)
	_set_stat("energy", pet.energy)
	_set_stat("cleanliness", pet.cleanliness)
	_set_stat("bond", pet.bond)
	_refresh_event_log()
	_refresh_chat_history()
	_refresh_souvenirs()
	_refresh_colony_view()
	_refresh_warehouse()
	if focus_status_label != null:
		focus_status_label.text = _focus_mode_status_text()
	if focus_toggle_button != null:
		focus_toggle_button.text = ("结束专注模式" if focus_mode_active else "进入专注模式")
	_refresh_tab_titles()
	_update_stage_visibility()

func _refresh_tab_titles() -> void:
	if tabs_view == null:
		return
	tabs_view.set_tab_title(TAB_ACTIONS, "互动" + (" · 专注中" if focus_mode_active else ""))
	tabs_view.set_tab_title(TAB_CHAT, "聊天")
	tabs_view.set_tab_title(TAB_SOUVENIRS, "纪念盒")
	tabs_view.set_tab_title(TAB_CAT_HOUSE, "猫屋")
	tabs_view.set_tab_title(TAB_WAREHOUSE, "猫仓")
	tabs_view.set_tab_title(TAB_SYNC, "联动")

func _set_stat(key: String, value: int) -> void:
	var bar: ProgressBar = stat_bars[key]
	var value_label: Label = stat_labels[key]
	bar.value = value
	value_label.text = str(value)

func _refresh_event_log() -> void:
	var pet := _scene_panel_pet()
	event_log.clear()
	for line: String in pet.event_history:
		event_log.append_text("• %s\n" % line)

func _refresh_chat_history() -> void:
	var pet := _current_pet()
	chat_history.clear()
	for row: Dictionary in pet.chat_entries:
		var role := "\u4f60" if String(row.get("role", "ai")) == "user" else pet.pet_name
		chat_history.append_text("[%s] %s\n" % [role, String(row.get("text", ""))])

func _refresh_souvenirs() -> void:
	var pet := _scene_panel_pet()
	var previous := souvenir_list.get_selected_items()
	var previous_index := previous[0] if previous.size() > 0 else -1
	souvenir_list.clear()
	for item: Dictionary in pet.souvenirs:
		souvenir_list.add_item("%s | %s" % [_type_label(String(item.get("type", ""))), String(item.get("title", ""))])
	if pet.souvenirs.is_empty():
		souvenir_title.text = "\u8fd8\u6ca1\u6709\u7eaa\u5ff5"
		souvenir_note.text = "%s \u5148\u53bb\u6563\u6563\u6b65\u5427\u3002" % pet.pet_name
		souvenir_image.texture = null
		return
	var target_index: int = clamp(previous_index, 0, pet.souvenirs.size() - 1)
	souvenir_list.select(target_index)
	_show_souvenir(target_index)

func _refresh_colony_view() -> void:
	if colony_view == null:
		return
	var colony_active_id := active_pet_id
	if not current_device_pet_id.is_empty() and colony_active_id == current_device_pet_id:
		for pet in cats:
			if pet.pet_id != current_device_pet_id:
				colony_active_id = pet.pet_id
				break
	colony_view.set_roster(cats, colony_active_id, current_device_pet_id, selected_return_pet_id, town_sync_active)
	if colony_selected_label != null:
		var pet := _find_pet_by_id(colony_active_id)
		if pet == null:
			colony_selected_label.text = "\u5148\u70b9\u4e00\u53ea\u732b"
		else:
			if pet.has_active_prompt():
				colony_selected_label.text = "\u5df2\u9009\u4e2d\uff1a%s  |  %s" % [pet.pet_name, String(pet.active_prompt.get("title", ""))]
			else:
				colony_selected_label.text = "\u5df2\u9009\u4e2d\uff1a%s" % pet.pet_name
	if colony_prompt_button != null:
		var selected_pet := _find_pet_by_id(colony_active_id)
		colony_prompt_button.visible = selected_pet != null and selected_pet.has_active_prompt()

func _refresh_warehouse() -> void:
	warehouse_list.clear()
	for pet in cats:
		var where := "\u7aef\u4fa7" if pet.pet_id == current_device_pet_id else "\u732b\u4ed3"
		if town_sync_active and pet.pet_id == current_device_pet_id:
			where = "\u8fdb\u57ce\u4e2d"
		var badges: Array[String] = []
		if pet.pet_id == selected_return_pet_id:
			badges.append("[\u56de]")
		if pet.has_active_prompt():
			badges.append("[\u5fc3]")
		var badge_text := ""
		if not badges.is_empty():
			badge_text = "  " + "".join(badges)
		warehouse_list.add_item("%s\n%s | %s%s" % [pet.pet_name, _kind_label(pet.pet_kind), where, badge_text])
	if not cats.is_empty():
		var index := _index_for_pet(active_pet_id)
		if index >= 0:
			warehouse_list.select(index)
	var current_name := _find_pet_by_id(current_device_pet_id).pet_name if _find_pet_by_id(current_device_pet_id) != null else "\u65e0"
	var return_name := _find_pet_by_id(selected_return_pet_id).pet_name if _find_pet_by_id(selected_return_pet_id) != null else "\u672a\u9009"
	warehouse_hint_label.text = "\u5f53\u524d\u7aef\u4fa7\uff1a%s\n\u51c6\u5907\u56de\u7aef\uff1a%s" % [current_name, return_name]
	_refresh_care_settings_ui()
	_refresh_warehouse_detail()

func _refresh_warehouse_detail() -> void:
	if warehouse_name_label == null:
		return
	var pet := _current_pet()
	if pet == null:
		warehouse_name_label.text = "\u6ca1\u6709\u53ef\u663e\u793a\u7684\u732b\u54aa"
		warehouse_meta_label.clear()
		for key in warehouse_stat_bars.keys():
			var empty_bar: ProgressBar = warehouse_stat_bars[key]
			var empty_label: Label = warehouse_stat_labels[key]
			empty_bar.value = 0
			empty_label.text = "0"
		warehouse_thought_title_label.visible = false
		warehouse_thought_label.visible = false
		warehouse_memory_label.clear()
		for button: Button in warehouse_scene_buttons.values():
			button.disabled = true
		return
	var where := "\u7aef\u4fa7" if pet.pet_id == current_device_pet_id else "\u7535\u8111\u4ed3\u5e93"
	if town_sync_active and pet.pet_id == current_device_pet_id:
		where = "\u8fdb\u57ce\u540c\u6b65\u4e2d"
	var target_text := "\u662f" if pet.pet_id == selected_return_pet_id else "\u5426"
	warehouse_name_label.text = pet.pet_name
	if warehouse_return_button != null:
		warehouse_return_button.text = ("\u5df2\u662f\u56de\u7aef\u5bf9\u8c61" if pet.pet_id == selected_return_pet_id else "\u8bbe\u4e3a\u56de\u7aef\u5bf9\u8c61")
		warehouse_return_button.disabled = pet.pet_id == selected_return_pet_id
	warehouse_meta_label.clear()
	warehouse_meta_label.append_text("[b]\u5f62\u8c61[/b] %s\n" % _kind_label(pet.pet_kind))
	warehouse_meta_label.append_text("[b]\u6027\u683c[/b] %s\n" % pet.personality_label())
	warehouse_meta_label.append_text("[b]\u5173\u7cfb[/b] %s\n" % pet.affection_label())
	warehouse_meta_label.append_text("[b]\u4f4d\u7f6e[/b] %s\n" % where)
	warehouse_meta_label.append_text("[b]\u56de\u7aef[/b] %s\n" % target_text)
	warehouse_meta_label.append_text("[b]\u8fd1\u51b5[/b] %s" % pet.status_summary())
	_set_warehouse_stat("fullness", pet.fullness)
	_set_warehouse_stat("mood", pet.mood)
	_set_warehouse_stat("energy", pet.energy)
	_set_warehouse_stat("cleanliness", pet.cleanliness)
	_set_warehouse_stat("bond", pet.bond)
	if pet.has_active_prompt():
		warehouse_thought_title_label.visible = true
		warehouse_thought_label.visible = true
		warehouse_thought_label.clear()
		warehouse_thought_label.append_text("[b]%s[/b]\n" % String(pet.active_prompt.get("title", "")))
		warehouse_thought_label.append_text("%s" % String(pet.active_prompt.get("body", "")))
		warehouse_prompt_button.visible = true
		warehouse_prompt_button.text = _prompt_button_label(String(pet.active_prompt.get("type", "")), pet.personality)
	else:
		warehouse_thought_title_label.visible = false
		warehouse_thought_label.visible = false
		warehouse_thought_label.clear()
		warehouse_prompt_button.visible = false
		warehouse_prompt_button.text = ""
	warehouse_memory_label.clear()
	var memory_lines := pet.memory_preview_lines(6)
	if memory_lines.is_empty():
		warehouse_memory_label.append_text("还没有特别想记下来的事。")
	else:
		for line: String in memory_lines:
			warehouse_memory_label.append_text("• %s\n" % line)
	for button: Button in warehouse_scene_buttons.values():
		button.disabled = false

func _refresh_care_settings_ui() -> void:
	if care_enabled_checkbox == null:
		return
	care_enabled_checkbox.button_pressed = bool(care_settings.get("enabled", true))
	for key: String in care_scene_checkboxes.keys():
		var checkbox: CheckBox = care_scene_checkboxes[key]
		checkbox.button_pressed = bool(care_settings.get(key, true))
		checkbox.disabled = not care_enabled_checkbox.button_pressed
	if care_reunion_spin != null:
		care_reunion_spin.value = int(care_settings.get("reunion_hours", 1))
		care_reunion_spin.editable = care_enabled_checkbox.button_pressed and bool(care_settings.get("reunion", true))
	if care_weather_spin != null:
		care_weather_spin.value = int(care_settings.get("weather_cooldown_minutes", 3))
		care_weather_spin.editable = care_enabled_checkbox.button_pressed and bool(care_settings.get("weather", true))
	if care_absence_spin != null:
		care_absence_spin.value = int(care_settings.get("absence_minutes", 30))
		care_absence_spin.editable = care_enabled_checkbox.button_pressed and bool(care_settings.get("absence", true))

func _on_care_settings_changed(_toggled: bool) -> void:
	_apply_care_settings_from_ui()

func _on_care_settings_value_changed(_value: float) -> void:
	_apply_care_settings_from_ui()

func _apply_care_settings_from_ui() -> void:
	if care_enabled_checkbox == null:
		return
	care_settings["enabled"] = care_enabled_checkbox.button_pressed
	for key: String in care_scene_checkboxes.keys():
		var checkbox: CheckBox = care_scene_checkboxes[key]
		care_settings[key] = checkbox.button_pressed
	care_settings["reunion_hours"] = int(care_reunion_spin.value) if care_reunion_spin != null else 1
	care_settings["weather_cooldown_minutes"] = int(care_weather_spin.value) if care_weather_spin != null else 3
	care_settings["absence_minutes"] = int(care_absence_spin.value) if care_absence_spin != null else 30
	_refresh_care_settings_ui()

func _set_warehouse_stat(key: String, value: int) -> void:
	if not warehouse_stat_bars.has(key):
		return
	var bar: ProgressBar = warehouse_stat_bars[key]
	var value_label: Label = warehouse_stat_labels[key]
	bar.value = value
	value_label.text = str(value)

func _prompt_button_label(prompt_type: String, personality: String) -> String:
	match personality:
		PetState.PERSONALITY_CLINGY:
			match prompt_type:
				"care":
					return "\u73b0\u5728\u5c31\u53bb\u770b\u770b\u5b83"
				"rest":
					return "\u966a\u5b83\u4e00\u8d77\u4f11\u606f"
				"outing":
					return "\u7b54\u5e94\u966a\u5b83\u51fa\u95e8"
				"share":
					return "\u53bb\u966a\u966a\u5b83"
				"question":
					return "\u56de\u5b83\u4e00\u53e5"
				"chat":
					return "\u73b0\u5728\u5c31\u8ddf\u5b83\u804a"
		PetState.PERSONALITY_CALM:
			match prompt_type:
				"care":
					return "\u66ff\u5b83\u6536\u62fe\u4e00\u4e0b"
				"rest":
					return "\u8ba9\u5b83\u597d\u597d\u4f11\u606f"
				"outing":
					return "\u966a\u5b83\u6162\u6162\u51fa\u95e8"
				"share":
					return "\u542c\u5b83\u6162\u6162\u8bf4"
				"question":
					return "\u6162\u6162\u56de\u5b83"
				"chat":
					return "\u5750\u4e0b\u6765\u542c\u5b83\u8bf4"
		PetState.PERSONALITY_ALOOF:
			match prompt_type:
				"care":
					return "\u987a\u624b\u7167\u987e\u4e00\u4e0b"
				"rest":
					return "\u8ba9\u5b83\u5148\u772f\u4e00\u4f1a"
				"outing":
					return "\u7ed9\u5b83\u4e00\u70b9\u51fa\u95e8\u65f6\u95f4"
				"share":
					return "\u966a\u5b83\u5750\u4e00\u4f1a\u513f"
				"question":
					return "\u987a\u624b\u56de\u5b83"
				"chat":
					return "\u542c\u542c\u5b83\u60f3\u8bf4\u4ec0\u4e48"
	match prompt_type:
		"care":
			return "\u73b0\u5728\u53bb\u7167\u987e\u5b83"
		"rest":
			return "\u966a\u5b83\u5c0f\u7761"
		"outing":
			return "\u7b54\u5e94\u5b83\u51fa\u95e8"
		"share":
			return "\u966a\u5b83\u5f85\u4f1a\u513f"
		"question":
			return "\u56de\u5b83\u4e00\u53e5"
		"chat":
			return "\u73b0\u5728\u5c31\u804a\u804a"
		_:
			return "\u56de\u5e94\u8fd9\u4e2a\u5c0f\u5fc3\u601d"

func _respond_to_active_prompt() -> void:
	var pet := _current_pet()
	if pet == null or not pet.has_active_prompt():
		return
	pet.note_player_interaction()
	var prompt_type := String(pet.active_prompt.get("type", ""))
	var prompt_title := String(pet.active_prompt.get("title", ""))
	match prompt_type:
		"care":
			if pet.fullness < 35:
				pet.feed(_now())
			else:
				pet.clean_up(_now())
		"rest":
			pet.nap(_now())
		"outing":
			pet.start_outing(_now())
		"share":
			pet.mood = min(PetState.MAX_STAT, pet.mood + 4)
			pet.bond = min(PetState.MAX_STAT, pet.bond + 3)
			pet.say_ambient(pet.prompt_response_line(prompt_type), 10.0)
			pet.clear_active_prompt()
		"chat":
			pet.say_ambient(pet.prompt_response_line(prompt_type), 10.0)
			pet.clear_active_prompt()
		"question":
			pet.mood = min(PetState.MAX_STAT, pet.mood + 5)
			pet.bond = min(PetState.MAX_STAT, pet.bond + 4)
			pet.say_ambient(pet.prompt_response_line(prompt_type), 10.0)
			pet.clear_active_prompt()
		_:
			pet.play(_now())
	pet.remember_prompt_response(prompt_type, prompt_title)
	_refresh_all()

func _index_for_pet(pet_id: String) -> int:
	for i in cats.size():
		if cats[i].pet_id == pet_id:
			return i
	return -1

func _on_souvenir_selected(index: int) -> void:
	_show_souvenir(index)

func _on_tab_changed(_tab: int) -> void:
	_update_stage_visibility()

func _update_stage_visibility() -> void:
	if top_panel == null or tabs_view == null:
		return
	var show_stage := tabs_view.current_tab in [0, 2]
	top_panel.visible = show_stage

func _show_souvenir(index: int) -> void:
	var pet := _scene_panel_pet()
	if index < 0 or index >= pet.souvenirs.size():
		return
	var item: Dictionary = pet.souvenirs[index]
	var kind := String(item.get("type", ""))
	var key := String(item.get("key", ""))
	souvenir_title.text = "%s | %s" % [_type_label(kind), String(item.get("title", ""))]
	souvenir_note.text = String(item.get("note", ""))
	var texture_path := "res://assets/photos/%s.png" % key
	if kind == "photo" and ResourceLoader.exists(texture_path):
		souvenir_image.texture = load(texture_path)
	else:
		souvenir_image.texture = null

func _type_label(kind: String) -> String:
	match kind:
		"photo":
			return "\u7167\u7247"
		"note":
			return "\u7eb8\u6761"
		"item":
			return "\u5c0f\u7269"
		_:
			return "\u7eaa\u5ff5"

func _kind_label(kind: String) -> String:
	match kind:
		"purple":
			return "\u7d2b\u732b"
		_:
			return "\u6a58\u732b"

func _focus_pet() -> PetState:
	if focus_mode_pet_id.is_empty():
		return _current_pet()
	return _find_pet_by_id(focus_mode_pet_id)

func _is_focus_interrupting_key(event: InputEventKey) -> bool:
	if event.keycode in [KEY_CTRL, KEY_SHIFT, KEY_ALT, KEY_META]:
		return false
	return true

func _toggle_focus_mode() -> void:
	if focus_mode_active:
		_finish_focus_mode(false)
	else:
		_start_focus_mode()

func _start_focus_mode() -> void:
	var pet := _current_pet()
	if pet == null:
		return
	focus_mode_active = true
	focus_mode_started_at = _now()
	focus_mode_ends_at = focus_mode_started_at + FOCUS_MODE_DURATION
	focus_mode_last_warn_at = -1000.0
	focus_mode_pet_id = pet.pet_id
	focus_mode_activity = String(FOCUS_ACTIVITIES[randi() % FOCUS_ACTIVITIES.size()])
	pet.note_player_interaction()
	pet.say_ambient("我在这里陪你专心。", 10.0)
	pet._push_event("开始陪你专注%d分钟" % int(FOCUS_MODE_DURATION / 60.0))
	_append_sync_log("%s 进入了专注陪伴模式" % pet.pet_name)
	if stage_view != null:
		stage_view.trigger_idle()
	_refresh_all()

func _finish_focus_mode(completed: bool) -> void:
	var pet := _focus_pet()
	focus_mode_active = false
	focus_mode_started_at = 0.0
	focus_mode_ends_at = 0.0
	focus_mode_last_warn_at = -1000.0
	focus_mode_activity = ""
	focus_mode_pet_id = ""
	if pet == null:
		_refresh_all()
		return
	if completed:
		pet.mood = min(PetState.MAX_STAT, pet.mood + 10)
		pet.bond = min(PetState.MAX_STAT, pet.bond + 8)
		pet.say_ambient("做得很好，休息一下吧。", 10.0)
		pet._push_event("你们一起完成了一次专注")
		if stage_view != null:
			stage_view.trigger_happy()
	else:
		pet.say_ambient("没关系，我们下次再一起专心。", 8.0)
		pet._push_event("这次专注先暂停了")
	_append_sync_log("%s" % ("专注完成" if completed else "专注已结束"))
	_refresh_all()

func _handle_focus_interrupt() -> void:
	var now := _now()
	if now - focus_mode_last_warn_at < FOCUS_WARN_COOLDOWN:
		return
	focus_mode_last_warn_at = now
	var pet := _focus_pet()
	if pet == null:
		return
	pet.say_ambient("专心哦，我在这里陪你。", 6.0)
	if stage_view != null:
		stage_view.trigger_talk(800)
	_refresh_all()

func _focus_mode_status_text() -> String:
	if not focus_mode_active:
		return "未进入专注模式"
	var remaining: float = max(0.0, focus_mode_ends_at - _now())
	var minutes: int = int(remaining) / 60
	var seconds: int = int(remaining) % 60
	return "专注中 %02d:%02d · %s" % [minutes, seconds, focus_mode_activity]

func _stage_status_line(pet: PetState) -> String:
	if focus_mode_active and pet != null and pet.pet_id == focus_mode_pet_id:
		return "%s | %s" % [pet.pet_name, _focus_mode_status_text()]
	return "%s | %s" % [pet.pet_name, pet.status_summary()]

func _run_action(name: String, ok: bool) -> void:
	var pet := _current_pet()
	pet.note_player_interaction()
	if ok:
		pet.event_history.push_front("\u6267\u884c\u52a8\u4f5c\uff1a%s" % name)
		if pet.event_history.size() > 12:
			pet.event_history.resize(12)
		match name:
			"\u5582\u98df", "\u73a9\u4e50", "\u6e05\u7406", "\u5c0f\u6e38\u620f", "\u5916\u51fa":
				stage_view.trigger_happy()
			"\u5c0f\u7761":
				stage_view.trigger_idle()
	else:
		pet.event_history.push_front("\u6682\u65f6\u4e0d\u60f3%s" % name)
		if pet.event_history.size() > 12:
			pet.event_history.resize(12)
	_refresh_all()

func _on_tick() -> void:
	for pet in cats:
		pet.tick(1.0, _now())
	for pet in cats:
		pet.maybe_auto_care_scene(care_settings, latest_weather_label)
	if focus_mode_active and _now() >= focus_mode_ends_at:
		_finish_focus_mode(true)
	_refresh_all()

func _on_chat_submitted(text: String) -> void:
	var pet := _current_pet()
	var cleaned := text.strip_edges()
	if cleaned.is_empty():
		return
	pet.note_player_interaction()
	pet.add_chat("user", cleaned)
	chat_input.clear()
	var reply := pet.chat_reply(cleaned)
	stage_view.trigger_talk()
	pet.add_chat("ai", reply)
	_refresh_chat_history()

func _on_warehouse_selected(index: int) -> void:
	if index < 0 or index >= cats.size():
		return
	active_pet_id = cats[index].pet_id
	_refresh_all()

func _on_colony_pet_selected(pet_id: String) -> void:
	active_pet_id = pet_id
	var pet := _find_pet_by_id(pet_id)
	if pet != null and not pet.has_active_prompt():
		pet.note_player_interaction()
		pet.say_ambient(pet.ambient_line_for("click"), 8.5)
	_refresh_all()

func _greet_active_pet() -> void:
	var pet := _current_pet()
	if pet == null:
		return
	pet.note_player_interaction()
	var line := pet.ambient_line_for("greet")
	pet.say_ambient(line, 9.5)
	_refresh_all()

func _open_active_pet_profile() -> void:
	if tabs_view != null:
		tabs_view.current_tab = TAB_WAREHOUSE

func _select_highlighted_cat() -> void:
	var selected := warehouse_list.get_selected_items()
	if selected.is_empty():
		return
	active_pet_id = cats[selected[0]].pet_id
	_refresh_all()

func _set_selected_as_return_target() -> void:
	var selected := warehouse_list.get_selected_items()
	if selected.is_empty():
		return
	selected_return_pet_id = cats[selected[0]].pet_id
	_refresh_warehouse()
	_append_sync_log("\u5df2\u9009\u62e9 %s \u4f5c\u4e3a\u56de\u7aef\u5bf9\u8c61" % cats[selected[0]].pet_name)

func _toggle_bridge() -> void:
	if bridge == null:
		return
	if bridge.listening:
		bridge.stop_listening()
	else:
		bridge.start_listening()

func _set_sync_mode(active: bool, message: String = "") -> void:
	town_sync_active = active
	sync_enter_button.disabled = active
	sync_leave_button.disabled = not active
	if active:
		sync_heartbeat_timer.start()
	else:
		sync_heartbeat_timer.stop()
	if not message.is_empty():
		sync_status_label.text = message
	_refresh_warehouse()

func _on_sync_enter_pressed() -> void:
	var result: Dictionary = bridge.request_sync_enter(sync_host_input.text)
	if not bool(result.get("ok", false)):
		var message := String(result.get("error", "\u8fdb\u57ce\u5931\u8d25"))
		sync_status_label.text = "\u8fdb\u57ce\u5931\u8d25"
		sync_state_label.text = message
		_append_sync_log("\u8fdb\u57ce\u5931\u8d25: %s" % message)
		return
	var snapshot: Variant = result.get("snapshot", {})
	if snapshot is Dictionary:
		var synced_pet := _apply_device_snapshot(snapshot as Dictionary)
		active_pet_id = synced_pet.pet_id
		selected_return_pet_id = synced_pet.pet_id
		current_device_pet_id = synced_pet.pet_id
	_refresh_all()
	_set_sync_mode(true, "\u5df2\u7ecf\u63a5\u5230\u57ce\u91cc")
	sync_state_label.text = "\u5f53\u524d\u7aef\u4fa7\u5c0f\u732b\u5df2\u7ecf\u5728\u7535\u8111\u7aef\u4ed3\u5e93\u91cc\uff0c\u53ef\u4ee5\u9009\u62e9\u4efb\u610f\u4e00\u53ea\u56de\u7aef\u3002"
	_append_sync_log("\u5df2\u8fdb\u57ce\u540c\u6b65\uff0c\u63a5\u624b\u7aef\u4fa7\u72b6\u6001")

func _apply_device_snapshot(snapshot: Dictionary) -> PetState:
	var pet_obj: Dictionary = snapshot.get("pet", {})
	var incoming_id := String(pet_obj.get("id", current_device_pet_id))
	var target := _find_sync_merge_candidate(pet_obj)
	if target == null:
		target = PetState.new()
		cats.append(target)
	var old_pet_id := target.pet_id
	target.load_from_sync_dict(snapshot)
	target.ensure_identity()
	if not incoming_id.is_empty() and old_pet_id != incoming_id:
		DesktopPersistence.rename_pet_memory(old_pet_id, incoming_id, target.to_memory_dict())
	_collapse_duplicate_identity_cats(target)
	return target

func _on_sync_leave_pressed() -> void:
	var chosen := _find_pet_by_id(selected_return_pet_id)
	if chosen == null:
		sync_status_label.text = "\u6ca1\u6709\u53ef\u56de\u7aef\u7684\u732b"
		return
	var snapshot: Dictionary = chosen.to_sync_dict()
	var result: Dictionary = bridge.request_sync_leave(sync_host_input.text, snapshot)
	if not bool(result.get("ok", false)):
		var message := String(result.get("error", "\u51fa\u57ce\u5931\u8d25"))
		sync_status_label.text = "\u51fa\u57ce\u5931\u8d25"
		sync_state_label.text = message
		_append_sync_log("\u51fa\u57ce\u5931\u8d25: %s" % message)
		return
	current_device_pet_id = chosen.pet_id
	active_pet_id = chosen.pet_id
	_set_sync_mode(false, "\u5df2\u7ecf\u9001\u56de\u7aef\u4fa7")
	sync_state_label.text = "%s \u5df2\u7ecf\u56de\u5230\u7aef\u4fa7\u3002" % chosen.pet_name
	_append_sync_log("\u5df2\u628a %s \u9001\u56de\u7aef\u4fa7" % chosen.pet_name)
	_refresh_all()

func _on_sync_heartbeat() -> void:
	if not town_sync_active:
		return
	var result: Dictionary = bridge.request_sync_ping(sync_host_input.text)
	if bool(result.get("ok", false)):
		return
	_set_sync_mode(false, "\u8fde\u63a5\u5df2\u65ad\u5f00")
	sync_state_label.text = "\u7aef\u4fa7\u5e94\u8be5\u5df2\u7ecf\u81ea\u5df1\u56de\u5bb6\u4e86\u3002"
	_append_sync_log("\u5fc3\u8df3\u5931\u8d25\uff0c\u7aef\u4fa7\u4f1a\u81ea\u52a8\u7ed3\u675f\u8fdb\u57ce\u72b6\u6001")

func _on_bridge_state_changed(listening: bool, message: String) -> void:
	sync_toggle_button.text = ("\u505c\u6b62\u76d1\u542c" if listening else "\u5f00\u59cb\u76d1\u542c")
	_append_sync_log(message)

func _on_bridge_raw_packet(text: String) -> void:
	_append_sync_log("\u6536\u5230\u5e7f\u64ad: %s" % text)

func _on_bridge_state_packet(data: Dictionary) -> void:
	var mode := String(data.get("m", "\u672a\u77e5"))
	var weather := str(data.get("w", "-"))
	var temperature := str(data.get("t", "-"))
	var humidity := str(data.get("rh", "-"))
	latest_weather_label = _weather_label_from_value(weather, temperature, humidity)
	if not town_sync_active:
		sync_status_label.text = "\u76d1\u542c\u5728\u7ebf"
		sync_state_label.text = "\u8bbe\u5907\u72b6\u6001\uff1amode=%s weather=%s temp=%s rh=%s" % [mode, weather, temperature, humidity]
	stage_view.set_weather_info(
		int(data.get("w", -1)),
		("" if temperature == "-" else "%s\u00b0" % temperature),
		("" if humidity == "-" else "%s%%" % humidity)
	)

func _on_bridge_chat_packet(role: String, text: String) -> void:
	_append_sync_log("\u8bbe\u5907\u804a\u5929: [%s] %s" % [role, text])

func _append_sync_log(text: String) -> void:
	sync_log.append_text("• %s\n" % text)

func _weather_label_from_value(weather: String, temperature: String, humidity: String) -> String:
	var weather_text := weather
	if weather_text == "-" or weather_text.is_empty():
		weather_text = "今天的天气"
	var parts: Array[String] = [weather_text]
	if temperature != "-" and not temperature.is_empty():
		parts.append("%s°" % temperature)
	if humidity != "-" and not humidity.is_empty():
		parts.append("%s%%" % humidity)
	return " ".join(parts)

func _trigger_memory_scene(scene_id: String) -> void:
	var pet := _current_pet()
	if pet == null:
		return
	pet.note_player_interaction()
	var weather_hint := latest_weather_label
	var line := pet.trigger_care_scene(scene_id, weather_hint)
	if not line.is_empty():
		_append_sync_log("%s 对 %s 说：%s" % [scene_id, pet.pet_name, line])
	_refresh_all()

func _on_push_text_submitted(text: String) -> void:
	var cleaned := text.strip_edges()
	if cleaned.is_empty():
		return
	var ok := bridge.send_text(sync_host_input.text, cleaned, false)
	if ok:
		_append_sync_log("\u5df2\u63a8\u9001\u6587\u672c\u5230\u8bbe\u5907")
		sync_push_input.clear()
	else:
		_append_sync_log("\u63a8\u9001\u5931\u8d25\uff0c\u8bf7\u68c0\u67e5\u8bbe\u5907 IP")

func _save_state() -> void:
	var pet_entries: Array[Dictionary] = []
	for pet in cats:
		pet_entries.append(pet.to_core_dict())
		DesktopPersistence.save_pet_memory(pet.pet_id, pet.to_memory_dict())
	var data := {
		"cats": pet_entries,
		"active_pet_id": active_pet_id,
		"current_device_pet_id": current_device_pet_id,
		"selected_return_pet_id": selected_return_pet_id,
		"sync_host": sync_host_input.text if sync_host_input != null else "",
		"care_settings": care_settings.duplicate(true),
	}
	DesktopPersistence.save_data(data)

func _load_state() -> void:
	var data: Dictionary = DesktopPersistence.load_data()
	if data.is_empty():
		return
	cats.clear()
	for row: Variant in data.get("cats", []):
		if row is Dictionary:
			var pet := PetState.new()
			var row_dict := row as Dictionary
			if row_dict.has("memory_tags") or row_dict.has("memory_notes") or row_dict.has("events") or row_dict.has("chat_entries"):
				pet.load_from_dict(row_dict)
			else:
				pet.load_core_dict(row_dict)
				var memory_data := DesktopPersistence.load_pet_memory(pet.pet_id)
				if not memory_data.is_empty():
					pet.load_memory_dict(memory_data)
			DesktopPersistence.ensure_pet_memory_file(pet.pet_id, pet.to_memory_dict())
			cats.append(pet)
	active_pet_id = String(data.get("active_pet_id", active_pet_id))
	current_device_pet_id = String(data.get("current_device_pet_id", current_device_pet_id))
	selected_return_pet_id = String(data.get("selected_return_pet_id", selected_return_pet_id))
	saved_sync_host = String(data.get("sync_host", saved_sync_host))
	var loaded_care: Variant = data.get("care_settings", {})
	if loaded_care is Dictionary:
		care_settings = DEFAULT_CARE_SETTINGS.duplicate(true)
		for key: Variant in (loaded_care as Dictionary).keys():
			care_settings[String(key)] = (loaded_care as Dictionary)[key]
