# Waveshare AMOLED 1.8 适配工作总结

## 目标

本文总结当前仓库把原本偏 Cardputer 的主界面迁移到 Waveshare ESP32-S3 Touch AMOLED 1.8 的完整过程，记录已经确认的根因、现阶段稳定方案、关键代码入口、仍未完成的部分，以及后续建议路线。

当前结论已经比较明确：

- 屏幕硬件和触摸硬件本身没有坏
- 原问题不在单一驱动，而在“旧 UI 显示模型 + 板级资源冲突 + 字体/触控适配不完整”的组合问题
- Waveshare 上可靠的方案是：
  - 使用独立触摸路径
  - 避开 SD 与触摸 I2C 的脚位冲突
  - 主界面改为 LVGL 局部刷新
  - 不再直接复用 Cardputer 的整屏重绘链

---

## 一、问题现象回顾

最开始在 Waveshare 板子上，主固件有这些明显问题：

- 屏幕狂闪
- 触控无反应
- UI 切页、聊天返回等交互无法正常工作
- 即使能显示，也明显不是这块 AMOLED 该有的稳定效果

后续在不同阶段又出现过：

- 上传端口被串口监视占用
- 触摸按压状态能到，但坐标始终固定或无效
- 新界面可显示，但依然闪烁
- 直接改到底层触摸后，某些版本出现黑屏
- 中文字体大量缺字
- 主界面虽然稳定，但视觉仍和旧版舞台感不一致

---

## 二、排查过程与关键结论

### 1. baseline 验证阶段

首先用独立 baseline 固件验证最底层能力，目标是回答两个问题：

- SH8601 显示是否正常
- FT3168 触摸是否正常

相关文件：

