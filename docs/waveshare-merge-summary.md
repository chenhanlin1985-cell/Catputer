# Waveshare 端开发进度与合并总结

日期：2026-04-12

本文用于下一步把当前 Waveshare ESP32-S3 Touch AMOLED 1.8 端和原 Cardputer 项目合并时对账。它记录当前已经完成的功能、关键文件、排障过程中的根因与解决方案，以及合并时需要特别注意的风险点。

## 当前结论

Waveshare 端已经从“硬件适配和排障阶段”进入“可与原项目合并的功能端口阶段”。目前已验证通过的关键能力包括：

- 屏幕稳定显示，主界面走 LVGL 局部刷新，不再沿用 Cardputer 的整屏 Canvas 重绘路线。
- 中文显示问题已解决，当前使用生成的 LVGL CJK 字体。
- 触摸、左右/上下滑动、BOOT/PWR 两个实体键均已接入当前交互。
- 宠物主页、天气/亲密度顶部信息、数值状态、动作菜单、快捷面板等基础 UI 已完成并经多轮调优。
- 外出、纪念品、照片纪念卡片、多卡翻页等功能已经恢复。
- SD 卡已从旧 SPI 路线切换为 Waveshare 可用的 SD_MMC 路线，用户已验证 SD 相关功能可用。
- ES8311 + I2S 扬声器链路已接通，音量调节可用，用户已验证有声音且音量功能正常。
- 本地 ESP-SR TTS 已接入并刷入 voice_data，经过 PSRAM 配置修复后用户已验证可以发声。

仍未完成或仍需谨慎处理的部分：

- 语音转文字 STT 链路尚未完整恢复。
- Waveshare 麦克风录音仍需要从同步读取改造成不会卡住 UI 的状态机或后台任务。
- 聊天页、AI 对话流、记忆摘要等能力还没有完整迁到 Waveshare 的 LVGL 页面体系中。
- 当前 `PWR` 长按暂时作为 TTS 测试入口，后续应改回“按住说话、松开转写”的语音入口。

## 主要新增与改动文件

Waveshare 端核心入口：

- `src/main_waveshare_lvgl.cpp`
- `src/waveshare_device.cpp`
- `src/waveshare_device.h`
- `src/waveshare_es8311_bridge.c`

诊断与参考固件：

- `src/waveshare_diag.cpp`
- `src/waveshare_baseline.cpp`
- `src/waveshare_lvgl_probe.cpp`

字体与图片资源：

- `src/lv_font_cat_cn_16.c`
- `tools/generate_lvgl_font.py`
- `tools/lvgl_font_chars.txt`
- `src/waveshare_photo_assets.cpp`
- `src/waveshare_photo_assets.h`
- `tools/generate_waveshare_photo_assets.mjs`

本地 TTS：

- `src/local_tts.cpp`
- `src/local_tts.h`
- `src/tts_playback.cpp`
- `src/tts_playback.h`
- `tools/local_tts/`
- `scripts/windows/flash-local-tts-voice.ps1`
- `partitions_catputer_local_tts.csv`

存储和业务复用：

- `src/pet_storage.cpp`
- `src/pet_storage.h`
- `src/companion.cpp`
- `src/companion.h`
- `src/config.cpp`
- `src/config.h`

构建配置：

- `platformio.ini`

## 构建与刷机状态

当前 Waveshare 主环境：

```ini
[env:waveshare-amoled-18]
board = esp32-s3-devkitc1-n16r8
board_build.partitions = partitions_catputer_local_tts.csv
board_build.flash_size = 16MB
board_build.arduino.memory_type = qio_opi
board_build.psram_type = opi
```

关键原因：

- Waveshare 实机是 16MB Flash + 8MB PSRAM。
- 原先使用 `esp32-s3-devkitc-1` 时，PlatformIO 板卡定义是“8MB Flash、无 PSRAM”。
- 虽然 esptool 能探测到硬件有 PSRAM，但固件没有按 OPI PSRAM 配置构建，`MALLOC_CAP_SPIRAM` 分配会失败。
- 改成 `esp32-s3-devkitc1-n16r8` 后，PlatformIO 输出已确认是 `16 MB Flash Quad, 8 MB PSRAM Octal`。

主构建命令：

```powershell
pio run -e waveshare-amoled-18
```

刷机时需要同时刷固件和本地 TTS voice_data：

