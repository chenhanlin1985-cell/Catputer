# Waveshare 触屏端合并与冒烟总结

日期：2026-04-12

本文记录把另一台电脑上开发的 Waveshare ESP32-S3 Touch AMOLED 1.8 端侧代码合并回当前 Catputer 主仓后的过程、关键结论和冒烟结果。目标是让后续继续开发、换电脑恢复环境、重新刷机时，不再重复踩同一批坑。

## 当前结论

这轮合并已经完成一次最小冒烟：触屏端代码可以在当前仓库编译，设备可以通过专用脚本刷入，固件和本地 TTS 语音资源都能写入。PWR 触发本地 TTS 的测试链路已验证可用，并且已清理成功播放时残留的英文诊断文案。

这次合并不是把两个项目硬拼成一个 `main`，而是保留双端结构：

- Cardputer 端继续使用原来的 `src/main.cpp`。
- Waveshare 触屏端使用独立入口 `src/main_waveshare_lvgl.cpp`。
- 共享业务能力继续放在 `Companion`、`PetStorage`、`Config`、`TTSPlayback`、`LocalTTS`、`AIClient` 等模块里。
- 设备差异通过条件编译和适配层处理，不把键盘端 UI 和触摸端 UI 混在一起。

## 合并范围

主要新增或接入的触屏端内容：

- `src/main_waveshare_lvgl.cpp`：Waveshare LVGL 主入口。
- `src/waveshare_device.*`：Waveshare 设备适配。
- `src/waveshare_es8311_bridge.c`：ES8311 音频桥接。
- `src/lv_font_cat_cn_16.c`：触屏端中文字体。
- `include/lv_conf.h`：LVGL 配置。
- `lib/waveshare/`：Waveshare 相关厂商库。
- `_vendor/`：必要的厂商参考实现和音频驱动来源。
- `tools/local_tts/`：本地 ESP-SR TTS 库与 voice data 资源。
- `partitions_catputer_local_tts.csv`：给本地 TTS 语音包预留分区。
- `scripts/windows/flash-waveshare-amoled.ps1`：触屏端专用刷写脚本。

主要改动的共享能力：

- `PetStorage`：支持 Cardputer 使用 `SD`，Waveshare 使用 `SD_MMC`。
- `TTSPlayback` / `LocalTTS`：触屏端复用本地 TTS 合成与播放链路。
- `Companion`：支持待播报语音队列。
- `Config`：保留本地 TTS 和自动播报相关配置。
- `utils.h`：按不同设备尺寸区分屏幕参数。
- `platformio.ini`：增加 `waveshare-amoled-18` 及诊断环境。

## 构建配置

触屏端主环境为：

```ini
[env:waveshare-amoled-18]
platform = https://github.com/pioarduino/platform-espressif32/releases/download/stable/platform-espressif32.zip
board = esp32-s3-devkitc1-n16r8
framework = arduino
board_build.partitions = partitions_catputer_local_tts.csv
board_build.flash_size = 16MB
board_build.arduino.memory_type = qio_opi
board_build.psram_type = opi
```

这里不能随手改回普通 `esp32-s3-devkitc-1`。触屏设备实际是 16MB Flash + 8MB OPI PSRAM，本地 TTS 需要正确的 PSRAM 配置，否则大块 PCM 缓冲会分配失败。

编译命令：

```powershell
python -m platformio run -e waveshare-amoled-18
```

## 刷写流程

这次确认了一个关键点：设备不需要手动进入下载模式，但普通 `platformio upload` 在当前机器上不能稳定把它切到可刷写状态。表现是 `No serial data received`。

稳定流程是：

1. 运行态设备通常枚举为 `COM8`。
2. 用 1200bps touch 触发设备重枚举。
3. 设备进入 bootloader 后通常枚举为 `COM9`。
4. 直接调用 PlatformIO 自带 `esptool.py`，同时写入 bootloader、partition、boot_app0、firmware 和本地 TTS voice data。

后续推荐直接使用脚本：

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File scripts\windows\flash-waveshare-amoled.ps1
```

脚本会做这些事：

- 自动编译 `waveshare-amoled-18`。
- 自动查找当前 `VID_303A&PID_1001` 串口。
- 用 1200bps touch 触发 bootloader。
- 等待设备从运行态端口重新枚举到 bootloader 端口。
- 调用 `esptool.py` 写入固件和 `0x290000` 的 TTS 语音包。

如果已经编译过，只想重刷当前产物：

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File scripts\windows\flash-waveshare-amoled.ps1 -SkipBuild
```

## 本地 TTS 验证

本轮冒烟里，PWR 长按释放仍作为临时本地 TTS 测试入口。已确认：

- voice data 会随脚本刷入 `0x290000`。
- 本地 TTS 可以发声。
- 成功播放时不再显示 `TTS: synthesizing`。
- 成功播放时不再显示 `TTS: playback requested`。

当前仍保留失败提示，例如缓冲分配失败或合成失败时会显示错误。这是有意保留的，因为失败状态对后续排障仍有价值。

## 已发现并解决的问题

普通 PlatformIO 上传失败：

- 现象：`Failed to connect to ESP32-S3: No serial data received.`
- 根因：自动 reset 未把设备稳定切到 bootloader。
- 解决：改用 1200bps touch + 直接 esptool 写入。

