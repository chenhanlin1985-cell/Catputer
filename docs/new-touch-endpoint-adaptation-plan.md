# 新触摸端侧适配计划（ESP-S3）

目标：在不破坏现有 Cardputer 体验的前提下，接入新的触摸端，并让 PC 猫舍能管理“哪只猫去哪个端侧”。

## 范围定义

- 现有端侧：`cardputer`（键盘交互）
- 新端侧：`esp32_touch`（触摸交互）
- 中控：PC 猫舍（设备发现、状态观察、猫咪派遣）

## 里程碑

### M1 协议与身份（本轮）

- [x] 端侧广播增加设备身份：`did`（device_id）、`dt`（device_type）
- [x] 端侧广播增加能力位：`tc/kc/mc/sp`
- [x] Town Sync 快照增加 `device` 对象（id/type/name/caps）
- [x] PC 端建立 `device_registry`（按 `device_id` 记录）并持久化
- [x] PC 端同步命令携带 `client` 元信息（为多端协商预留）

交付结果：协议已具备“多端可识别”能力，但 UI 仍以单目标主机为主。

当前代码进度补充：

- 已增加 PC 端设备下拉选择（基于广播自动发现）。
- 已为每只猫增加 `assigned_device_id` 与 `runtime_location` 字段并持久化。
- 已在回端快照里预留 `route.target_device_id`，端侧可识别并记录日志。

### M2 PC 猫舍多端调度

- [ ] 联动页增加“设备列表”（在线状态、类型、能力、最后心跳）
- [ ] 每只猫新增 `assigned_device_id`（派遣目标）
- [ ] 支持“派遣到指定端侧”
- [ ] 冲突保护：同一只猫不能同时驻留多个端侧
- [ ] 断线回仓：端侧超时自动回仓，避免“猫走失”

交付结果：PC 端可管理“多端驻留关系”。

### M3 新触摸端 MVP

- [ ] 新建 `esp32_touch` 端侧工程/入口
- [ ] 最小 UI：主页（猫+状态）/互动按钮/聊天入口
- [ ] 同步协议接入（sync_enter / sync_ping / sync_leave）
- [ ] 支持被 PC 指定接收某只猫并应用快照

交付结果：触摸端可完整接入猫咪流转。

### M4 体验增强

- [ ] 触摸手势：点击互动、滑动切页、长按菜单
- [ ] 差异化行为：触摸端偏“展示陪伴”，Cardputer 偏“随身轻交互”
- [ ] 多端日志与诊断页（失败原因、重试次数、同步耗时）

## 数据模型建议

- `device_registry[device_id]`
  - `device_type`
  - `source_ip`
  - `caps.touch/keyboard/mic/speaker`
  - `online`
  - `last_seen_unix`
- `pet`
  - `assigned_device_id`（计划派遣端）
  - `runtime_location`（`pc` / `device`）

## 协议约定（建议）

- 广播状态包（UDP）：
  - `did`, `dt`, `tc`, `kc`, `mc`, `sp`
- Town Sync 快照（TCP）：
  - `snapshot.device.id/type/name/caps`
- 命令请求（TCP）：
  - 增加 `client.id/type/protocol` 便于版本协商和日志审计

## 验收标准

1. PC 能同时识别至少 2 个端侧设备（Cardputer + 触摸端）。
2. 可以从猫舍选择任意一只猫派遣到指定端侧。
3. 端侧断线后，猫咪状态安全回仓，无重复实例。
4. 同步记录包含设备维度信息，便于问题追踪。
