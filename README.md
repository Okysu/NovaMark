<div align="center">

<h1>NovaMark</h1>

<p><strong>专为文字游戏、互动小说与视觉小说设计的领域专用脚本语言及运行时引擎。</strong></p>

<p>
  <a href="#architecture">架构设计</a> &middot;
  <a href="#syntax">语法示例</a> &middot;
  <a href="#quickstart">快速开始</a> &middot;
  <a href="#platforms">平台接入</a> &middot;
  <a href="#roadmap">路线图</a>
</p>

<p>
  <img src="https://img.shields.io/badge/版本-v0.1_alpha-blue" alt="版本">
  <img src="https://img.shields.io/badge/许可证-MIT-green" alt="许可证">
  <img src="https://img.shields.io/badge/C%2B%2B-17-orange" alt="C++17">
</p>

</div>

---

NovaMark 通过严格的 VM/渲染器边界将叙事逻辑与渲染层分离，使同一个编译后的游戏包无需修改即可运行于 Web、桌面和移动平台。

> **文档：** 完整的语法规范、架构说明与 API 参考，请查阅 [NovaMark 官方文档](https://novamark.example.com)（文档源码位于 `docs/`，基于 Hugo 构建）。

---

## 架构与设计原则 <a id="architecture"></a>

现有的视觉小说引擎大多将游戏逻辑与特定渲染层紧密耦合——Ren'Py 脚本依赖 Ren'Py 运行时；Ink 在处理文本以外的媒体时需要大量集成工作。NovaMark 采取不同的立场：**引擎核心是一个不含任何渲染依赖的纯粹状态机**，渲染器被设计为"哑"的——它只接收状态快照并将其绘制出来。

这种分离带来了具体的工程收益：

<table>
<thead>
<tr><th>收益</th><th>说明</th></tr>
</thead>
<tbody>
<tr>
  <td><strong>可移植的游戏包</strong></td>
  <td>编译后的 <code>.nvmp</code> 文件将所有脚本、资产和元数据打包进单一二进制归档。分发时不需要额外的工具链或资产管理流程。</td>
</tr>
<tr>
  <td><strong>可替换的渲染器</strong></td>
  <td>同一个游戏包可以运行在用于调试的终端渲染器、通过 WebAssembly 运行的浏览器，或通过 C FFI 接入的原生应用上。新增目标平台不需要修改游戏本身或引擎核心。</td>
</tr>
<tr>
  <td><strong>可序列化的状态</strong></td>
  <td>所有游戏状态均通过单一的 <code>NovaState</code> 结构体流转。存档、读档、回放和回溯均可在渲染器层实现，无需修改引擎。</td>
</tr>
</tbody>
</table>

### 数据流向

```
.nvmp 游戏包
  ├── AST 字节码
  ├── 资源索引表（偏移量记录）
  └── 二进制资源数据（图片、音频、字体）
          |
          v
    NovaMark VM
  (执行 AST → 维护离散状态机 → 监听外部输入)
          |
    ┌─────┴──────┬──────────────┐
    v            v              v
Text Mode    Web Chat Mode   Web VN Mode
(CLI 调试)    (WASM 渲染器)   (WASM 渲染器)
```

### 状态契约

渲染器在每次 VM 状态推演后通过 `vm.state()` 获取当前快照，
随后调用 `vm.advance()` 步进，或调用 `vm.choose(id)` 选择分支，驱动主循环。

```typescript
interface NovaState {
  bg: string | null;
  bgm: string | null;
  sfx: Array<{ id: string; path: string; loop: boolean }>;
  sprites: Array<{ id: string; url: string; x: number; y: number; opacity: number; zIndex: number }>;
  dialogue: { isShow: boolean; name: string; text: string; color: string } | null;
  choices: Array<{ id: string; text: string; disabled: boolean }>;
}
```

---

## 语法示例 <a id="syntax"></a>

NovaMark 采用基于缩进与指令标记的极简语法，支持变量运算与分支控制流。

```novamark
---
title: 示例场景
version: 1.0
---

@char 小明
  color: #4A90D9
  sprite_default: xiaoming_normal.png
@end

@var affection = 0

#scene_start "序章"

@bg room.png
@bgm peaceful.mp3

> 这是一个普通的早晨。

小明: 早安！今天天气真好。

? 你要怎么回应？
- [回以微笑] -> .smile
- [保持沉默] -> .silent

.smile
小明: 看来你心情不错呢。
@set affection = affection + 10
-> scene_next

.silent
小明: ...怎么了？
-> scene_next
```

---

## 快速开始 <a id="quickstart"></a>

### 构建环境要求

- CMake 3.16 或更高版本
- 支持 C++17 标准的编译器（GCC、Clang 或 MSVC）
- vcpkg 包管理器

### 源码编译

```bash
cmake -B build -S . -DCMAKE_TOOLCHAIN_FILE=[vcpkg_path]/scripts/buildsystems/vcpkg.cmake
cmake --build build --config Release
ctest --test-dir build --output-on-failure
```

### CLI 工具使用

编译器与调试器集成于 `nova-cli` 工具中：

```bash
# 语法检查与静态分析
./build/src/cli/nova-cli check scripts/

# 编译脚本资源为二进制包
./build/src/cli/nova-cli build scripts/ -o game.nvmp

# 在 Text Mode 下启动调试运行
./build/src/cli/nova-cli run game.nvmp
```

**Text Mode 调试热键：** `Enter` — 步进 &middot; `1`–`9` — 选项分支 &middot; `S` — 创建快照（存档）&middot; `L` — 恢复快照（读档）&middot; `Q` — 中断进程

---

## 平台接入机制 <a id="platforms"></a>

NovaMark 为上层应用提供两种标准化接入路径：

### WebAssembly

VM 通过 Emscripten 编译并暴露 JavaScript API。项目 `template/web/` 目录下提供了两种主流布局的参考渲染器（聊天模式与视觉小说模式），建议作为 Web 部署的起点。

### C API / 原生 FFI

针对原生目标平台，VM 暴露标准的 C API 接口，可被任何支持 C FFI 的语言（如 Rust、C#、Python）调用。参考实现的渲染器代码量均控制在 200 行以内，便于直接 Fork 改造。

---

## 项目状态与路线图 <a id="roadmap"></a>

**当前版本：v0.1 (Alpha)**

### 核心特性矩阵

<table>
<thead>
<tr><th>模块</th><th>特性</th><th>状态</th></tr>
</thead>
<tbody>
<tr>
  <td rowspan="3"><strong>语言解析</strong></td>
  <td>角色定义、场景声明、标签跳转</td>
  <td>稳定</td>
</tr>
<tr>
  <td>变量运算、骰子表达式（<code>2d6+3</code>）、条件判断（<code>@if</code> / <code>@else</code>）</td>
  <td>稳定</td>
</tr>
<tr>
  <td>物品系统（<code>@give</code> / <code>@take</code>）、多结局支持、子程序（<code>@call</code>）</td>
  <td>稳定</td>
</tr>
<tr>
  <td rowspan="3"><strong>运行时</strong></td>
  <td>Text Mode 终端渲染器（开发与调试）</td>
  <td>稳定</td>
</tr>
<tr>
  <td>WASM 编译支持与 Web 渲染器模板</td>
  <td>稳定</td>
</tr>
<tr>
  <td>跨语言 C API 暴露</td>
  <td>稳定</td>
</tr>
</tbody>
</table>

### 迭代规划

**v0.2 — 渲染器增强**
- [ ] 支持状态快照缩略图生成
- [ ] 历史文本回溯系统
- [ ] 自动化步进与跳过逻辑

**v0.3 — 工具链完善**
- [ ] NovaMark Creator（可视化节点编辑器与打包 GUI 工具）
- [ ] 原生平台渲染器参考实现（iOS / Android / HarmonyOS）

**v1.0 — 生产就绪**
- [ ] 语法规范冻结与向后兼容承诺
- [ ] 性能剖析基准测试与内存分配优化
- [ ] 完整 API Reference 覆盖

---

## 依赖树与代码库结构

为保证引擎在移动端和嵌入式环境中的极低开销，NovaMark 严格限制第三方依赖，不引入 Boost 或 Qt 等重型框架。

<table>
<thead>
<tr><th>组件库</th><th>用途说明</th><th>分发策略</th></tr>
</thead>
<tbody>
<tr>
  <td>nlohmann/json</td>
  <td>状态机快照与配置解析</td>
  <td>Header-only，随源码分发</td>
</tr>
<tr>
  <td>GoogleTest</td>
  <td>核心逻辑单元测试</td>
  <td>仅开发环境依赖</td>
</tr>
</tbody>
</table>

### 核心目录结构

```
NovaMark/
├── src/           # 引擎核心代码
│   ├── lexer/     # 词法分析器
│   ├── parser/    # 语法分析器
│   ├── ast/       # 抽象语法树定义
│   ├── semantic/  # 语义检查
│   ├── vm/        # 状态机与运行时环境
│   ├── packer/    # 资源打包模块
│   └── cli/       # 命令行入口
├── include/nova/  # 公开 C++ Headers 与 C API
├── tests/         # 单元测试与集成测试例程
├── docs/          # 文档站点源码
├── examples/      # 各类语法的示例工程
└── template/      # 各平台参考渲染器实现
```

---

## 参与贡献

欢迎通过 Issue 提交异常报告或功能提案。涉及架构调整、语法变更的 Pull Request，请务必先创建 Issue 进行技术方案评审。代码提交需通过全部 CMake CTest 校验。

## 许可证

本项目基于 [MIT License](./LICENSE) 协议开源。