# LizzieYzy Qt6/C++ 全量重构计划

## Summary

- 使用仓库根目录中的 Qt 6 + C++20 + CMake 实现取代旧 Java 应用；旧 Java 源码从工作树移除，仓库历史仍可作为行为参考。
- 目标平台：Linux KDE + Wayland + NVIDIA、Windows + NVIDIA；首版不支持 macOS。
- UI 采用混合架构：主框架/菜单/设置/表格用 Widgets，棋盘/ownership/候选点动画用 Qt Quick 或自定义 QQuickItem。
- 首版只保证 KataGo，一等支持实时分析、批量分析、形势判断、候选点、变化图、人机对局、双引擎对比、引擎对局、SGF 管理。
- 不迁移首版范围：非 KataGo 引擎兼容、远程 SSH、Fox/在线棋谱、屏幕棋盘同步、可视化 KataGo 分布式训练。

## Architecture

- 分层拆分：
    - core：棋盘规则、坐标、SGF、分支树、对局信息、分析结果模型，不依赖 Qt UI。
    - engine：KataGo 进程管理、GTP 协议、analysis JSON 协议、命令队列、日志、错误诊断。
    - app：应用状态、配置、会话恢复、快捷键、动作分发、文件导入导出。
    - ui：Widgets 主窗口、Dock 面板、设置页、GTP 控制台、图表、棋盘 Quick 渲染。
    - tests：核心单测、协议 fixture 测试、可选真实 KataGo 集成测试。

- 线程模型：
    - UI 主线程只处理交互和绘制。
    - KataGo QProcess I/O 在 engine worker 对象中异步处理，通过 signal/slot 投递结构化事件。
    - 批量分析使用独立 analysis 进程；实时分析与批量分析不共用同一个 KataGo 进程。

- 状态原则：
    - 禁止复刻 Java 版 Lizzie.* 全局状态。
    - 用 GameModel 持有当前棋谱树，用 AnalysisStore 持有每个节点的分析快照，用 EngineManager 管理 KataGo 会话。
    - UI 只订阅模型变更，不直接拼 GTP 命令。

## Core Model

- 棋盘支持 9/13/19 路和 KataGo 支持的矩形棋盘；默认 19 路。
- 数据类型：
    - Point{x,y}、Color{Black,White}、Move{color, point|pass|resign}。
    - BoardPosition{size, stones, captures, ko, sideToMove, zobrist}。
    - GameNode{id, parent, children, move, setupStones, comment, properties, analysis}。
    - GameInfo{players, komi, rules, handicap, result, date, source}。
    - MoveCandidate{move, visits, winrate, scoreMean, scoreStdev, policy, pv, pvVisits, ownership}。

- SGF：
    - 读写 GM/FF/SZ/KM/HA/PB/PW/BR/WR/RE/DT/C/B/W/AB/AW/AE/LB/CR/SQ/TR/MA/MN。
    - 保留未知属性，避免打开保存后破坏用户棋谱。
    - 支持分支、注释、标记、让子、pass、resign、旧 Lizzie 分析注释导入。

- 规则：
    - 首版内置 Chinese/Japanese/Tromp-Taylor/AGA/NZ/Korean 规则枚举，实际裁判以 KataGo 设置为准。
    - 本地规则至少保证合法落子、提子、自杀禁入、劫、撤销、分支切换、试下。

## KataGo Engine

- 用户配置：
    - KataGo 可执行文件路径、模型路径、GTP config、analysis config、工作目录、额外参数。
    - GPU 后端不由应用猜测；应用只显示 KataGo stderr 中的 CUDA/OpenCL/TensorRT/错误信息。

- 实时 GTP：
    - 启动后发送 name、version、list_commands，检测 kata-analyze、kata-set-rules、kata-raw-nn、genmove。
    - 局面同步统一走 clear_board + boardsize/rectangular_boardsize + komi + 全量 play 重放；避免复杂 undo 漂移。
    - 分析命令使用 kata-analyze <interval>，按设置附加 ownership、turn、visits/playouts 等参数。
    - 用户切换分支、落子、改 komi、改规则时，停止当前分析，重放局面，再恢复分析。

- Analysis JSON：
    - 批量分析启动 katago analysis -model ... -config ... -quit-without-waiting。
    - 每个待分析节点发唯一 id，请求包含 moves、initialStones、rules、komi、maxVisits/maxTime、includeOwnership。
    - 解析返回 moveInfos、ownership、rootInfo，写回对应 GameNode.analysis。

- 失败处理：
    - 启动失败显示命令、工作目录、stderr 摘要。
    - 协议解析失败保留原始行到 GTP 控制台，不让 UI 崩溃。
    - KataGo 退出时自动停用分析状态，不自动重启，提供“重新启动引擎”。