COM 口选择不稳定：

- 现象：刷写后设备会从 `COM9` 回到 `COM8`，Windows `Get-PnpDevice` 有时会留下陈旧 `COM9`。
- 解决：脚本改用 `Win32_SerialPort` 查询当前真实存在的 `VID_303A&PID_1001` 端口。

TTS 成功状态污染界面：

- 现象：屏幕显示 `TTS: playback requested`。
- 根因：`serviceLocalTts()` 成功播放后仍写入临时诊断状态。
- 解决：成功路径改为静默，只保留失败提示。

无效上传参数：

- 现象：排查中曾尝试把 `upload_speed = 115200` 加到 `platformio.ini`。
- 结论：这不是根因，已移除，避免后续误导。

## 冒烟结果

已完成：

- `waveshare-amoled-18` 编译通过。
- 使用 `scripts/windows/flash-waveshare-amoled.ps1` 完成实际刷写。
- 固件写入成功。
- 本地 TTS voice data 写入成功。
- 设备刷写后从 bootloader 端口回到运行态端口。
- 已确认源码中不再包含 `TTS: playback requested` / `TTS: synthesizing` 两条成功路径 UI 文案。

仍建议人工确认：

- 设备主界面是否正常显示。
- PWR 临时测试语音是否仍能正常发声。
- 成功发声时界面是否不再出现英文 TTS 状态文本。
- 后续如果恢复语音输入，PWR 测试入口需要被替换成真正的按住说话状态机。

## 后续开发注意事项

不要把 Waveshare 触屏端退回 Cardputer 的整屏 Canvas 绘制路线。触屏端已经选择 LVGL 局部刷新，后续 UI 迭代都应沿着 `main_waveshare_lvgl.cpp` 继续。

不要把 Waveshare 的 SD 存储退回 Cardputer 的旧 SPI SD 引脚。旧引脚会和触摸 I2C 冲突，应该继续使用当前的 `SD_MMC` 分支。

不要把 `board` 改回无 PSRAM 的板卡定义。本地 TTS、图片和 LVGL 都依赖当前 16MB Flash + 8MB OPI PSRAM 配置。

不要在成功路径上显示底层诊断英文。用户界面应只展示宠物语义和必要错误，底层日志放在串口里。

新增中文 UI 文案时，要同步考虑 LVGL 字体字符覆盖，否则会出现方框或缺字。

## USB 串口配置

触屏端不适合在屏幕上输入 WiFi 密码和 API Key，因此当前已接入运行态 USB 串口配置协议。设备正常运行时会枚举为 USB 串口，例如 `COM8`；不要使用刷机 bootloader 临时出现的端口。

端侧支持的 JSON 行命令：

```json
{"cmd":"ping"}
{"cmd":"get_config"}
{"cmd":"set_config","wifi_ssid":"your-ssid","wifi_pass":"your-pass","api_host":"dashscope.aliyuncs.com","api_port":"443","api_key":"your-key","city":"深圳"}
```

推荐使用脚本，不手写 JSON：

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File scripts\windows\configure-device-serial.ps1 -Port COM8
```

脚本会优先读取 `clawputer.local.ps1` 导出的环境变量，也可以用命令行参数覆盖。脚本输出只打印 SSID、Host 和城市，不打印 WiFi 密码或 API Key：

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File scripts\windows\configure-device-serial.ps1 -Port COM8 -WifiSsid "your-ssid" -WifiPass "your-pass" -ApiHost "dashscope.aliyuncs.com" -ApiPort "443" -City "深圳"
```

当前会写入 NVS 的字段包括：

- `wifi_ssid` / `wifi_pass`
- `wifi_ssid2` / `wifi_pass2`
- `api_key`
- `api_host` / `api_port`
- `gateway_token`
- `stt_host` / `stt_port`
- `api_host2`
- `city`
- `auto_speak`
- `local_tts`
- `speaker_volume`

2026-04-12 实机验证结果：

- 运行态端口 `COM8` 可收到 `ping`。
- `set_config` 可写入 NVS。
- `get_config` 可读回配置。
- 中文城市 `深圳` 可通过 UTF-8 串口写入并读回。
- 已验证脚本可从 `clawputer.local.ps1` 读取本机配置并下发到触屏端。

注意：当前仓库同时包含 Cardputer 的 PlatformIO 官方 `espressif32@6.13.0` 环境和 Waveshare 使用的 pioarduino 环境，两者依赖的 `framework-arduinoespressif32` 包版本不同。不要并行构建两个环境；如果来回切换后出现 `FRAMEWORK_DIR` 为 `None` 或 esptool 参数版本不匹配，优先单独重建目标环境，必要时恢复对应平台的 framework 包缓存。

## 下一步建议

短期下一步可以继续做触屏端真实交互，而不是继续停在硬件排障：

- 把 PWR 临时 TTS 测试入口改成“按住说话、松开转写”的语音入口。
- 把聊天页按触屏交互重新设计，不直接搬 Cardputer 键盘逻辑。
- 把本地 TTS 从测试入口接到宠物反馈和 AI 回复朗读。
- 继续把猫舍/多端同步协议抽象成设备无关接口，让 PC 可以管理 Cardputer 和 Waveshare 两类端侧。
