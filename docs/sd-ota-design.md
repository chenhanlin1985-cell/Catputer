# SD 卡本地刷机（SD OTA）设计说明

## 目标
- 在无串口连接条件下，通过把固件放入 SD 卡完成设备自更新。
- 首版路径固定为：`/firmware/update.bin`。

## 当前实现（已接入代码）
- 模块：`src/sd_ota.h` / `src/sd_ota.cpp`
- 入口：触屏端下滑面板下半区中间点击（文案：`下中刷机`）
- 刷机状态：
  - `待机`
  - `刷机中 xx%`
  - `刷机完成，重启生效`
  - `刷机失败`
- 成功后再次点击“下中”会触发重启加载新固件。
- Windows 打包脚本：`scripts/windows/prepare-sd-ota.ps1`
  - 默认输出到仓库 `sdcard/firmware/update.bin`
  - 可用 `-TargetRoot <SD盘符路径>` 直接输出到真实 SD 卡。

## 工作流程
1. 检查 SD 卡可用并打开 `/firmware/update.bin`。
2. 检查 OTA 目标分区是否存在（`esp_ota_get_next_update_partition`）。
3. `Update.begin(size, U_FLASH)` 初始化。
4. 按块写入（2KB）并更新进度。
5. `Update.end(true)` 校验成功后标记完成，等待用户重启。

## 关键约束
- 当前工程默认分区是 `partitions_catputer_local_tts.csv`，可能只含单应用分区。
- 若分区不支持 OTA，界面会显示：`当前分区不支持OTA`。
- 这属于预期保护，不会误写运行分区。

## 下一步（必须）
1. （已完成）触屏端改为 OTA 双分区表：`partitions_waveshare_ota.csv`。
2. （已完成）校准 `voice_data` / `spiffs` 偏移，避免与 OTA 分区重叠。
3. （已完成）更新 Waveshare 刷机脚本语音包偏移到 `0x890000`。
4. 增加固件签名/哈希校验（至少 SHA256）。

## 分区迁移提醒
- 本次仅切到 `waveshare-amoled-18` 环境使用 `partitions_waveshare_ota.csv`。
- 首次切换分区表后，需要用串口完整刷一次（bootloader + partitions + firmware）。
- 之后才可长期使用 SD OTA 做“无电脑更新”。
