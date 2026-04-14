# 宠物像素资源

这里是手持端与桌面端共用的宠物原始美术资源目录。

## 资源规格

- 画布尺寸：`16x16 px`
- 风格：纯像素风
- 背景：透明
- 朝向：默认朝右绘制
- 格式：`PNG`
- 不要抗锯齿
- 颜色尽量控制在固定的小色板内

## 动作组

每只猫建议提供这几组帧：

- `idle`：4 帧
- `happy`：2 帧
- `sleep`：1 帧
- `talk`：2 帧

## 目录结构

- `orange/idle/idle_1.png` 到 `idle_4.png`
- `orange/happy/happy_1.png` 到 `happy_2.png`
- `orange/sleep/sleep_1.png`
- `orange/talk/talk_1.png` 到 `talk_2.png`
- `purple/...`

## 当前项目对应关系

当前正式运行中的代码资源在：

- `src/sprites.h`
- `src/sprites_purple.h`

桌面端导出脚本在：

- `scripts/windows/export_handheld_sprites.py`

代码资源反导出为 PNG 的脚本在：

- `scripts/windows/export_pet_pngs.py`

## 命名规则

- 文件名统一使用小写 ASCII
- 推荐命名：
  - `idle_1.png`
  - `happy_1.png`
  - `sleep_1.png`
  - `talk_1.png`

## 使用建议

- 后续你直接在这里修改 PNG 原稿
- 改完后再做“导回代码资源”的流程
- 这样手持端和桌面端都能继续保持同源
