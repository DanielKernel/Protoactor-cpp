# Linux服务器构建总结

## 项目定位

ProtoActor C++专为**Linux服务器环境**设计，支持在ARM和x86架构上编译运行。

## 支持的架构（64位）

| 架构 | 说明 | 示例平台 |
|------|------|----------|
| x86_64 | 64位Intel/AMD | 标准服务器 |
| ARM64 | AArch64 | ARM服务器、云实例 |

**注意**: 仅支持64位架构，不支持32位系统。

## 构建方式

### 1. 构建脚本（最简单）

```bash
./build.sh --arch x86_64
./build.sh --arch arm64
```

### 2. Makefile

```bash
make ARCH=x86_64
make ARCH=arm64
```

### 3. CMake直接构建

```bash
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
```

## 交叉编译

支持在x86服务器上编译ARM版本，或在ARM服务器上编译x86版本：

```bash
# x86 → ARM64
sudo apt-get install gcc-aarch64-linux-gnu g++-aarch64-linux-gnu
cmake .. -DCMAKE_SYSTEM_PROCESSOR=aarch64 \
  -DCMAKE_C_COMPILER=aarch64-linux-gnu-gcc \
  -DCMAKE_CXX_COMPILER=aarch64-linux-gnu-g++
```

## 系统要求

- **操作系统**: Linux (任何主流发行版)
- **编译器**: GCC 4.8+ 或 Clang 3.3+
- **CMake**: 3.10+
- **依赖**: pthread (系统自带)

## 优化特性

- **x86_64**: 自动启用SSE/AVX优化，使用pause指令优化自旋循环
- **ARM64**: ARMv8-A指令集优化，使用yield指令优化自旋循环
- **所有架构**: 统一的C++11内存模型

## 完成状态

✅ **90%完成** - 核心功能全部实现，专为Linux服务器环境优化
