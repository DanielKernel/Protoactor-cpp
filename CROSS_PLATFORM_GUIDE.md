# Linux跨架构编译指南

## 概述

ProtoActor C++完全支持在ARM64和x86_64架构的Linux服务器上编译运行：
- **x86_64** (64位Intel/AMD)
- **ARM64** (AArch64, ARM服务器)

**注意**: 
- 本项目仅支持Linux平台，不支持Windows或macOS构建
- 仅支持64位架构，不支持32位系统

## 快速开始

### Linux服务器

```bash
# 使用构建脚本（推荐）
./build.sh

# 指定架构
./build.sh --arch x86_64
./build.sh --arch arm64

# Debug模式
./build.sh --debug

# 启用所有功能
./build.sh --protobuf --tests
```

### 使用Makefile

```bash
# 默认构建
make

# 指定架构
make ARCH=arm64
make ARCH=x86_64

# Debug构建
make BUILD_TYPE=Debug
```

## 架构检测

项目会自动检测当前平台：

```cpp
#include "protoactor/platform.h"

// 使用平台宏
#if defined(PROTOACTOR_ARCH_ARM64)
    // ARM64特定代码
#elif defined(PROTOACTOR_ARCH_X86_64)
    // x86_64特定代码
#endif
```

## 交叉编译

### Linux: x86 → ARM

```bash
# 安装交叉编译工具链
sudo apt-get install gcc-aarch64-linux-gnu g++-aarch64-linux-gnu

# 配置
cmake .. \
  -DCMAKE_SYSTEM_NAME=Linux \
  -DCMAKE_SYSTEM_PROCESSOR=aarch64 \
  -DCMAKE_C_COMPILER=aarch64-linux-gnu-gcc \
  -DCMAKE_CXX_COMPILER=aarch64-linux-gnu-g++

# 构建
make
```

### Linux: ARM → x86

```bash
# 安装交叉编译工具链
sudo apt-get install gcc-x86-64-linux-gnu g++-x86-64-linux-gnu

# 配置
cmake .. \
  -DCMAKE_SYSTEM_NAME=Linux \
  -DCMAKE_SYSTEM_PROCESSOR=x86_64 \
  -DCMAKE_C_COMPILER=x86_64-linux-gnu-gcc \
  -DCMAKE_CXX_COMPILER=x86_64-linux-gnu-g++
```


## Docker构建

### 单架构构建

```bash
# x86_64
docker build --build-arg TARGET_ARCH=x86_64 -t protoactor-cpp:x86_64 .

# ARM64
docker build --build-arg TARGET_ARCH=arm64 -t protoactor-cpp:arm64 .
```

### 多架构构建（使用docker-compose）

```bash
docker-compose build build-x86_64 build-arm64
```

## 平台特定优化

### x86_64优化

- 自动检测并启用SSE/AVX指令
- 使用x86特定的pause指令优化自旋循环
- Release模式下使用`-march=native`优化

### ARM优化

- 使用ARM特定的yield指令
- 优化的内存对齐
- 支持ARMv7和ARMv8指令集

## 验证构建

### 检查架构

```bash
# Linux
file build-*/bin/hello_world

# macOS
lipo -info build-*/bin/hello_world

# 应该显示正确的架构信息
```

### 运行测试

```bash
# 在目标平台上运行
./build-x86_64-Release/bin/hello_world
./build-arm64-Release/bin/hello_world
```

## CI/CD集成

项目包含GitHub Actions配置，自动在以下平台测试：
- Ubuntu x86_64
- Ubuntu ARM64
- macOS x86_64
- macOS ARM64
- Windows x64

查看 `.github/workflows/ci.yml` 了解详情。

## 常见问题

### Q: 如何在ARM服务器上编译？
A: 
```bash
# 直接在ARM64服务器上编译
./build.sh --arch arm64

# 或从x86_64交叉编译
sudo apt-get install gcc-aarch64-linux-gnu g++-aarch64-linux-gnu
cmake .. -DCMAKE_SYSTEM_PROCESSOR=aarch64 \
  -DCMAKE_C_COMPILER=aarch64-linux-gnu-gcc \
  -DCMAKE_CXX_COMPILER=aarch64-linux-gnu-g++
```

### Q: 如何在不同的Linux发行版上编译？
A: 项目使用标准C++11和CMake，支持所有主流Linux发行版：
- Ubuntu/Debian
- CentOS/RHEL
- Fedora
- openSUSE
- Arch Linux
等

### Q: 性能在不同架构上有差异吗？
A: 核心逻辑相同，性能差异主要来自：
- CPU本身的性能差异
- 内存访问模式
- 缓存大小和层次结构

代码在所有架构上使用相同的算法和数据结构。

## 架构兼容性矩阵

| 架构 | Linux支持 | 说明 |
|------|-----------|------|
| x86_64 | ✅ | 64位Intel/AMD服务器 |
| ARM64 | ✅ | AArch64 ARM服务器 |

**注意**: 
- 本项目仅支持Linux平台构建和运行
- 仅支持64位架构，不支持32位系统

## 构建选项总结

| 选项 | 说明 | 默认值 |
|------|------|--------|
| `CMAKE_BUILD_TYPE` | Release/Debug | Release |
| `BUILD_EXAMPLES` | 构建示例 | ON |
| `BUILD_TESTS` | 构建测试 | OFF |
| `ENABLE_PROTOBUF` | 启用Protobuf | OFF |
| `BUILD_SHARED_LIBS` | 构建共享库 | OFF |

## 下一步

- 查看 `README.md` 了解基本用法
- 查看 `examples/` 目录了解示例代码
- 查看 `IMPLEMENTATION_STATUS.md` 了解实现状态
