# Waveshare 功能还原方案

## 背景

Waveshare ESP32-S3 Touch AMOLED 1.8 端已经完成第一阶段适配：中文显示、主界面、触摸/滑动、BOOT/PWR 两个实体键都已经可用。下一阶段目标不是继续堆 UI，而是把 Cardputer 端已经存在的核心能力按 Waveshare 的硬件形态重新接回来。

当前需要坚持两条原则：

- 显示层继续走 `main_waveshare_lvgl.cpp` 的 LVGL 局部刷新路线，不回退到 Cardputer 的整屏 Canvas 重绘模型。
- 业务能力优先复用 `Companion`、`Chat`、`AIClient`、`PetStorage`、`VoiceInput`、`TTSPlayback`、`LocalTTS` 等已有模块，但在 Waveshare 端加一层适配，不直接搬键盘交互。

## 当前基线

- 屏幕：368x448 AMOLED，适合手表式卡片、抽屉、滑动分页。
- 输入：触摸屏、BOOT/GPIO0、PWR/EXIO4。
- 音频：双数字麦克风阵列、ES8311 音频链路、本地 TTS 分区规划已经在工程中存在。
- 网络：已有 WiFi、OpenAI-compatible chat completions、天气客户端、TTS/STT 代理链路。
- 存储：当前 Waveshare 端需要避免复用会占用触摸 I2C 引脚的旧 SD SPI 初始化；短期优先使用 NVS，后续再评估 SDMMC 或安全的文件存储路径。

## 信息架构

建议把 Waveshare 端拆成 4 个主页面，保持手表式导航：

- `主页`：宠物、天气、亲密度、核心状态、动作抽屉。
- `聊天`：最近对话、快捷回复、语音输入状态、AI 回复和 TTS 状态。
- `记忆`：最近事件、纪念品、外出回忆、可清理/导出入口。
- `设置`：WiFi、API/TTS/STT、音量、亮度、本地 TTS 状态。

交互约定：

- 左/右滑：主页面切换。
- 上滑：当前页面的动作抽屉。
- 下滑：快捷面板。
- BOOT 短按：返回主页/关闭浮层。
- PWR 短按：打开或切换当前页主动作。
- PWR 长按：语音输入入口，注意不要引导用户长按超过 6 秒，避免触发硬件关机。

## 阶段 1：外出与纪念品

目标：先恢复不依赖网络和音频的宠物业务闭环。

可复用模块：

- `Companion::startOuting()`
- `Companion::tickOuting()`
- `Companion::getSouvenirItem()`
- `Companion::getSouvenirNote()`
- `Config` 中已有纪念品槽位

实现建议：

- 保留主页动作抽屉里的 `外出` 按钮，点击后进入外出状态。
- 主页显示“正在外出 / 预计回来”的简短状态，不需要阻塞 UI。
- 外出完成后显示一个居中的纪念品卡片，提供 `查看`、`收起` 两个触摸动作。
- 纪念品短期写入 NVS 中已有槽位，不先启用旧 SD 路径。
- 后续如果要恢复照片类纪念品，再单独评估 SDMMC 和图片解码，不和第一版外出混在一起。

验收标准：

- 点击外出后宠物状态改变。
- 到点返回后出现纪念品提示。
- 重启后最近纪念品仍能从 NVS 恢复。

## 阶段 2：联网聊天

目标：恢复文字聊天，不先做复杂输入法。

可复用模块：

- `Chat`
- `AIClient`
- `Config`
- `WeatherClient`

实现建议：

- 新增 `聊天页`，左/右滑进入。
- 第一版输入使用快捷回复按钮，例如“你好”“继续说”“今天怎么样”“讲个小故事”。
- AI 回复用流式追加到聊天气泡，屏幕每次只保留最近 3-5 条。
- PWR 短按在聊天页打开快捷回复抽屉；BOOT 返回主页。
- 网络状态放进下滑快捷面板，避免聊天失败时用户不知道是 WiFi 还是 API 问题。

暂不建议第一版就做：

- 全量触屏中文键盘。
- 复杂拼音输入法。
- 大段历史滚动列表。

验收标准：

- WiFi 已连接时可发送快捷回复并得到 AI 回复。
- AI 忙碌时 UI 有“思考中”状态。
- 网络失败时显示明确错误，不让界面像卡死。

## 阶段 3：语音输入与本地 TTS

