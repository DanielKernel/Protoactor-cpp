# 跨架构编译指南

## 支持的架构

ProtoActor C++完全支持在ARM64和x86_64架构的Linux服务器上编译运行：
- ✅ **x86_64** (64位Intel/AMD)
- ✅ **ARM64** (AArch64, ARM服务器)

**注意**: 
- 本项目仅支持Linux平台，不支持Windows或macOS构建
- 仅支持64位架构，不支持32位系统

## 快速开始

```bash
# 使用构建脚本（推荐）
./build.sh

# 指定架构
./build.sh --arch x86_64
./build.sh --arch arm64

# Debug模式
./build.sh --debug
```

## 交叉编译

### 在x86_64服务器上编译ARM64版本

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

### 在ARM64服务器上编译x86_64版本

```bash
# 安装交叉编译工具链
sudo apt-get install gcc-x86-64-linux-gnu g++-x86-64-linux-gnu

# 配置CMake
cmake .. \
  -DCMAKE_SYSTEM_NAME=Linux \
  -DCMAKE_SYSTEM_PROCESSOR=x86_64 \
  -DCMAKE_C_COMPILER=x86-64-linux-gnu-gcc \
  -DCMAKE_CXX_COMPILER=x86-64-linux-gnu-g++

# 构建
make -j$(nproc)
```

## 架构特定优化

### x86_64优化
- 自动检测并启用SSE/AVX指令
- 使用x86特定的pause指令优化自旋循环
- Release模式下使用`-march=native`优化

### ARM64优化
- 使用ARM特定的yield指令
- 优化的内存对齐
- 支持ARMv8-A指令集

## 验证构建

```bash
# 检查二进制架构
file build-*/bin/hello_world

# 运行测试
./build-x86_64-Release/bin/hello_world
./build-arm64-Release/bin/hello_world
```

## 常见问题

**Q: 如何在ARM服务器上编译？**  
A: 直接在ARM64服务器上运行 `./build.sh --arch arm64`，或从x86_64交叉编译。

**Q: 性能在不同架构上有差异吗？**  
A: 核心逻辑相同，性能差异主要来自CPU本身的性能差异。

**Q: 支持哪些Linux发行版？**  
A: 支持所有主流Linux发行版（Ubuntu、Debian、CentOS、RHEL、Fedora等）。

详细构建说明请查看 `BUILD_GUIDE.md`
