# Linux跨架构编译指南

ProtoActor C++支持在ARM64和x86_64架构的Linux服务器上编译运行。

## 支持的架构

- **x86_64** (64位Intel/AMD)
- **ARM64** (AArch64)

## 支持的操作系统

- **Linux** (Ubuntu, Debian, CentOS, RHEL, Fedora等)

**注意**: 
- 本项目仅支持Linux平台，专为Linux服务器环境设计
- 仅支持64位架构，不支持32位系统

## 快速开始

### Linux服务器

使用提供的构建脚本：

```bash
# 构建当前平台
./build.sh

# 构建指定架构（64位）
./build.sh --arch x86_64
./build.sh --arch arm64

# Debug模式
./build.sh --debug

# 清理并重新构建
./build.sh --clean

# 启用Protobuf支持
./build.sh --protobuf
```

### 手动构建

```bash
mkdir build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
```

## 交叉编译

### ARM64 on x86_64 (Linux)

```bash
# 安装交叉编译工具链
sudo apt-get install gcc-aarch64-linux-gnu g++-aarch64-linux-gnu

# 配置CMake
cmake .. \
  -DCMAKE_SYSTEM_NAME=Linux \
  -DCMAKE_SYSTEM_PROCESSOR=aarch64 \
  -DCMAKE_C_COMPILER=aarch64-linux-gnu-gcc \
  -DCMAKE_CXX_COMPILER=aarch64-linux-gnu-g++

# 构建
make -j$(nproc)
```

### x86_64 on ARM64 (Linux)

```bash
# 安装交叉编译工具链
sudo apt-get install gcc-x86-64-linux-gnu g++-x86-64-linux-gnu

# 配置CMake
cmake .. \
  -DCMAKE_SYSTEM_NAME=Linux \
  -DCMAKE_SYSTEM_PROCESSOR=x86_64 \
  -DCMAKE_C_COMPILER=x86_64-linux-gnu-gcc \
  -DCMAKE_CXX_COMPILER=x86_64-linux-gnu-g++

# 构建
make -j$(nproc)
```

## 架构特定说明

### ARM64架构

- 使用标准的C++11原子操作
- 支持ARMv8-A (AArch64)指令集
- 内存对齐遵循ARM规范
- 使用ARM特定的yield指令优化自旋循环
- 编译选项: `-march=armv8-a`

### x86_64架构

- 支持SSE/AVX优化（如果可用）
- 使用x86特定的pause指令优化自旋循环
- Release模式下可使用`-march=native`优化
- 编译选项: `-march=x86-64 -mtune=generic` 或 `-march=native`

### 内存模型

- 所有架构使用相同的C++11内存模型
- 原子操作在所有架构上保持一致
- 字节序：小端（Little Endian）

## 依赖项

### 必需
- **Linux操作系统**
- C++11编译器（GCC 4.8+, Clang 3.3+）
- CMake 3.10+
- pthread库

### 可选
- Protobuf (用于消息序列化)
- gRPC (用于远程通信)

## 测试

在不同架构上测试：

```bash
# Linux x86_64
./build.sh --arch x86_64 && ./build-x86_64-Release/bin/hello_world

# Linux ARM64
./build.sh --arch arm64 && ./build-arm64-Release/bin/hello_world

# 当前平台（自动检测）
./build.sh && ./build-native-Release/bin/hello_world
```

## 常见问题

### Q: 如何在ARM设备上运行x86编译的程序？
A: 不能直接运行，需要使用交叉编译或QEMU模拟。

### Q: 性能在不同架构上有差异吗？
A: 核心逻辑相同，但原子操作和内存屏障的实现可能因架构而异。性能差异主要来自CPU本身。

### Q: 支持哪些编译器？
A: GCC 4.8+, Clang 3.3+。推荐使用较新版本以获得更好的C++11支持。

### Q: 支持哪些Linux发行版？
A: 支持所有主流Linux发行版，包括Ubuntu、Debian、CentOS、RHEL、Fedora、openSUSE、Arch Linux等。

## CI/CD

项目包含GitHub Actions工作流，自动在Linux平台上测试：
- Ubuntu (x86_64, ARM64)
- Release和Debug模式

查看 `.github/workflows/ci.yml` 了解详情。