```powershell
$env:PYTHONIOENCODING='utf-8'; & 'C:\Users\limch\.platformio\penv\Scripts\python.exe' '.platformio-packages\tool-esptoolpy\esptool.py' --chip esp32s3 --port COM6 --baud 460800 --before default_reset --after hard_reset write_flash -z --flash_mode dio --flash_freq 80m --flash_size 16MB 0x0 '.pio\build\waveshare-amoled-18\bootloader.bin' 0x8000 '.pio\build\waveshare-amoled-18\partitions.bin' 0xe000 '.platformio-packages\framework-arduinoespressif32\tools\partitions\boot_app0.bin' 0x10000 '.pio\build\waveshare-amoled-18\firmware.bin' 0x290000 'tools\local_tts\voice_data\esp_tts_voice_data_xiaoxin.dat'
```

注意：

- 普通 `pio run -e waveshare-amoled-18 -t upload` 之前多次在 esptool 阶段卡住；目前直接调用 esptool 更稳定。
- `voice_data` 分区偏移是 `0x290000`，来自 `partitions_catputer_local_tts.csv`。

## 显示与 UI 适配

最初问题：

- 屏幕闪烁严重。
- 触摸不稳定。
- 直接复用 Cardputer 的整屏 Canvas 更新模型会在 AMOLED 上表现很差。

排查过程：

- 先用 `waveshare_baseline.cpp` 验证 SH8601 显示和 FT3168 触摸硬件本身正常。
- 再用 `waveshare_lvgl_probe.cpp` 验证 LVGL + 局部刷新稳定。
- 最终建立 `main_waveshare_lvgl.cpp` 作为 Waveshare 专用主入口。

最终方案：

- 显示层使用 LVGL。
- `flush_cb` 只刷新 LVGL 提供的 dirty area。
- 主界面不回退到旧 `main.cpp` 的整屏重绘路径。
- 中央宠物、天气背景、状态面板、动作抽屉、快捷面板、纪念卡片都在 LVGL 对象体系下实现。

合并建议：

- 保留 `main.cpp` 作为 Cardputer 主入口。
- 保留 `main_waveshare_lvgl.cpp` 作为 Waveshare 主入口。
- 业务逻辑继续下沉到 `Companion`、`Config`、`PetStorage`、`TTSPlayback` 等共享模块。
- 不建议把 Cardputer 的 Canvas UI 和 Waveshare 的 LVGL UI 硬合并成一个文件。

## 中文字体问题

问题现象：

- 早期中文大量显示成方框或缺字。
- 单纯替换上层文案或局部转义不能真正解决，因为根因是 LVGL 字体字形覆盖不足。

解决方案：

- 参考 Cardputer 端已经解决过的字体思路，改为生成 Waveshare/LVGL 可用的 CJK 字体。
- 使用 `tools/lvgl_font_chars.txt` 维护需要覆盖的字符集合。
- 使用 `tools/generate_lvgl_font.py` 生成 `src/lv_font_cat_cn_16.c`。
- `main_waveshare_lvgl.cpp` 中使用生成字体作为中文 UI 字体。

合并注意：

- PowerShell 默认 `Get-Content` 可能会把 UTF-8 中文显示成乱码，不要因此误改源文件。
- 如果需要读中文文本，使用 `Get-Content -Encoding UTF8`。
- 后续新增中文文案时，需要同步更新 `tools/lvgl_font_chars.txt` 并重新生成字体。

## 触摸与按键交互

当前交互模型：

- 左右滑：切换动作/纪念等上下文页面。
- 上滑：打开动作菜单。
- 下滑：打开快捷面板。
- BOOT：返回主页或关闭浮层。
- PWR 短按：打开/切换动作抽屉。
- PWR 长按：当前临时用于 TTS/语音链路测试，后续应接入“按住说话”。

过程中解决的问题：

- 早期触摸滑动完全不生效，后续通过 LVGL 输入回调和根对象 gesture bubble 调整解决。
- 右上角冗余按钮已去掉，天气和亲密度改为顶部芯片。
- 底部数值条太占空间，改为 `90/100` 形式，一行显示两个。
- 上滑动作说明曾挡住数值条，后移到更合理位置。
- 动作和功能菜单曾显示在下方且看不清，改为屏幕中间区域。
- 数值框数字曾靠下/裁切，改用更适合数字的字体和布局修正。

当前建议：

- 合并时把交互“语义”保留，把 UI “呈现”按设备分开。
- Cardputer 继续键盘优先。
- Waveshare 继续手表式触摸 + 两个实体键。

## 宠物状态、外出和纪念卡片

已完成：

- 宠物状态重叠问题已处理，避免同时显示“外出”和“休息中”等冲突状态。
- `外出` 功能已恢复。
- 外出返回后会显示纪念卡片。
- 多条纪念卡片支持翻页。
- 纪念卡片支持图片显示。

过程中的问题：

