# Catputer

[English](README.md)

`Catputer` 是一个围绕 ESP32-S3 触屏端、PC 猫舍和 SD 卡资源构建的随身宠物项目。当前方向已经不再是“一个小聊天终端”，而是让宠物像在自己的小房间里真实生活：会移动、会记得最近的小事、会根据时间和天气说短句，联网时再用 AI 生成更自然的场景化反馈。

当前主线端侧是 `Waveshare ESP32-S3 Touch AMOLED 1.8`。早期的 `M5Stack Cardputer / Cardputer Adv` 版本仍保留在仓库中，但已经进入维护归档状态，短期不再作为宠物表现功能的开发主线。

## 当前结构

- **Waveshare 触屏端**：当前主端侧，负责宠物房间、触摸交互、天气/时间、SD 资源、SD OTA 和关注回合气泡。
- **PC 猫舍**：Godot 桌面端，用于管理多只宠物、查看记忆、选择端侧、接回/送出宠物，未来也适合承担 AI 世界事件生成。
- **Cardputer 端**：保留已有键盘形态实验能力，但短期只维护，不再主动新增宠物表现功能。

## 有趣的点

Catputer 想营造的是“宠物不被盯着的时候，也在自己的世界里生活”的感觉：

- 宠物不是静态图标，而是在小房间里活动。
- 用户隔一段时间再触摸宠物，会触发重逢和最近小事。
- 天气、时间段、宠物状态、记忆和纪念品可以影响短句。
- AI 不只是聊天机器人，而是用来生成“宠物刚才做了什么”的小世界事件。
- 没有网络时仍有本地短句回退，宠物不会直接沉默。

## 主要功能

- Waveshare AMOLED 触屏宠物主页。
- 宠物基础属性：饱腹、心情、体力、清洁、亲密。
- 多套宠物形象资源，包括猫和企鹅。
- SD 卡资源：对话、背景、照片、输入法资源、OTA 固件。
- 根据时间切换房间背景，天气和时间支持缓存显示。
- 关注回合：重逢短句 + 最近小事/记忆上下文。
- 联网可用时，触摸宠物会请求 AI 生成 2 到 3 句场景化气泡。
- PC 猫舍支持多宠物管理和端侧迁移。
- SD OTA：把固件放进 SD 卡后，可以从端侧设置页触发刷机。

## 配置

不要提交真实 Wi-Fi、API Key 或个人 token。使用示例文件创建本地配置：

- `.env.example`
- `clawputer.local.ps1.example`

常见配置项：

- `WIFI_SSID`
- `WIFI_PASS`
- `API_KEY`
- `API_HOST`
- `API_PORT`
- `DEFAULT_CITY`

## 编译

默认开发目标已经切到 Waveshare 触屏端：

```powershell
powershell -ExecutionPolicy Bypass -File scripts\windows\pio-run.ps1
```

显式编译触屏端：

```powershell
powershell -ExecutionPolicy Bypass -File scripts\windows\pio-run.ps1 -EnvName waveshare-amoled-18
```

如果需要验证旧 Cardputer 端：

```powershell
powershell -ExecutionPolicy Bypass -File scripts\windows\pio-run.ps1 -EnvName m5stack-cardputer
```

## Waveshare 串口刷机

使用已固定的 COM9 等待流程，不要直接把运行态 `COM8` 当刷写端口：

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File scripts\windows\flash-waveshare-com9-wait.ps1 -AcknowledgeChecklist -WaitSeconds 35 -TouchAttempts 3
```

排障前先看：

- [Waveshare 刷机必读](docs/flash-must-read.md)
- [刷机与联网排查 SOP](docs/刷机与联网排查SOP.md)

## SD OTA

生成 OTA 固件包：

```powershell
powershell -ExecutionPolicy Bypass -File scripts\windows\prepare-sd-ota.ps1
```

复制到真实 SD 卡：

```text
/firmware/update.bin
```

然后在触屏端设置页触发 SD 刷机。

## 推荐阅读

- [当前功能、设置与注意事项](docs/current-product-state-and-setup.md)
- [触屏端优先与 AI 世界感路线](docs/touch-first-ai-world-roadmap.md)
- [关注回合与情绪价值设计](docs/attention-session-emotional-design.md)
- [PlatformIO 缓存隔离说明](docs/platformio-cache-isolation.md)
- [BLE 与触屏排障记录](docs/ble-keyboard-troubleshooting.md)

## 仓库说明

- 不要提交真实 Wi-Fi、API Key 或其他个人配置。
- 不要提交生成固件、日志和本地临时文件。
- 每次新增功能都应该同步更新对应文档。
- Waveshare 本地 TTS 暂时不要恢复，除非先完成分区和内存隔离方案。
