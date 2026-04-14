# 手动触发蓝牙配网方案（触屏端）

## 目标
- 不再“无 Wi-Fi 自动进入配网”。
- 只在用户从下滑设置面板主动点击后，进入蓝牙配网流程。
- 用 BLE 做“发现与引导”，用本地 Web 页做“配置提交与状态反馈”。

## 用户流程（最终体验）
1. 用户下滑打开设置面板，点击 `蓝牙配网`。
2. 设备进入 `配网模式`（默认 5 分钟），屏幕显示：
   - `等待蓝牙连接`
   - 设备名（如 `Catputer-xxxx`）
   - 倒计时
3. 手机/电脑蓝牙连接设备。
4. 屏幕提示访问地址：
   - 优先：`http://catputer.local`
   - 备用：`http://<device_ip>`（若已拿到 IP）
5. 用户在页面填写/修改 Wi-Fi（SSID/密码）并提交。
6. 设备尝试联网并回显结果：
   - 成功：`已连接 Wi-Fi`
   - 失败：`认证失败 / 超时 / DNS失败 / 网关不可达`
7. 退出配网模式，回到宠物主界面。

## 触屏设置面板改动
- 新增入口：`蓝牙配网`
- 配网中状态文案：
  - `配网中（xx秒）`
  - `已连接蓝牙，等待配置`
  - `正在验证网络`
  - `配置成功` / `配置失败：<原因>`
- 新增按钮：
  - `取消配网`（随时返回主界面）
  - `重试`（失败后可直接重试，不必重启）

## 端侧状态机（建议字段）
```cpp
enum class ProvisionState : uint8_t {
  Idle,              // 默认
  BleAdvertising,    // BLE广播中，等待连接
  BleConnected,      // BLE已连接，等待Web提交
  ApplyingConfig,    // 正在写入配置
  WifiVerifying,     // 正在联网验证
  Success,           // 成功
  Failed,            // 失败
  Cancelled,         // 用户取消
  Timeout            // 配网超时
};
```

```cpp
struct ProvisionContext {
  ProvisionState state;
  uint32_t startedAtMs;
  uint32_t expiresAtMs;      // 默认 5 分钟
  uint8_t retryCount;
  String lastErrorCode;      // AUTH_FAIL / DHCP_TIMEOUT / DNS_FAIL / HTTP_FAIL...
  String lastErrorMessage;   // 面向用户的中文提示
  bool tokenIssued;          // 是否签发一次性配置token
  String sessionToken;       // Web提交时校验
};
```

## 配置数据结构（持久化）
- `wifi.ssid`
- `wifi.password`
- `wifi.ssid2`（可选）
- `wifi.password2`（可选）
- `weather.city`
- `gateway.host`
- `gateway.port`
- `gateway.token`
- `schema.version`（配置版本，便于兼容升级）

## Web API（最小可用）
- `GET /api/provision/status`
  - 返回当前状态、倒计时、最后错误
- `POST /api/provision/config`
  - 提交 Wi-Fi 配置（需 token）
- `POST /api/provision/cancel`
  - 取消配网并返回 `Idle`
- `POST /api/provision/retry`
  - 失败后重试联网验证

## BLE 职责边界（避免复杂化）
- BLE 不做完整配置写入。
- BLE 仅做：
  - 设备发现
  - 会话建立
  - 下发一次性 token / 引导信息
- 真实配置写入、联网验证全部走 Web API。

## 异常与兜底
- 超时自动退出（`Timeout -> Idle`）。
- 写入配置失败不覆盖旧配置（先校验，再提交）。
- 联网验证失败保留旧可用配置（避免“改坏后回不来”）。
- 配网模式中不影响宠物基础渲染（主循环持续可用）。

## 验收用例（这版实现必须通过）
1. 下滑设置点击 `蓝牙配网`，进入倒计时页面。
2. 5 分钟不操作自动退出，不重启。
3. 提交错误密码，出现明确错误文案，可重试。
4. 提交正确配置后联网成功，退出配网，状态恢复正常。
5. 反复进入/退出 10 次，无卡死、无重启、无内存持续下降。

## 开发顺序（建议）
1. 先做状态机与设置入口（只显示状态，不接真实网络）。
2. 接 BLE 会话与 token 下发。
3. 接 Web API 与配置落盘。
4. 接 Wi-Fi 验证与错误码映射。
5. 最后做 UI 文案收口与超时/重试细节。

