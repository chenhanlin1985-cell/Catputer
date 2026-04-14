# Catputer Agent Notes

这份文件是进入本仓库后的优先阅读入口。遇到 Waveshare/Cardputer 端侧问题时，先按这里的硬规则执行，再看 `docs/` 里的长文档。

## Waveshare 触屏端刷机

- 触屏端运行态通常是 `COM8`，bootloader 常切到 `COM9`。
- 1200bps touch 触发重枚举时，出现“端口不存在 / IO aborted”通常是正常现象。
- 固定入口使用 `scripts/windows/flash-waveshare-amoled.ps1`，不要手写零散刷机命令，除非脚本明确失败且需要按文档手动救援。
- 刷机脚本默认先找 `COM9` bootloader；如果 `COM9` 已存在，直接 `no_reset` 刷 `COM9`，不要绕道 `COM8`。只有 `COM9` 不存在时，才用运行态端口触发 `1200bps touch -> COM9 bootloader -> no_reset`；只有明确要排查时才传 `-TryDirectReset`。
- 如果 `COM9` bootloader 写入、校验、`Hard resetting` 已完成，先以设备实际表现为准，不要继续把 `COM8` 暂时没有串口日志当成新的根因。
- Waveshare 与 Cardputer 使用不同 PlatformIO core：Waveshare 走 `.pio-core/waveshare`，不要混用 Cardputer 的包缓存。

## Waveshare 联网排查

- 不要先改天气、HTTP 或聊天业务层。顺序固定为：配置层、WiFi 驱动状态、内部 RAM、HTTP 层、业务层。
- 如果最小诊断固件 WiFi 正常、主固件 `WiFi.mode(WIFI_STA)` 失败，优先怀疑主固件内部 RAM/初始化顺序。
- LVGL 绘图缓冲不要改回接近全屏的大块 DMA 双缓冲；当前使用 32 行分块刷新是为了给 WiFi 驱动保留内部 RAM。
- 不要恢复开机强制联网；当前设计是按需联网，并在离线时优先使用缓存。
- Waveshare SD 背景图当前使用 `184x224 RGB565`，运行时 2x 放大到 `368x448`，单张约 82KB；背景缓存只能走 PSRAM，绝不能失败后回退到内部 RAM，也不要在 WiFi 连接中或 TTS 播放中切换背景。

## 稳定性规则

- 已经验证稳定的路径不要在无证据时重构。新增功能应绕开稳定链路，而不是顺手修改底层。
- 串口日志只能作为证据的一部分；如果脚本已经在 bootloader 端口完成写入，不要让运行态端口短暂空窗带偏判断。
- 触屏端交互后黑屏/闪回曾由本地 TTS 阻塞主循环触发，类似问题优先查主循环阻塞和内部 RAM。