目标：恢复“按住说话”和“AI 回复朗读”，优先使用现有音频模块。

可复用模块：

- `VoiceInput`
- `TTSPlayback`
- `LocalTTS`
- `Config::getPreferLocalTTS()`

实现建议：

- PWR 长按开始录音，松开结束并进入转写。
- 录音期间显示全屏或半屏语音状态条，避免误以为卡住。
- 优先尝试本地 TTS；本地不可用时回落到 TTS 代理。
- 播放期间任意触摸或 BOOT 短按停止播放。
- 音频缓冲继续按需分配，不在主界面常驻占用大块内存。

硬件注意：

- PWR 是电源相关按键，交互文案应强调“按住说话，松开结束”，不要设计成特别长的按住操作。
- 语音输入和 TTS 播放不要同时占用同一缓冲。

验收标准：

- PWR 长按时出现录音状态。
- 松开后能把识别文本送入聊天。
- AI 回复结束后可朗读，且可被触摸/按键打断。

## 阶段 4：记忆功能

目标：让宠物状态、聊天摘要、外出事件可持续沉淀。

实现建议：

- 第一阶段使用 NVS 保存轻量记忆：亲密度、纪念品、最近几条摘要。
- 聊天完成后生成短摘要，写入宠物事件日志。
- 外出返回、喂食、清洁、玩耍等动作写入最近事件。
- 记忆页展示最近事件，不在主页塞太多文本。

存储路线：

- 短期：NVS，稳定优先。
- 中期：评估 SDMMC，前提是不再占用触摸 I2C 的 GPIO14/GPIO15。
- 长期：如果需要图片、长日志、离线资源，再做文件系统方案。

验收标准：

- 重启后基础记忆存在。
- 聊天和外出可以影响后续状态文案。
- 存储失败时不影响主 UI 和触摸。

## 阶段 5：结合 Waveshare 硬件的新能力

可以考虑的增强：

- 快捷面板加入亮度调节，因为 AMOLED 手表形态对亮度很敏感。
- 加入“抬腕/运动”类互动，如果后续确认并接入板载 IMU。
- 利用双麦克风做更明确的语音入口，而不是把语音藏在聊天页里。
- 用 AMOLED 做夜间低亮度表盘，减少常亮面积。
- 如果电源管理读数稳定，顶部或快捷面板显示电量/充电状态。
- 后续可做“离线陪伴模式”：没网时仍可外出、记忆、本地 TTS 和状态互动。

## 优先级

建议下一步顺序：

1. 修正状态数值垂直对齐和剩余小 UI polish。
2. 恢复外出与纪念品，先不碰网络/音频。
3. 做聊天页的快捷回复 MVP。
4. 接入 PWR 长按语音输入。
5. 接入 TTSPlayback 和 LocalTTS。
6. 梳理记忆存储，先 NVS，后文件系统。

## 风险点

- 旧 `main.cpp` 里很多逻辑和 Cardputer 键盘/Canvas 强耦合，不能直接迁移到 LVGL 页面。
- SD/文件存储要避开触摸 I2C 引脚冲突，否则会复发触摸坐标异常。
- 语音和 TTS 会占用较大内存，必须继续按需申请/释放。
- PWR 长按存在硬件关机语义，不能把长按时间设计得太长。
- 触屏中文输入法成本较高，第一版聊天建议用快捷回复和语音输入绕开。

## 实施记录

- 2026-04-12：完成状态数值区域的字体度量修正。中文标签继续使用 CJK 字体，纯数字 `90/100` 改用 LVGL 默认数字字体，避免生成中文字体的行高/基线让数字视觉下沉；详情行去掉重复的亲密度，亲密度只保留在顶部芯片。
- 2026-04-12：阶段 1 外出与纪念品已接入 Waveshare UI 适配层。`外出` 继续调用 `Companion::startOuting()`，外出返回或点击 `纪念` 时显示居中的纪念品卡片，卡片可点按收起并会自动隐藏；业务、随机纪念品和 NVS 槽位仍由 `Companion`/`Config` 负责。
- 2026-04-12：纪念品照片在 Waveshare 端先改成固件内嵌资源。`sdcard/pet/photos/*.png` 会通过 `tools/generate_waveshare_photo_assets.mjs` 转成 LVGL `lv_img_dsc_t`，运行时不读取旧 SD 路径，避免重新占用触摸 I2C 使用的 GPIO14。
- 2026-04-12：修正 Waveshare 端照片纪念品生成条件。原逻辑要求 `PetStorage::isAvailable()` 才可能生成 `photo:` 纪念品，但 Waveshare 当前故意不启用旧 SD，所以只会生成文字纪念品；现改为 Waveshare 使用内嵌照片资源即可生成 `photo:`。为便于实机验收，当前 Waveshare 外出暂时必带照片，确认显示正常后可把概率调回自然值。
- 待设备验收：确认数值框内数字不再贴近下边缘或被裁切；确认外出 45-90 秒返回后会弹出纪念品卡片；确认重启后 `纪念` 入口仍能打开最近纪念品。

