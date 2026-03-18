---
title: "安装指南"
weight: 1
---

# 安装指南

## 系统要求

- C++17 兼容编译器 (GCC 9+, Clang 10+, MSVC 2019+)
- CMake 3.16+
- vcpkg (可选，用于依赖管理)

## 从源码构建

### 1. 克隆仓库

```bash
git clone https://github.com/Okysu/NovaMark.git
cd novamark
```

### 2. 安装依赖

使用 vcpkg:

```bash
# 安装 vcpkg (如果尚未安装)
git clone https://github.com/Microsoft/vcpkg.git
./vcpkg/bootstrap-vcpkg.sh

# 安装依赖
./vcpkg/vcpkg install
```

### 3. 构建项目

```bash
# 配置
cmake -B build -S . -DCMAKE_TOOLCHAIN_FILE=[vcpkg]/scripts/buildsystems/vcpkg.cmake

# 构建
cmake --build build --config Release
```

### 4. 运行测试

```bash
ctest --test-dir build --output-on-failure
```

## 验证安装

```bash
./build/src/cli/nova-cli --version
```

输出应显示版本信息。

## 下一步

- [快速入门](../quickstart/) - 创建你的第一个游戏
