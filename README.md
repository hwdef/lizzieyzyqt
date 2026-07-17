# LizzieYzy Qt

![LizzieYzy Qt application screenshot](docs/screenshot.png)

基于 Qt 6 / C++20 的围棋分析工具，使用 KataGo 引擎提供实时局面分析、
批量复盘、候选着点、形势判断、胜率/目差图表、人机/引擎对局等功能。
是对 [Java 版 lizzieyzy](https://github.com/yzyray/lizzieyzy) 的 AI 辅助重构。

Linux 和 Windows 可用，macOS 首版暂不支持。

## 构建

```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

**依赖**: C++20 编译器、CMake、Qt 6 (Core/Widgets/Quick/QuickWidgets)、Ninja。

## 运行

```bash
./build/lizzieyzy_qt
```

安装后运行：

```bash
./install/bin/lizzieyzy_qt
```

### 首次启动

首次启动时，应用提供三种方式：

1. **配置引擎** — 提供本地 KataGo 可执行文件、模型、GTP 配置和分析配置的完整路径。
2. **导入 Java 配置** — 从旧 Java 版本的 `config.txt` 一次性导入引擎配置。
3. **无引擎模式** — 仅查看、编辑 SGF 棋谱，不连接引擎。

KataGo、模型和配置文件需用户自行提供，不随应用分发。

## 安装与打包

```bash
cmake --install build --prefix "$PWD/install"
cmake --build build --target package
```

CPack 生成 `LizzieYzyQt-<version>-Linux.tar.gz` 或 `LizzieYzyQt-<version>-Windows.zip`。

## 更多文档

- [AGENTS.md](AGENTS.md) — 开发指南：项目架构、测试说明、CLI 诊断参数、打包验证规则
- [PLAN.md](PLAN.md) — 原始重构计划与验收标准
- [docs/Migration.md](docs/Migration.md) — 从 Java 版迁移指南
- [docs/Verification.md](docs/Verification.md) — 目标平台验证清单

### 集成测试（需要真实 KataGo）

```bash
export LIZZIE_KATAGO_EXECUTABLE=/path/to/katago
export LIZZIE_KATAGO_MODEL=/path/to/model.bin.gz
export LIZZIE_KATAGO_ANALYSIS_CONFIG=/path/to/analysis.cfg
export LIZZIE_KATAGO_GTP_CONFIG=/path/to/gtp.cfg
ctest --test-dir build -R lizzie_katago_integration_tests --output-on-failure
```

## License

[LICENSE](LICENSE)