- 最初只显示一行文字，不显示图片。
- 后续接入固件内嵌照片资源后仍只显示文字，最终修复了图片资源选择与展示逻辑。
- 早期为了避开 SD/触摸冲突，照片先走内嵌资源；后来 SD_MMC 恢复后，存储后端进一步统一。

当前实现要点：

- `Companion` 仍负责外出、纪念品生成和宠物业务逻辑。
- `main_waveshare_lvgl.cpp` 负责在 Waveshare 屏幕上显示居中的纪念卡片。
- `waveshare_photo_assets.*` 提供 Waveshare 端可直接使用的图片资源。
- `PetStorage::fs()` 提供统一文件系统入口，Waveshare 用 `SD_MMC`，Cardputer 用 `SD`。

## SD 卡

早期根因：

- Cardputer 旧 SPI SD 路线使用 `GPIO14`。
- Waveshare 触摸 I2C 的 `SCL` 也在 `GPIO14`。
- 直接复用旧 SD SPI 初始化会破坏触摸坐标读取。

最终方案：

- Waveshare 不复用旧 Cardputer SPI SD 引脚。
- Waveshare 改为官方示例中的 SD_MMC 路线。
- `PetStorage` 中通过条件编译选择后端：
  - Waveshare：`SD_MMC`
  - Cardputer：`SD`
- `PetStorage::fs()` 暴露统一文件系统接口，上层不直接写死 `SD`。

用户验证：

- SD 卡相关功能已验证可用。

合并注意：

- 合并时必须保留 Cardputer 与 Waveshare 的存储后端差异。
- 不要把 Waveshare 改回旧 SPI SD 引脚，否则会复发触摸异常。
- 任何直接 `SD.open(...)` 的旧代码都应逐步改为 `PetStorage::fs().open(...)`。

## 声音、音量与 ES8311

已完成：

- Waveshare 的 `M5.Speaker` 不再是空实现。
- 通过 `ESP_I2S` 和厂商 ES8311 驱动接入扬声器。
- 已实现 `begin()`、`setVolume()`、`tone()`、`playRaw()`、`isPlaying()`、`stop()`。
- 使用 `src/waveshare_es8311_bridge.c` 只编译厂商 `es8311.c`，避免 PlatformIO 扫描整个 vendor 示例目录。
- 音量默认限制到较安全的范围，快捷面板支持调音量。

用户验证：

- 有声音。
- 音量功能可用。

关键硬件点：

- 功放使能：`PA = GPIO46`
- I2S：`MCLK=16`、`BCLK=9`、`WS=45`、`DOUT=8`、`DIN=10`
- ES8311 I2C 地址：`0x18`

合并注意：

- Cardputer 的音频实现和 Waveshare 的 ES8311 实现应保持设备分支。
- 当前 Waveshare `playRaw()` 是阻塞式写入，短句 TTS 可以接受；后续长 AI 回复朗读建议改成后台任务或分块播放，避免 UI 卡顿。

## 本地 TTS

目标：

- 先验证 Waveshare 端能否复用原项目本地 TTS 能力，为后续聊天朗读和离线陪伴打基础。

已完成：

- `platformio.ini` 中 Waveshare 环境启用 `CATPUTER_ENABLE_LOCAL_ESP_TTS=1`。
- 链接 ESP-SR / ESP-TTS 相关 include 与静态库。
- 使用 `partitions_catputer_local_tts.csv` 预留 `voice_data` 分区。
- `local_tts.cpp` 负责查找、mmap 并初始化 `voice_data`。
- `TTSPlayback` 支持绑定 `LocalTTS`，优先走本地 TTS 合成 PCM。
- `main_waveshare_lvgl.cpp` 中按需申请 TTS PCM 缓冲，播放后释放。
- `PWR` 长按释放触发一条测试语音。

关键问题与解决：

- 最初 `TTS test` 会显示但没有声音，说明按键入口通了，问题在 TTS 服务之后。
- 早期把外部分区 voice_data 用 `esp_tts_voice_xiaole` 初始化，和 Espressif 参考例程不一致；改为 `esp_tts_voice_template` 后与外部分区方案匹配。
- 诊断信息原来显示在底部，容易被动作菜单挡住；新增了屏幕中间 TTS 诊断浮层。
- 长按 PWR 曾同时触发菜单逻辑，干扰测试；现在长按测试会先关闭动作菜单、快捷面板和纪念面板。
- 后续出现 `buffer allocation failed`，根因是 PlatformIO 使用了“无 PSRAM”的板卡定义；改为 `esp32-s3-devkitc1-n16r8` 并设置 `qio_opi` / `opi` 后解决。

用户验证：

