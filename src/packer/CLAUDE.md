[根目录](../../CLAUDE.md) > [src](..) > **packer**

# Packer -- 打包器

## 模块职责

将 AST 序列化为二进制字节码，收集并打包游戏资源，输出 `.nvmp` 格式的单一二进制归档文件。同时提供 `.nvmp` 读取功能，供 VM 加载。

## 入口与启动

- **打包入口：** `nova::Packer`（`include/nova/packer/packer.h`）
- **便捷函数：** `packProject(scriptDir, assetDir, outputPath)`
- **AST 构建：** `buildCombinedProgramFromScripts()` / `buildCombinedProgramFromPath()`

## 对外接口

### Packer（打包流程）

```
1. addScript(path) / addScriptDirectory(path)  -- 添加脚本
2. setAssetDirectory(path)                      -- 设置资源目录
3. setOutputPath(path)                          -- 设置输出路径
4. setMetadata(metadata)                        -- 设置游戏元数据
5. pack()                                       -- 执行打包 -> PackResult
```

### .nvmp 格式 (`nvmp_format.h`)

- **版本：** `NVMP_VERSION = 3`
- **魔术字：** `"NOVA"`（4 字节）
- **结构：** Header (32B) + Index + AST Bytecode + Data Section
- **资源类型：** `Image(0)`, `Audio(1)`, `Font(2)`, `Video(3)`, `Other(255)`

### 字节码格式

- **操作码 (`OpCode`)：** 节点类型标记 (0-34) + 字面量类型 (100-103) + 控制标记 (200-201)
- **写入器：** `BytecodeWriter` -- 支持 byte/u16/u32/u64/float/double/string 写入
- **读取器：** `BytecodeReader` -- 对应的读取操作

### 序列化/反序列化

- **AstSerializer：** `serialize(ProgramNode*) -> vector<uint8_t>`
- **AstDeserializer：** `deserialize(vector<uint8_t>) -> unique_ptr<ProgramNode>`
  - 接受 `package_version` 参数进行版本兼容处理

### 资源打包

- **AssetBundler：** 收集资源文件，构建索引表和数据区
- **NvmpWriter / NvmpReader：** `.nvmp` 文件的写入和读取

## 关键依赖与配置

- **上游依赖：** `nova-core`、`nova-ast`（AST 节点）
- **下游消费者：** `nova-cli`（build 命令）、`nova-renderer`（加载 .nvmp）
- **CMake 目标：** `nova-packer`（静态库）

## 数据模型

### PackResult

```cpp
struct PackResult {
    bool success;
    string outputPath, error;
    size_t assetCount, bytecodeSize, totalSize;
};
```

### MemoryScript

```cpp
struct MemoryScript {
    string path;
    string content;
};
```

## 测试与质量

- 间接测试通过 `tests/vm_test.cpp`（加载 .nvmp 并执行）和 `tests/c_api_test.cpp`

## 相关文件清单

| 文件 | 说明 |
|------|------|
| `include/nova/packer/packer.h` | Packer 类与便捷函数 |
| `include/nova/packer/nvmp_format.h` | .nvmp 格式定义、OpCode、BytecodeWriter/Reader |
| `include/nova/packer/nvmp_writer.h` | NvmpWriter / NvmpReader |
| `include/nova/packer/ast_serializer.h` | AstSerializer / AstDeserializer |
| `include/nova/packer/asset_bundler.h` | AssetBundler |
| `src/packer/packer.cpp` | Packer 实现 |
| `src/packer/nvmp_format.cpp` | 字节码读写器实现 |
| `src/packer/nvmp_writer.cpp` | .nvmp 文件写入/读取实现 |
| `src/packer/ast_serializer.cpp` | AST 序列化/反序列化实现 |
| `src/packer/asset_bundler.cpp` | 资源打包实现 |
| `src/packer/CMakeLists.txt` | CMake 构建配置 |

## 变更记录 (Changelog)

| 时间 | 操作 | 说明 |
|------|------|------|
| 2026-05-11 08:41:29 | 初始化 | 由 init-architect 自动生成 |
