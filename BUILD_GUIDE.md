# Linux服务器构建指南

## 系统要求

- **操作系统**: Linux (Ubuntu, Debian, CentOS, RHEL, Fedora等)
- **架构**: x86_64 或 ARM64 (仅支持64位)
- **编译器**: GCC 4.8+ 或 Clang 3.3+
- **CMake**: 3.10+
- **依赖库**: pthread (系统自带)

## 快速构建

### 方法1: 使用构建脚本（推荐）

```bash
# 构建当前架构
./build.sh

# 构建指定架构（64位）
./build.sh --arch x86_64    # Intel/AMD 64位
./build.sh --arch arm64     # ARM 64位

# Debug模式
./build.sh --debug

# 清理并重新构建
./build.sh --clean

# 查看帮助
./build.sh --help
```

### 方法2: 使用Makefile

```bash
# 默认构建
make

# 指定架构（64位）
make ARCH=x86_64
make ARCH=arm64

# Debug构建
make BUILD_TYPE=Debug

# 清理
make clean
```

### 方法3: 使用CMake直接构建

```bash
mkdir build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
```

## 交叉编译（64位架构）

### 在x86_64服务器上编译ARM64版本

```bash
# 1. 安装交叉编译工具链
sudo apt-get update
sudo apt-get install -y gcc-aarch64-linux-gnu g++-aarch64-linux-gnu

# 2. 配置CMake
mkdir build-arm64
cd build-arm64
cmake .. \
  -DCMAKE_SYSTEM_NAME=Linux \
  -DCMAKE_SYSTEM_PROCESSOR=aarch64 \
  -DCMAKE_C_COMPILER=aarch64-linux-gnu-gcc \
  -DCMAKE_CXX_COMPILER=aarch64-linux-gnu-g++ \
  -DCMAKE_BUILD_TYPE=Release

# 3. 构建
make -j$(nproc)
```

### 在ARM64服务器上编译x86_64版本

```bash
# 1. 安装交叉编译工具链
sudo apt-get update
sudo apt-get install -y gcc-x86-64-linux-gnu g++-x86-64-linux-gnu

# 2. 配置CMake
mkdir build-x86_64
cd build-x86_64
cmake .. \
  -DCMAKE_SYSTEM_NAME=Linux \
  -DCMAKE_SYSTEM_PROCESSOR=x86_64 \
  -DCMAKE_C_COMPILER=x86-64-linux-gnu-gcc \
  -DCMAKE_CXX_COMPILER=x86-64-linux-gnu-g++ \
  -DCMAKE_BUILD_TYPE=Release

# 3. 构建
make -j$(nproc)
```

## 架构特定优化

### x86_64优化

Release模式下会自动启用：
- `-march=native` (如果编译器支持，否则使用`-march=x86-64`)
- SSE/AVX指令集优化
- x86特定的pause指令优化自旋循环

### ARM64优化

- ARMv8-A指令集 (`-march=armv8-a`)
- ARM特定的yield指令优化自旋循环
- 优化的内存对齐

## 验证构建

### 检查二进制架构

```bash
# 查看二进制文件信息
file build-*/bin/hello_world

# x86_64应该显示：
# ELF 64-bit LSB executable, x86-64, version 1 (SYSV), dynamically linked

# ARM64应该显示：
# ELF 64-bit LSB executable, ARM aarch64, version 1 (SYSV), dynamically linked
```

### 运行示例

```bash
# 运行Hello World示例
./build-x86_64-Release/bin/hello_world

# 运行监督示例
./build-x86_64-Release/bin/supervision_example
```

## 常见问题

### Q: 如何检查当前系统架构？
A:
```bash
uname -m
# 输出: x86_64, aarch64, armv7l等
```

### Q: 编译时出现"找不到编译器"错误？
A: 确保已安装GCC或Clang：
```bash
# Ubuntu/Debian
sudo apt-get install build-essential

# CentOS/RHEL
sudo yum install gcc gcc-c++ make cmake
```

### Q: 如何指定编译器？
A:
```bash
cmake .. -DCMAKE_C_COMPILER=gcc -DCMAKE_CXX_COMPILER=g++
# 或使用Clang
cmake .. -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++
```

### Q: 如何启用更多优化？
A: 在Release模式下，CMake会自动启用优化。也可以手动指定：
```bash
cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_FLAGS="-O3 -march=native"
```

## 生产环境部署

### 最小化依赖

项目只依赖系统标准库，可以静态链接：

```bash
cmake .. -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_EXE_LINKER_FLAGS="-static-libgcc -static-libstdc++"
```

### Docker部署

```bash
# 构建Docker镜像
docker build --build-arg TARGET_ARCH=x86_64 -t protoactor-cpp:latest .

# 运行
docker run protoactor-cpp:latest
```

## 性能调优

### 编译选项

```bash
# 启用所有优化
cmake .. -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_CXX_FLAGS="-O3 -march=native -mtune=native"

# 启用链接时优化
cmake .. -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_CXX_FLAGS="-flto" \
  -DCMAKE_EXE_LINKER_FLAGS="-flto"
```

### 运行时优化

- 使用Release模式构建
- 根据目标CPU架构选择编译选项
- 考虑使用静态链接减少依赖

## 总结

项目专为Linux服务器环境设计，支持在ARM和x86架构上编译运行。所有构建工具和脚本都针对Linux进行了优化，确保最佳的构建体验和运行时性能。