- 当前本地 TTS 已经有声音。

合并注意：

- `voice_data` 文件需要刷到 `0x290000`。
- 本地 TTS 依赖 PSRAM 配置正确，否则大块 PCM 缓冲申请会失败。
- `LocalTTS`、`TTSPlayback` 是共享能力，但播放硬件仍需走设备适配层。
- 目前 TTS 是“已验证能发声”，还不是完整聊天朗读体验；长回复播放和打断还需要继续做。

## STT 与麦克风

当前状态：

- STT 链路还没有完整接回 Waveshare。
- Waveshare 麦克风录音仍是下一阶段重点。

为什么还不能直接照搬：

- 厂商录音示例多是同步 `readBytes()` 循环。
- 当前 UI 是 LVGL 主循环，直接同步录音会卡住触摸和界面刷新。
- 原项目 `VoiceInput` 更适合被改造成按住录音、松开提交的异步或后台任务模型。

建议下一步：

- 先实现 Waveshare `Mic.record()` 的非阻塞状态机或后台任务。
- PWR 长按开始录音，松开结束。
- 录音期间显示中间或顶部语音状态，不再复用 TTS test 文案。
- 录音结束后走现有 STT proxy，再送入聊天页。
- 语音输入和本地 TTS 共享大缓冲时，必须保证不会同时录音和播放。

## 合并建议

建议按以下顺序合并，降低冲突风险：

1. 先合并构建配置和设备边界：`platformio.ini`、`waveshare_device.*`、`main_waveshare_lvgl.cpp`、诊断固件。
2. 再合并共享后端能力：`PetStorage::fs()`、SD_MMC 条件分支、`LocalTTS`、`TTSPlayback` 的本地 TTS 支持。
3. 再合并 `Companion` 中与外出、纪念品、照片、状态互斥、语音队列相关的业务变更。
4. 最后再合并 UI 细节和桌面同步等非 Waveshare 主路径改动。

合并时需要特别小心：

- 当前工作树很脏，里面有 Waveshare 迁移，也有桌面端、主 `main.cpp`、聊天、记忆等其它演进，不要用整仓 diff 盲合并。
- Waveshare UI 与 Cardputer UI 应该共享业务，不共享显示入口。
- 所有中文源文件都要按 UTF-8 处理。
- 不要把 `esp32-s3-devkitc1-n16r8` 改回 `esp32-s3-devkitc-1`。
- 不要把 Waveshare SD 后端改回旧 SPI SD 引脚。
- TTS voice_data 分区和刷机步骤必须保留，否则会出现固件能跑但本地 TTS 不可用。

## 建议验收清单

合并后建议逐项验证：

- `pio run -e waveshare-amoled-18` 可以构建通过。
- 启动后屏幕不闪，触摸坐标正常，滑动和两个实体键正常。
- 中文没有大量方框。
- 顶部天气/亲密度显示正常。
- 数值状态 `90/100` 类文本不被裁切。
- 上滑动作菜单、下滑快捷面板、BOOT 返回都可用。
- 外出后能返回并显示纪念卡片。
- 多张纪念卡片可翻页。
- 照片纪念卡片能显示图片。
- SD 卡状态可见，插卡后文件后端可读。
- 音量调节可用，声音不会过大。
- 长按 PWR 的 TTS 测试可以发声。
- 若后续接入 STT，长按录音时 UI 不应卡死。

## 下一步开发建议

短期优先级：

1. 把 `PWR` 长按从 TTS test 改成真正的语音输入状态机。
2. 实现 Waveshare 麦克风录音，不阻塞 LVGL 主循环。
3. 把 STT proxy 转写结果接入聊天输入。
4. 做 Waveshare 聊天页 MVP，先用快捷回复和语音输入，不急着做全屏中文键盘。
5. 把本地 TTS 从测试入口改成 AI 回复朗读入口，并支持打断。

中期优先级：

1. 梳理 `Companion` 中当前混合的 Cardputer/Waveshare 条件编译，尽量把设备差异下沉到适配层。
2. 清理早期诊断代码和临时 TTS 状态浮层，只保留必要的错误提示。
3. 更新旧文档中已经过期的“SD 暂不恢复”“中文仍缺字”等结论。
4. 给 SD、音频、TTS、触摸补一套最小诊断流程，方便以后换机器或重新刷机时快速定位。

## 一句话总结

当前 Waveshare 端已经完成显示、中文、交互、外出纪念、SD、音频、本地 TTS 这些关键底座。下一阶段的主线不再是硬件排障，而是把原项目的聊天、记忆、STT 业务按 Waveshare 的 LVGL + 触摸交互模型合并回来。
