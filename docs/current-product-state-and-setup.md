# Catputer 当前功能、设置与注意事项

这份文档用于把当前项目收敛成一套可复制的说明。目标不是记录每一次调试细节，而是让下一台电脑、下一块端侧设备、下一次迭代都能从稳定状态继续。

## 1. 当前产品形态

Catputer 现在由三部分组成：

- **Cardputer 端侧**：键盘形态的随身宠物设备，支持离线养成、中文输入、聊天、BLE 键盘等能力。
- **Waveshare 触屏端侧**：触摸屏形态的随身宠物设备，当前重点是沉浸式主页、触摸交互、天气/时间、SD 资源与 SD OTA。
- **PC 猫舍**：Godot 桌面端，用于管理多只宠物、查看状态/记忆，并把宠物送往不同端侧或从端侧接回。

当前核心思路是：端侧负责“陪伴感”和即时反馈，PC 端负责“管理”和更大屏的信息展示，SD 卡负责可替换资源与离线更新。

## 2. 已完成的主要功能

### 通用宠物功能

- 宠物有饱腹、心情、体力、清洁、亲密等基础数值。
- 支持喂食、玩乐、休息/睡觉、清理等基础照料行为。
- 支持多套形象资源，包括橘猫、紫猫、Q 版企鹅。
- 支持宠物记忆、纪念盒、照片/小纸条等轻量陪伴内容。
- 对话和提示优先中文显示。

### PC 猫舍

- 桌面端使用 Godot 项目：`desktop/CatputerGodot`。
- 支持猫屋/猫仓、宠物状态查看、不同宠物形象管理。
- 支持把宠物送到端侧、从端侧接回，并记录当前宠物所在位置。
- PC 端适合承担后续多宠物管理、记忆查看、端侧配置入口等功能。

### Cardputer 端侧

- 支持离线基础宠物循环。
- 支持中文输入法词库，词库可放到 SD 卡。
- 支持联网聊天、语音输入链路、BLE 键盘模式。
- 支持本地 TTS 实验链路，但这部分对内存和分区敏感，后续不要随意扩展。

### Waveshare 触屏端侧

- 使用 `waveshare-amoled-18` PlatformIO 环境。
- 主界面使用宠物之家背景，宠物会在房间内移动。
- 背景资源放在 SD 卡，当前使用 `184x224 RGB565`，运行时 2x 放大到 `368x448`。
- 背景可根据时间切换为早上、中午、傍晚、夜晚。
- 天气和时间会显示在主界面，并在离线时尽量使用缓存。
- 下滑显示信息面板；在信息面板按 `PWR` 进入设置页。
- 设置页提供大按钮，避免在信息面板里堆叠小按钮。
- 支持 AP/Web 配网入口，当前 BLE 配网默认关闭以避免内存/崩溃风险。
- 支持 SD OTA：把固件放到 SD 卡后，可在端侧设置页触发刷机。

## 3. 配置方法

### 本地配置

不要提交真实 Wi-Fi、API Key 或个人 token。使用示例文件创建本地配置：

- `.env.example`
- `clawputer.local.ps1.example`

常用配置项：

- `WIFI_SSID`
- `WIFI_PASS`
- `API_KEY`
- `API_HOST`
- `API_PORT`
- `DEFAULT_CITY`

当前城市默认按深圳使用；如果换城市，优先改配置，不要硬编码到业务逻辑里。

### Waveshare 触屏端联网

触屏端不再推荐开机强制联网。联网应该按需触发，并在离线时使用缓存，避免 WiFi 初始化挤占内部 RAM。

如果需要重新配网：

1. 下滑打开信息面板。
2. 按 `PWR` 进入设置页。
3. 点击配网按钮。
4. 设备会开启 `Catputer-Setup` 热点。
5. 用手机或电脑连接该热点，密码按屏幕提示或固件默认配置。
6. 访问设备显示的地址，通常是 `http://192.168.4.1`。
7. 选择或填写 Wi-Fi，提交后等待设备保存并重连。

说明：当前 BLE 配网入口默认关闭。之前点击 BLE 配网出现过重启，根因和内存/蓝牙栈初始化有关，后续除非专门做分区和内存评估，不要把它作为默认配网链路。

## 4. 编译与刷机

### Cardputer

常规编译：

```powershell
pio run -e m5stack-cardputer
```

常规刷机脚本：

```powershell
powershell -ExecutionPolicy Bypass -File scripts\windows\flash.ps1 -Port COM5
```