## UI/UX

- 主窗口：
    - 中央棋盘，右侧候选点列表和 PV，底部胜率/目差图，左侧棋谱树可隐藏。
    - 顶部工具栏只放常用动作：打开、保存、分析开关、形势判断、前进后退、试下、设置。
    - 高级功能放菜单和 Dock，不复制旧版过密按钮布局。

- 棋盘渲染：
    - 4K 下使用 device-independent coordinates，棋盘线、星位、候选点、文字全部矢量绘制。
    - 棋子可用主题贴图；无高分辨率贴图时用程序绘制高质量棋子。
    - ownership 用半透明热图或方块层，可开关、调透明度、切换主棋盘/小棋盘显示。

- 图表：
    - 胜率、目差、恶手条、访问量趋势分层绘制。
    - 点击图表跳转对应手数；hover 显示手数、胜率、目差、访问量。

- 设置：
    - 引擎设置、分析设置、棋盘显示、快捷键、主题、语言、文件行为分组。
    - 首次启动向导只要求配置 KataGo、模型、GTP config 和 analysis config；允许“无引擎模式”进入。

## Feature Scope

- 必做：
    - SGF 打开/保存/另存、分支编辑、注释、标记、移动导航。
    - KataGo 实时分析、候选点、PV 预览、ownership、scoreMean、winrate。
    - 批量分析 SGF，显示进度，可取消，结果写回 SGF 或 sidecar JSON。
    - 人机对局、KataGo 自对局、双 KataGo 对比。
    - GTP 控制台、引擎日志、错误诊断。
    - 中英文界面、深浅主题、4K/高 DPI。

- 延后：
    - 远程 SSH 引擎、Fox/野狐相关、屏幕同步、分布式训练、非 KataGo 引擎、旧版分享/上传能力。

- 兼容策略：
    - 旧 Java 配置只做一次性导入，不保证字段完全兼容。
    - 旧 SGF 中的 Lizzie/KataGo 分析注释尽量读取；保存时优先写标准 SGF 属性和兼容注释。

## Implementation Phases

- Phase 0：建立根目录 Qt 工程、CI 构建、Qt6/CMake 基础、代码格式、测试框架。
- Phase 1：实现 core，完成棋盘规则、SGF、分支树、坐标、基础单测。
- Phase 2：实现 KataGo GTP 会话、命令队列、实时分析解析、假引擎 fixture 测试。
- Phase 3：实现主窗口、棋盘渲染、候选点、PV、胜率/目差图、GTP 控制台。
- Phase 4：实现 analysis JSON 批量分析、进度、取消、结果缓存和写回。
- Phase 5：实现人机对局、双引擎对比、引擎对局、时间/visits/playouts/PDA/WRN 设置。
- Phase 6：高 DPI、主题、语言、Windows 打包、Linux AppImage 或 tar 包、KDE Wayland 验收。
- Phase 7：迁移验证，对照 Java 版功能清单，补齐首版范围内遗漏项，编写用户迁移说明。

## Test Plan

- 单测：坐标转换、提子、劫、pass/resign、SGF 读写、分支编辑、分析数据合并。
- 协议测试：用 fixture 覆盖 kata-analyze 文本输出、analysis JSON、stderr 错误、异常退出。
- 集成测试：环境变量提供真实 KataGo 路径时，跑最小 9 路局面分析和一盘短自对局。
- UI 测试：4K、150%/200% 缩放、Wayland、Windows、窗口恢复、多显示器、深浅主题。
- 性能测试：大 SGF、长 PV、高频 KataGo 输出、批量 100 个节点分析时 UI 不阻塞。

## Acceptance Criteria

- KDE Wayland + NVIDIA 和 Windows + NVIDIA 都能启动、配置 KataGo、实时分析当前局面。
- 19 路 4K 屏幕下棋盘、文字、候选点、图表清晰不糊、不重叠。
- SGF 基本读写与分支导航可靠，打开保存后不丢主线和常见属性。
- KataGo 候选点、PV、胜率、目差、ownership 与 KataGo 原始输出一致。
- 批量分析可取消、可恢复 UI、失败节点有诊断信息。
- 首版范围内不出现 Java 版 Swing/AWT 依赖。

## Assumptions And Sources

- 默认 Qt 6 LTS 版本，具体小版本以开发时可用 SDK 和目标发行平台为准。
- KataGo 和模型由用户提供路径，不随应用包分发。
- 参考资料：KataGo GTP Extensions、KataGo Analysis Engine、Qt High DPI 文档、Qt Supported Platforms。
