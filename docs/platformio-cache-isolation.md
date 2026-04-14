# PlatformIO 多端缓存隔离

本文记录 Catputer 当前的构建安全机制，用来避免 Cardputer 和 Waveshare 触屏端反复覆盖同一个 PlatformIO 全局包缓存。

## 问题根因

当前项目同时维护两类端侧：

- `m5stack-cardputer` 使用官方 PlatformIO `espressif32@6.13.0`，对应旧版 `framework-arduinoespressif32`。
- `waveshare-amoled-18` 使用 `pioarduino/platform-espressif32`，对应 Arduino ESP32 `3.3.7`，并需要 `framework-arduinoespressif32-libs`。

PlatformIO 默认把框架包安装到用户全局目录：

```text
%USERPROFILE%\.platformio\packages
```

这会导致一个实际问题：刚编译完 Cardputer 后，`framework-arduinoespressif32` 可能被切成旧版；再编译 Waveshare 时，pioarduino 需要的 `tools/pioarduino-build.py` 或 libs manifest 就可能不存在。反过来也可能污染 Cardputer 的构建环境。

## 解决策略

项目内新增了端侧隔离缓存目录：

```text
.pio-core/cardputer
.pio-core/waveshare
.pio-core/native
```

这些目录通过 `PLATFORMIO_CORE_DIR` 指定，只存在本机，不进入 Git。`.gitignore` 已忽略 `.pio-core/`。

## 推荐命令

以后不要直接在这个项目里运行裸 `pio run`。优先使用包装脚本：

```powershell
.\scripts\windows\pio-run.ps1 -EnvName waveshare-amoled-18
.\scripts\windows\pio-run.ps1 -EnvName m5stack-cardputer
```

也可以执行常用辅助操作：

```powershell
.\scripts\windows\pio-run.ps1 -EnvName waveshare-amoled-18 -PkgList
.\scripts\windows\pio-run.ps1 -EnvName waveshare-amoled-18 -Target upload -UploadPort COM8
```

脚本会根据 `-EnvName` 自动选择缓存桶：

- `waveshare-*` -> `.pio-core/waveshare`
- `m5stack-*` -> `.pio-core/cardputer`
- `native` -> `.pio-core/native`

## 刷机脚本

以下脚本也已经改成使用隔离缓存：

```powershell
.\scripts\windows\flash-waveshare-amoled.ps1
.\scripts\windows\flash.ps1
```

Waveshare 刷机脚本会从 `.pio-core/waveshare/packages` 查找 esptool 和 Arduino boot app：

```text
.pio-core/waveshare/packages/tool-esptoolpy/esptool.py
.pio-core/waveshare/packages/framework-arduinoespressif32/tools/partitions/boot_app0.bin
```

这样即使刚刚编译过 Cardputer，Waveshare 的刷机依赖也不会被覆盖。

## 如果已经污染或半安装

如果又出现类似错误：

```text
MissingPackageManifestError
Could not find package.json
argument should be a str or an os.PathLike object, not 'NoneType'
```

优先删除对应隔离缓存，不要删除整个用户级 PlatformIO：

```powershell
Remove-Item .pio-core\waveshare -Recurse -Force
.\scripts\windows\pio-run.ps1 run -e waveshare-amoled-18
```

或针对 Cardputer：

```powershell
Remove-Item .pio-core\cardputer -Recurse -Force
.\scripts\windows\pio-run.ps1 run -e m5stack-cardputer
```

## 开发约定

- 多端构建统一使用 `scripts/windows/pio-run.ps1`。
- 端侧刷机统一使用对应脚本，不直接手写全局 `.platformio/packages` 路径。
- 文档、脚本和排障记录中如出现 `pio run`，后续应逐步替换为隔离脚本。
- `.pio-core/` 是本机缓存目录，不要提交。