端口按实际设备调整。

### Waveshare 触屏端编译

触屏端使用独立 PlatformIO core，避免被 Cardputer 环境污染：

```powershell
powershell -ExecutionPolicy Bypass -File scripts\windows\pio-run.ps1 -EnvName waveshare-amoled-18
```

或直接：

```powershell
pio run -e waveshare-amoled-18
```

### Waveshare 触屏端串口刷机

优先使用已验证脚本，不要手写零散命令：

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File scripts\windows\flash-waveshare-com9-wait.ps1 -SkipBuild -AcknowledgeChecklist -WaitSeconds 35 -TouchAttempts 3
```

已验证成功链路：

1. `COM8` 触发 1200bps touch，系统出现断开/重连提示音。
2. 脚本高频轮询端口，日志出现 `probe ... COM9`。
3. 立即进入 `[COM9-WAIT] COM9 detected. Starting esptool flash...`。
4. 写入并校验通过：`Hash of data verified.`。
5. 复位完成：`Hard resetting via RTS pin...` 与 `[COM9-WAIT] Flash completed on COM9.`。

如果写入和校验已经成功，先看设备实际表现，不要被运行态 `COM8` 的短暂空窗带偏。

### SD OTA

生成 SD OTA 固件包：

```powershell
powershell -ExecutionPolicy Bypass -File scripts\windows\prepare-sd-ota.ps1
```

产物路径：

```text
sdcard/firmware/update.bin
```

复制到真实 SD 卡时保持同样路径：

```text
/firmware/update.bin
```

注意：

- `update.bin` 是生成物，不提交到 git。
- 触屏端 SD 卡热插拔后不一定会自动重新挂载；如果刚把 SD 卡插回设备却提示找不到固件，先重启设备。
- SD OTA 当前会尝试多个路径，但推荐统一使用 `/firmware/update.bin`。

## 5. SD 卡资源布局

推荐 SD 卡结构：

```text
/firmware/update.bin
/pet/backgrounds/home_morning.rgb565
/pet/backgrounds/home_noon.rgb565
/pet/backgrounds/home_dusk.rgb565
/pet/backgrounds/home_night.rgb565
/pet/backgrounds/*.ok
/pet/dialogue/prompts.txt
/pet/dialogue/dialogue.txt
/pet/ime/*.txt
/pet/photos/*.png
/pet/photos_manifest.txt
```

背景图当前约定：

- 资源尺寸：`184x224`
- 格式：`RGB565`
- 显示方式：运行时 2x 放大到 `368x448`
- 缓存位置：PSRAM

不要把背景缓存回退到内部 RAM；ESP32-S3 的 DMA/内部 RAM 很小，WiFi、LVGL、触摸和音频都会争抢这部分空间。

## 6. 当前限制与高风险点

- Waveshare 本地 TTS 已从主链路移除。它曾导致联网或触摸后重启，后续如果恢复，需要先做分区、内存和任务隔离设计。
- BLE 配网默认关闭。当前更稳的方案是 AP/Web 配网。
- 不要恢复开机强制联网。联网应按需触发，天气也不需要高频刷新。
- 不要随意改 LVGL 绘图缓冲。当前分块刷新是为 WiFi 和系统任务保留内部 RAM。
- 不要把 `sdcard/firmware/update.bin`、日志、临时图、真实配置提交到 git。
- 不要在没有证据时重构已经验证稳定的刷机、联网、SD OTA 链路。

## 7. Git 提交前检查

每次准备提交前至少做这些检查：

```powershell
git status --short --untracked-files=all
rg -n "sk-|gsk_|WIFI_PASS|wifiroom|ChillyRoom|API_KEY|GROQ|OPENAI|password|pass" -S --glob "!*.bin" --glob "!*.png" --glob "!*.jpg" --glob "!lib/**" --glob "!_vendor/**" .
```

提交策略：

- 项目源码、脚本、文档、必要资源可以提交。
- 生成固件、日志、临时图片不提交。
- 真实密钥、真实 Wi-Fi、个人本地配置不提交。

## 8. 后续开发建议

- 优先继续把触屏端做成“轻交互、强陪伴”的端侧，而不是堆复杂输入。
- PC 猫舍继续承担多宠物管理、记忆查看、配置入口和端侧选择。
- 新增功能必须同步更新文档，尤其是刷机、联网、SD、内存相关变更。
- 每次功能完成后都做一轮回归：启动、天气/时间、基础交互、设置页、SD OTA、PC 同步。