## SD 卡恢复路线

旧 SD SPI 初始化当前不能直接在 Waveshare 端恢复，因为 `pet_storage.cpp` 里旧引脚定义为 `SCK=40, MISO=39, MOSI=14, CS=12`，其中 `GPIO14` 已经是 Waveshare 触摸 I2C 的 `SCL`。如果直接启用，会复发触摸坐标异常或触摸完全失效。

建议按三步恢复：

1. 先把 `PetStorage` 抽象成后端接口：`NVS`、`EmbeddedAssets`、`SD` 三种来源共用同一套读取 API，UI 不直接关心文件系统。
2. 查 Waveshare AMOLED 1.8 的真实可用存储总线：优先确认是否有独立 SDMMC 或不冲突 SPI 引脚；如果没有，就不要复用 `GPIO14`，改走 USB 串口同步、LittleFS/SPIFFS、外置 Flash 或固件内嵌资源。
3. 做一个单独的 SD 诊断固件环境，只测试卡初始化、文件列表、触摸坐标稳定性。验收通过后再把 SD 后端接回主固件，避免把存储实验混进主 UI。

## 2026-04-12 SD/音频恢复进展

SD 已从旧 Cardputer SPI 初始化切换为 Waveshare 官方 Arduino 示例里的 `SD_MMC` 路线。Waveshare 端启动时会配置扩展 IO 的 `EXIO7` 为输出高电平，并在 `PetStorage::begin()` 内使用 `SD_MMC.setPins(SDMMC_CLK, SDMMC_CMD, SDMMC_DATA)` 与 `SD_MMC.begin("/sdcard", true)` 挂载卡；Cardputer 端仍保持原来的 SPI SD 初始化不变。

`PetStorage` 现在暴露 `PetStorage::fs()`，输入法词库、对话词库、串口 SD 同步、旧 Canvas 照片读取都改为走统一文件系统后端，不再直接写死 `SD` 对象。这样 Waveshare 使用 `SD_MMC`、Cardputer 使用 `SD` 时，上层业务逻辑可以共享同一套读写入口。

声音已完成第一版底层接入：Waveshare 的 `M5.Speaker` 不再是空实现，而是使用厂商 Arduino 示例中的 ES8311 驱动和 Arduino core 的 `ESP_I2S`。当前已实现 `Speaker.begin()`、`setVolume()`、`tone()`、`playRaw()`、`isPlaying()`、`stop()`，可覆盖按键提示音和本地 TTS PCM 播放链路。为避免 PlatformIO 扫描整个示例目录，项目通过 `src/waveshare_es8311_bridge.c` 只编译厂商 `es8311.c`。

音频剩余工作：`Mic.record()` 仍保持安全 stub。厂商示例里的录音是同步 `readBytes()` 循环，而本项目 `VoiceInput` 当前假定录音过程近似异步；如果直接照搬会在 PWR 按住录音时卡住 UI。下一步应把 Waveshare Mic 实现成状态机或后台任务，再接入 PWR 按住录音、STT 和播放打断。

待实机验收：
- 启动串口应出现 `[SD] ready, size=...` 或明确的 `[SD] SD_MMC init failed` / `[SD] no card`。
- 插卡时 `/pet/souvenirs.txt`、`/pet/memory/*.txt`、`/pet/dialogue/*.txt`、`/pet/ime/*.txt` 应能被统一后端读取。
- 开机或触发按键音/本地 TTS 时应出现 `[AUDIO] es8311 speaker ready`，并能听到提示音或朗读。
- 如果听不到声音，优先检查 `PA=GPIO46` 功放使能、I2S 引脚 `MCLK=16/BCLK=9/WS=45/DOUT=8/DIN=10`、ES8311 I2C 地址 `0x18`。
