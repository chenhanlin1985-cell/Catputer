# Waveshare 刷机必读（强约束）

这份文档是 `scripts/windows/flash-waveshare-com9-wait.ps1` 的前置检查卡。  
每次刷机前，必须先按这套流程执行，不要临时改流程。

## 固定流程

1. 设备运行态一般在 `COM8`。
2. 用 `COM8` 做 1200bps touch 触发重枚举（会听到系统断开/重连提示音）。
3. 脚本高频轮询端口，必须看到 `probe ... COM9`。
4. 进入 `COM9` 后立即刷写，日志必须出现：
   - `[COM9-WAIT] COM9 detected. Starting esptool flash...`
5. 刷写完成必须看到：
   - `Hash of data verified.`
   - `[COM9-WAIT] Flash completed on COM9.`

## 关键原则

- 刷写端口只用 `COM9`，不要改成 `COM8`。
- `COM8` 只用于触发进入 bootloader，不用于写入固件。
- 如果脚本超时，优先检查是否出现过 `COM9`，而不是切回旧流程。

## 标准命令

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\windows\flash-waveshare-com9-wait.ps1 -AcknowledgeChecklist
```

如需自动触发 COM8 touch：

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\windows\flash-waveshare-com9-wait.ps1 -AutoTouchFromCOM8 -AcknowledgeChecklist
```