- [waveshare_baseline.cpp](file:///c:/clawputer/src/waveshare_baseline.cpp)

结论：

- 屏幕显示正常
- 触摸芯片正常
- 圆角屏存在安全区要求

因此最初就可以排除“硬件整体损坏”。

### 2. 旧主界面路径不兼容

后续把主固件逐步迁回 Waveshare 时发现：

- 一旦接回原本 Companion/Chat 的整屏更新逻辑，屏幕开始严重闪烁
- 触控在旧路径里也不稳定

这个阶段确认：

- 原项目的 Cardputer 风格显示链不适合直接搬到 SH8601 AMOLED 上
- 问题重点在渲染/交互层，而不是单独某个底层设备驱动

### 3. 触摸坐标不工作并不是“触摸芯片坏了”

曾出现过这种现象：

- `pressed` 可以变化
- `IRQ` 可以变化
- 但 `x/y` 坐标无效、固定或全是默认值

这说明：

- 中断到了
- 触摸“是否按下”的逻辑到了
- 但坐标寄存器并没有被正确读取

继续下钻后发现真正根因是 **资源冲突**。

### 4. 根因之一：SD 初始化占用了触摸 I2C 引脚

最终确认的第一个关键根因：

- Waveshare 触摸 I2C `SCL = GPIO14`
- 原项目的 `PetStorage` 把 `GPIO14` 当成了 `SD SPI MOSI`
- 主固件启动后执行 `PetStorage::begin()`，会把触摸线重新配置掉

结果就是：

- 触摸中断还能到
- 但 FT3168 坐标寄存器读取失败

关键位置：

- 板级引脚：[pin_config.h](file:///c:/clawputer/lib/waveshare/Mylibrary/pin_config.h)
- 冲突来源：[pet_storage.cpp](file:///c:/clawputer/src/pet_storage.cpp)
- 当前规避位置：[main.cpp](file:///c:/clawputer/src/main.cpp)

当前做法：

- 在 Waveshare 路线上先避开 `PetStorage::begin()`
- 保住触摸 I2C，不再让 SD 初始化覆盖 GPIO14

### 5. 根因之二：整屏重绘模型不适合当前 AMOLED 路线

查阅官方与 vendor 示例后确认，Waveshare 正常稳定的路径不是“每帧整屏刷”。

参考资料：

- [example_qspi_with_ram.c](file:///c:/clawputer/_vendor/ESP32-S3-Touch-AMOLED-1.8/examples/ESP-IDF-v5.3.2/05_LVGL_WITH_RAM/main/example_qspi_with_ram.c)
- [13_LVGL_Widgets.ino](file:///c:/clawputer/_vendor/ESP32-S3-Touch-AMOLED-1.8/examples/Arduino-v3.3.5/examples/13_LVGL_Widgets/13_LVGL_Widgets.ino)
- [display.md](file:///c:/clawputer/_vendor/ESP32-S3-Touch-AMOLED-1.8/examples/Arduino-v3.3.5/libraries/lvgl/docs/porting/display.md)

文档和例子共同指向：

- 局部刷新是正常方案
- 小块双缓冲是正常方案
- 不推荐继续沿用当前项目旧有的整屏刷新思路

这一步把问题从“猜测闪烁”变成了“文档明确不建议的架构”。

---

## 三、从旧渲染链切换到 LVGL 稳定路线

### 1. 探针固件

为了验证官方路线是否真能稳定，先单独写了一个 probe：

- [waveshare_lvgl_probe.cpp](file:///c:/clawputer/src/waveshare_lvgl_probe.cpp)

作用：

- 单独验证 LVGL
- 单独验证局部刷新
- 单独验证触摸输入
- 避免受原主界面逻辑干扰

结果：

- probe 稳定
- 不闪
- 触摸可用

这一步直接坐实：

- 闪烁根因不在硬件
- 而在原主界面的显示架构

### 2. 新主界面入口

在 probe 验证稳定后，新增了 Waveshare 专用主界面入口：

- [main_waveshare_lvgl.cpp](file:///c:/clawputer/src/main_waveshare_lvgl.cpp)

并把 `waveshare-amoled-18` 环境切换到这条入口：

- [platformio.ini](file:///c:/clawputer/platformio.ini)

当前这条路线的特点：

- 使用 LVGL draw buffer
- 使用 `flush_cb(area, color_p)` 局部刷新
- 使用单独触摸输入回调
- 不再走旧 `main.cpp` 里的整屏 Companion 显示路径

---

## 四、当前主界面已经完成的工作

### 1. 显示稳定

当前主界面已经确认：

- 不闪
- 亮度正常
- 动画和 UI 可以共存

这是整次适配最核心的突破点。

### 2. 触摸恢复

当前主界面已经恢复的基础交互：

- `主页 / 更多` 切页
- 主界面按钮响应
- 点宠物触发互动

为了改善早期“点很多次才成功”的问题，已做了这些调整：

- 记录最近一次有效触点坐标
- 避免抬手丢点
- 多个纯展示容器关闭滚动/吞触摸行为
- 交互从过严的点击判定调整为更适合触屏的释放触发

说明：

- 目前用户反馈是“开始时略迟钝，稍后会好一些”
- 说明触控链已经基本通，但仍值得继续优化输入稳定性和控件层级

### 3. 旧版宠物美术接回

当前中央小猫已经不再使用临时文本脸，而是接回了旧版像素资源：

- 橙色资源：[sprites.h](file:///c:/clawputer/src/sprites.h)
- 紫色资源：[sprites_purple.h](file:///c:/clawputer/src/sprites_purple.h)

当前已支持：

- 根据宠物种类切换橙猫 / 紫猫
- 根据状态和帧号切换 idle / happy / sleep / talk 等 sprite

用户已确认：

- 现在已经是之前的美术
- 不闪

### 4. 舞台背景回归

当前已加回“接近旧舞台”的背景层：

- 天空
- 地面
- 日月 / 星星 / 云
- 雨 / 雪 / 雾等天气层

这些背景由 `main_waveshare_lvgl.cpp` 内部背景绘制层负责，继续运行在 LVGL 局部刷新体系上。

用户反馈：

- 背景已经接近
- 小猫和背景已经基本 OK

### 5. 中文化

当前主界面文案已经开始中文化：

- 按钮名称中文化
- 天气中文化
- 状态中文化
- 提示文案中文化

但问题是：

- 使用 LVGL 内建 CJK 字体时，字库不全，仍然大量缺字

所以当前新增了一套 **构建时自动生成中文字库** 的方案：

- 字符清单：[lvgl_font_chars.txt](file:///c:/clawputer/tools/lvgl_font_chars.txt)
- 生成脚本：[generate_lvgl_font.py](file:///c:/clawputer/tools/generate_lvgl_font.py)
- 生成文件：[lv_font_cat_cn_18.c](file:///c:/clawputer/src/lv_font_cat_cn_18.c)

这套方案已经接入构建流程，但当前仍处在“逐步补齐字符集”的阶段，中文缺字问题尚未彻底清空。

---

## 五、当前仓库中的关键改动点

### 主界面主入口

- [main_waveshare_lvgl.cpp](file:///c:/clawputer/src/main_waveshare_lvgl.cpp)

当前主要承担：

- LVGL 初始化
- SH8601 显示初始化
- FT3168 触摸初始化
- 主舞台背景绘制
- 宠物 sprite 绘制
- stats / 动作按钮 / 提示条 / 详情面板

### 触摸 / 显示设备适配

- [waveshare_device.cpp](file:///c:/clawputer/src/waveshare_device.cpp)
- [waveshare_device.h](file:///c:/clawputer/src/waveshare_device.h)

说明：

- 这部分曾用于验证直接读取触摸寄存器
- 后续真正稳定的主界面路线已经改成在 `main_waveshare_lvgl.cpp` 中单独接入 LVGL + 触摸输入

### baseline 与 probe

- [waveshare_baseline.cpp](file:///c:/clawputer/src/waveshare_baseline.cpp)
- [waveshare_lvgl_probe.cpp](file:///c:/clawputer/src/waveshare_lvgl_probe.cpp)

建议保留原因：

- baseline 是底层显示/触摸金标准
- probe 是验证 LVGL 局部刷新稳定性的金标准

### 构建配置

- [platformio.ini](file:///c:/clawputer/platformio.ini)

当前与 Waveshare 相关的重点：

- `waveshare-amoled-18` 已切到 `main_waveshare_lvgl.cpp`
- 增加了 `waveshare-amoled-18-lvgl-probe`
- 增加了中文字库生成的 pre-script

### 中文字库工具链

- [lvgl_font_chars.txt](file:///c:/clawputer/tools/lvgl_font_chars.txt)
- [generate_lvgl_font.py](file:///c:/clawputer/tools/generate_lvgl_font.py)
- [lv_font_cat_cn_18.c](file:///c:/clawputer/src/lv_font_cat_cn_18.c)

---

## 六、当前已确认的根因汇总

### 已彻底确认的根因

1. **SD 与触摸 I2C 脚位冲突**
   - 触摸 `SCL = GPIO14`
   - 原 SD 初始化也占用 `GPIO14`
   - 导致触摸坐标读取失败

2. **旧主界面的整屏重绘模型不适合当前 AMOLED 路线**
   - 在 Waveshare 上会造成严重闪烁
   - 官方推荐方案是 LVGL + 局部刷新

3. **LVGL 内建 CJK 字体不够覆盖当前中文文案**
   - 会出现大量缺字和乱码感
   - 需要自定义字库而不是继续依赖内建字库

### 已缓解但未完全收口的问题

1. **触控前期手感偏迟钝**
   - 已有明显改善
   - 但仍需继续优化对象层级和输入判定

2. **视觉融合尚未完全回到旧版**
   - 当前“背景 + 小猫”已经基本对路
   - 但信息面板和底部操作条仍有较强 UI 面板感

3. **中文仍可能缺部分字符**
   - 字库生成链已打通
   - 但需要继续维护 `lvgl_font_chars.txt`

---

## 七、当前功能状态

### 已经可用

- Waveshare 主界面稳定显示
- 主界面不闪
- 触控基础交互可用
- `主页 / 更多` 可切换
- 主界面像素猫已恢复
- 舞台背景已恢复一部分
- 亮度和基本视觉层次已改善

### 暂时不作为主目标

- Chat 页面完整迁移

当前策略是：

- 先把主界面和基础交互做扎实
- Chat 后续单独迁移到同样的 LVGL 稳定路线

---

## 八、后续建议路线

### 第一优先级：把主界面做完整

建议继续优先做：

- 压低 stats / 提示条 / 面板存在感
- 进一步增强“猫直接站在舞台上”的效果
- 对应天气和时间继续细化背景
- 优化按钮触控成功率

### 第二优先级：彻底补齐中文字库

建议后续流程：

- 所有新增中文文案先统一收口
- 把缺失字符追加到 [lvgl_font_chars.txt](file:///c:/clawputer/tools/lvgl_font_chars.txt)
- 继续使用 [generate_lvgl_font.py](file:///c:/clawputer/tools/generate_lvgl_font.py) 自动生成字体

### 第三优先级：Chat 独立迁移

Chat 建议保持同样原则：

- 不回旧整屏重绘链
- 单独走 LVGL 局部刷新
- 语音、消息流、按钮分区都在新显示架构下重建

### 第四优先级：Waveshare 专属存储方案

由于原 Cardputer 存储路径与触摸脚位冲突，后续如果需要恢复存储能力，建议：

- 单独设计 Waveshare 的存储引脚方案
- 不再复用当前 Cardputer 风格的 `PetStorage` SPI 配置

---

## 九、当前文档对应的主要文件清单

- 主界面新实现：[main_waveshare_lvgl.cpp](file:///c:/clawputer/src/main_waveshare_lvgl.cpp)
- 主入口环境配置：[platformio.ini](file:///c:/clawputer/platformio.ini)
- 主界面字体生成脚本：[generate_lvgl_font.py](file:///c:/clawputer/tools/generate_lvgl_font.py)
- 主界面字体字符集：[lvgl_font_chars.txt](file:///c:/clawputer/tools/lvgl_font_chars.txt)
- 生成后的字体文件：[lv_font_cat_cn_18.c](file:///c:/clawputer/src/lv_font_cat_cn_18.c)
- 旧版橙猫资源：[sprites.h](file:///c:/clawputer/src/sprites.h)
- 旧版紫猫资源：[sprites_purple.h](file:///c:/clawputer/src/sprites_purple.h)
- 底层 baseline 参考：[waveshare_baseline.cpp](file:///c:/clawputer/src/waveshare_baseline.cpp)
- LVGL probe 参考：[waveshare_lvgl_probe.cpp](file:///c:/clawputer/src/waveshare_lvgl_probe.cpp)
- 触摸/显示设备文件：[waveshare_device.cpp](file:///c:/clawputer/src/waveshare_device.cpp)
- SD 冲突来源：[pet_storage.cpp](file:///c:/clawputer/src/pet_storage.cpp)
- 板级引脚定义：[pin_config.h](file:///c:/clawputer/lib/waveshare/Mylibrary/pin_config.h)

---

## 十、当前一句话结论

这次 Waveshare 适配已经从“驱动和闪烁排障阶段”，进入“基于稳定 LVGL 架构做正式主界面”的阶段。

真正起作用的关键转折点有三个：

- baseline 证明硬件没坏
- 查清 GPIO14 的 SD / 触摸冲突
- 主界面放弃旧整屏重绘，改走 LVGL 局部刷新

在这个基础上，后续继续完善主界面、补全中文、再迁 Chat，已经是一条可持续推进的正确路线。
