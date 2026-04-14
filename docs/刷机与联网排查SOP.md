# Catputer 刷机与联网排查 SOP（防重复踩坑）

这份文档是“固定流程”，目标是避免每次像第一次刷机一样重走弯路。  
结论先说：**先固化流程，再排查问题**，不要边猜边改。

## 1. 刷机固定入口

Waveshare 触屏端统一用：

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File scripts\windows\flash-waveshare-amoled.ps1
```

只重刷不重编译：

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File scripts\windows\flash-waveshare-amoled.ps1 -SkipBuild
```

说明：
- 脚本会优先查找 `COM9` bootloader；如果 `COM9` 已存在，直接用 `no_reset` 刷写 `COM9`，不要绕道 `COM8`。
- 如果 `COM9` 不存在，脚本才用运行态端口触发 `1200bps touch -> bootloader 端口 -> no_reset`。
- `COM8` 主要是运行态端口，不要把它当成首选刷写端口；除非在诊断时明确传 `-TryDirectReset`。

### 已验证成功模板（推荐）

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File scripts\windows\flash-waveshare-com9-wait.ps1 -SkipBuild -WaitSeconds 60 -PollMs 80 -AutoTouchFromCOM8
```

说明：
- 这个脚本会先触发一次 `COM8` 的 1200bps touch（你会听到设备断开/连接提示音）。
- 然后高频轮询端口，只要 `COM9` 出现就立刻对 `COM9` 执行 `esptool` 刷写。
- 如果 60 秒内没有出现 `COM9`，脚本会停止并报错，不会再绕回 `COM8` 盲刷。

成功判定关键字：
- `[COM9-WAIT] COM9 detected. Starting esptool flash...`
- `Hash of data verified.`
- `Hard resetting via RTS pin...`
- `[COM9-WAIT] Flash completed on COM9.`

## 2. 刷机前检查（必做）

1. 关闭所有串口占用程序（PlatformIO monitor、串口助手、日志窗口）。
2. 只保留目标设备连接，避免多块板子同时枚举成 `VID_303A&PID_1001`。
3. 用下面命令确认当前端口：

```powershell
Get-CimInstance Win32_SerialPort | Select-Object DeviceID,Name,PNPDeviceID
```

补充：
- 也可以用 `python -c "from serial.tools import list_ports; print([p.device for p in list_ports.comports()])"` 快速看端口变化，避免 WMI 偶发卡顿。

## 3. 联网问题固定排查顺序（底层优先）

不要直接改业务逻辑，先确认每一层：

1. **配置层**：串口 `get_config` 看 `wifi_ssid/api_host/api_port/city` 是否正确。
2. **连网层**：看 WiFi 事件日志（STA_START / DISCONNECTED reason / GOT_IP）。
3. **HTTP层**：区分错误类型：
   - `no http response`：TCP/服务端无响应。
   - `request write failed/body write failed`：发送阶段失败（连接中断/缓冲问题）。
   - `invalid port`：配置值非法（端口不是数字或范围异常）。
4. **业务层**：只在前 3 层确认后，再看聊天/天气逻辑。

## 4. 当前调试日志约定

触屏端联网诊断日志前缀：
- `[WEATHER] ...`
- `[WEATHER][EVT] ...`

重点字段：
- `status=xxx(WL_...)`
- `STA_DISCONNECTED reason=xxx (...)`
- `STA_GOT_IP ip=...`

## 5. 回归清单（每次改动后都跑）

1. 刷机成功，设备正常启动。
2. 天气能显示，离线时能显示缓存。
3. 聊天至少连发 3 次不出现秒失败。
4. 交互（喂食/清理/玩乐）不黑屏、不重启。
5. 串口日志无连续异常刷屏。

## 6. 约束规则（以后不再破坏稳定项）

- 稳定功能先“冻结”，新增功能不允许改动其底层链路。
- 任何网络改动必须附带“错误码映射 + 串口日志样例”。
- 刷机脚本是唯一入口，不再手打零散命令。
- 改完先过回归清单，再进入下一功能迭代。
