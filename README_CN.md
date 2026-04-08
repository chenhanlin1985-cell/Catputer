# 小橘猫口袋宠物

[English](README.md)

这是一个运行在 `M5Stack Cardputer / Cardputer Adv` 上的口袋宠物项目。

当前版本已经从早期的实验性聊天壳，整理成了更接近成品的小橘猫设备：

- 离线也能玩
- 联网后更聪明
- 中文优先
- 支持语音输入和朗读
- 支持外出、纪念盒、照片和成长记录

## 主要功能

### 离线宠物

- 喂食
- 玩乐
- 小睡
- 清理
- 迷你游戏
- 外出散步
- 纪念盒查看
- 本地成长存档

当前核心属性：

- 饱腹
- 心情
- 体力
- 清洁
- 亲密

### 联网增强

- AI 聊天
- 语音输入
- 语音朗读
- 天气同步

## 当前结构

现在的连接方式已经比较简单：

1. 设备文字聊天：直接连接兼容 OpenAI 的 HTTPS API
2. 本地电脑：只负责 `stt_proxy.py`，提供语音识别和 TTS
3. 不再依赖 `OpenClaw`

也就是说：

- 文字聊天不需要本地网关
- 只有语音输入和朗读需要本地服务

## 快速开始

### 1. 配置环境变量

至少需要这些：

- `WIFI_SSID`
- `WIFI_PASS`
- `API_KEY`

常见默认值：

- `API_HOST=dashscope.aliyuncs.com`
- `API_PORT=443`
- `DEFAULT_CITY=Shenzhen`

示例配置文件：

- `.env.example`
- `clawputer.local.ps1.example`

### 2. 编译和刷机

```bash
pio run -t upload
```

Windows 可以直接用脚本：

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\windows\flash.ps1 -Port COM5
```

### 3. 启动本地语音服务

如果只用文字聊天，这一步不是必须的。

如果要语音输入或朗读：

```bash
python tools/stt_proxy.py
```

Windows：

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\windows\start-stt-proxy.ps1
```

## 按键说明

### 宠物界面

- `TAB`：进入聊天
- `, / ; .`：移动
- `f`：喂食
- `p`：玩乐
- `n`：小睡
- `c`：清理
- `g`：迷你游戏
- `o`：外出
- `v`：查看纪念盒
- `h`：打开帮助面板
- `i`：打开状态面板
- `Fn + , / Fn + .`：调节音量
- `Fn + R`：重置配置

### 聊天界面

- `TAB`：返回宠物界面
- `Enter`：发送
- `Backspace`：删除
- `Fn`：按住说话
- `Fn + ; / Fn + /`：滚动聊天记录
- `Ctrl + Enter`：切换自动朗读

## 本地服务

语音功能仍然需要：

- `tools/stt_proxy.py`
- `ffmpeg`
- `edge-tts`
- `GROQ_API_KEY`

已经不再需要：

- `OpenClaw`

## SD 卡支持

如果插入 SD 卡：

- 纪念盒优先写到 `/pet/souvenirs.txt`
- 事件日志写到 `/pet/events.log`
- 外出照片从 `/pet/photos/` 读取

如果没有 SD 卡，核心存档会退回到 NVS。

## 仓库说明

- 不要提交真实的 Wi-Fi、API key 或其他个人配置
- 请使用示例配置文件作为模板
- 本地日志、测试请求和个人辅助文件已经默认忽略
