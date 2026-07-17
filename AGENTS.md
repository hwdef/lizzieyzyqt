# AGENTS.md — LizzieYzy Qt Developer Notes

## Architecture

```
src/
├── core/    棋盘规则、SGF 解析/写入、分支树、对局数据模型（不依赖 Qt UI）
├── engine/  KataGo QProcess 会话、GTP 协议、analysis JSON、命令队列、日志
├── app/     应用状态、配置、会话恢复、快捷键、动作分发、文件导入导出
├── ui/      Widgets 主窗口、Dock 面板、设置页、GTP 控制台、图表、棋盘 Quick 渲染
└── main.cpp
```

详情参见仓库根目录 [PLAN.md](PLAN.md)。

## Build

```bash
# Debug
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build build
ctest --test-dir build --parallel 2 --output-on-failure

# Release
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

**依赖**: C++20 编译器、CMake、Qt 6 (Core/Widgets/Quick/QuickWidgets)、Ninja（或其他 CMake generator）。
首版仅支持 Linux 和 Windows，macOS 在 CMake 阶段会被拦截。

## Format

仓库根目录有 `.clang-format`，从根目录运行 `clang-format --style=file` 以保证风格一致。

## Install & Package

```bash
cmake --install build --prefix "$PWD/install"
cmake --build build --target package
```

CPack 产出:
- Linux: `LizzieYzyQt-<version>-Linux.tar.gz`
- Windows: `LizzieYzyQt-<version>-Windows.zip`

Windows 包通过 Qt CMake 部署工具自动包含 Qt 运行时库和插件。
Linux `.tar.gz` 依赖目标系统 Qt/KDE 运行时，不捆绑发行版插件。
Katago、模型和配置文件由用户提供，不随包分发。

包验证脚本拒绝以下内容:
- Linux 包中的 Windows 运行时、Windows 包中的 Linux 运行时
- Java 归档、字节码、捆绑 JRE、非 Katago 引擎
- macOS app bundle / dylib / framework
- 远程 SSH 引擎、在线棋盘集成、屏幕同步、分布式训练相关产物

## CLI Diagnostics

安装在或打包的 `lizzieyzy_qt` 支持以下诊断参数:

### `--diagnostics`
打印完整的运行时环境诊断：Qt 平台插件、Quick 图形 API、OpenGL 上下文、应用路径、
QStandardPaths 可写位置、字体/区域/外观、主窗口 UI 结构、QSettings 位置、
已保存的引擎/分析/棋盘显示/文件行为/快捷键设置、窗口/会话恢复键、
当前平台插件候选、Wayland/Windows 平台插件存在性、Qt 构建/运行时版本、
用户/系统/显示环境变量、QML 导入路径、Quick Controls 配置、OS 详情、
Qt ABI/CPU 架构、屏幕几何/DPI/缩放、扩展图形环境诊断（NVIDIA/CUDA/Vulkan/EGL）、
PATH/LD_LIBRARY_PATH 等环境变量路径解析。

PATH 状态扩展：每个路径列表环境变量显示入口计数和每项的文件系统状态（exists/type/readable/writable 等）。
空白路径项报告 `hasText: false`，不解析为文件系统位置。
未设置的变量显示 `(unset)`，已设置但为空的显示 `(empty)`。

`plan.acceptance.*` 字段覆盖: 目标平台、KataGo 环境就绪、引擎配置、显示器候选、
实时/批量分析接受候选、双引擎支持、各模式最终阻塞诊断、手动 UI 检查清单、
原始 Katago 对比检查清单、外部验证清单、手动验证要求。

### `--target-acceptance-report`
将 `plan.acceptance.*` 字段输出为简明 Markdown 报告，
包含就绪状态、阻塞项和检查清单名称。

### `--target-acceptance-record-template`
打印用于记录手动验收结果的完整 INI 模板（在 Qt 平台初始化之前运行）。

### `--target-acceptance-record <ini>`
将已填写的手动验收 INI 记录注入诊断/报告输出。
也可通过 `LIZZIE_TARGET_ACCEPTANCE_RECORD_FILE` 环境变量设置。

记录文件包含 `appVersion` 和 `appExecutableSha256`，必须与当前运行时匹配。
`osPrettyName`、`osKernel*`、`qtRuntimeVersion`、`qtBuildAbi`、`cpuArchitecture`、
`targetMachine` 必须一致，防止记录跨环境复用。
证据文件修改时间不能晚于 `completedUtc` 超过内置的时钟偏差容限。
截图证据还要求 4K 像素包络。
SHA-256 证据固定值缺失/不匹配会导致 `acceptanceEvidenceSha256` 阻塞。

## Testing

### 单元测试

```bash
ctest --test-dir build --parallel 2 --output-on-failure
```

### Katago 集成测试

`lizzie_katago_integration_tests` 默认跳过。设置以下环境变量启用:

```bash
export LIZZIE_KATAGO_EXECUTABLE=/path/to/katago
export LIZZIE_KATAGO_MODEL=/path/to/model.bin.gz
export LIZZIE_KATAGO_ANALYSIS_CONFIG=/path/to/analysis.cfg
export LIZZIE_KATAGO_GTP_CONFIG=/path/to/gtp.cfg
ctest --test-dir build -R lizzie_katago_integration_tests --output-on-failure
```

`LIZZIE_KATAGO_TIMEOUT_MS` 可覆盖默认 30000ms 超时。

- Analysis config: 提交 9x9 让子根节点到批量规划器，验证候选着点、visit 数和 ownership 形状。
- GTP config: 通过实时同步路径重放 9x9 让子局面，验证 `kata-analyze` 输出，
  确认 9x13 `rectangular_boardsize` 同步，启动双引擎实时对比，
  生成两步 9x9 自对弈着点。

`lizzie_katago_integration_preflight_tests` 无需真实引擎，
验证无效路径在进程启动前正确报告状态。

### 覆盖率映射

- [docs/PlanCoverage.md](docs/PlanCoverage.md): 自动化测试和打包检查覆盖的 PLAN.md 需求映射
- [docs/PlanRequirementAudit.md](docs/PlanRequirementAudit.md): 逐条需求完成审计
- [docs/Verification.md](docs/Verification.md): 需要真实 Katago 安装、KDE Wayland 或 Windows 硬件的目标平台验证

## Migration

Java 版配置一次性导入行为、旧 LZ/LZOP SGF 分析导入、
Qt `LZYERR` 失败节点诊断、sidecar 分析文件，详见 [docs/Migration.md](docs/Migration.md)。
