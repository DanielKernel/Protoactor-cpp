# 构建指南

## 系统要求

- **操作系统**: Linux (Ubuntu, Debian, CentOS, RHEL, Fedora等)
- **架构**: x86_64 或 ARM64 (仅支持64位)
- **编译器**: GCC 4.8+ 或 Clang 3.3+
- **CMake**: 3.10+
- **依赖库**: pthread (系统自带)

## 依赖项说明

### 核心原则

ProtoActor C++采用**"使用成熟开源库，专注核心逻辑"**的策略：
- ✅ **使用开源库**: 网络、序列化、日志、测试等通用功能
- ✅ **自实现**: Actor系统核心逻辑，需要精确控制的部分

### 依赖分类

#### 必需依赖（核心功能）

| 库 | 用途 | 状态 |
|---|---|---|
| **pthread** | 线程支持 | ✅ 系统自带 |
| **C++11标准库** | 基础功能 | ✅ 编译器自带 |

#### 可选依赖（功能增强）

| 库 | 用途 | 推荐度 | 安装 |
|---|---|---|---|
| **gRPC** | 远程通信 | ⭐⭐⭐ 必需（远程功能） | `sudo apt-get install libgrpc++-dev protobuf-compiler-grpc` |
| **Protobuf** | 消息序列化 | ⭐⭐⭐ 必需（远程功能） | `sudo apt-get install libprotobuf-dev protobuf-compiler` |
| **spdlog** | 日志系统 | ⭐⭐ 强烈推荐 | 使用vcpkg或从源码编译 |
| **Google Test** | 测试框架 | ⭐⭐ 强烈推荐 | `sudo apt-get install libgtest-dev` |
| **nlohmann/json** | JSON支持 | ⭐ 可选 | `sudo apt-get install nlohmann-json3-dev` |

### 依赖安装

#### 快速安装（Ubuntu/Debian）

**必需依赖**:
```bash
sudo apt-get update
sudo apt-get install -y \
    build-essential \
    cmake \
    libprotobuf-dev \
    protobuf-compiler \
    libgrpc++-dev \
    protobuf-compiler-grpc \
    pkg-config
```

**推荐依赖**:
```bash
sudo apt-get install -y \
    libgtest-dev \
    libgmock-dev \
    nlohmann-json3-dev

# spdlog需要从源码编译或使用vcpkg
```

#### 使用vcpkg安装（推荐）

```bash
# 1. 安装vcpkg
cd ~
git clone https://github.com/Microsoft/vcpkg.git
cd vcpkg
./bootstrap-vcpkg.sh

# 2. 安装依赖
./vcpkg install \
    grpc \
    protobuf \
    spdlog \
    gtest \
    nlohmann-json

# 3. 配置CMake使用vcpkg
cmake .. \
  -DCMAKE_TOOLCHAIN_FILE=~/vcpkg/scripts/buildsystems/vcpkg.cmake \
  -DENABLE_PROTOBUF=ON \
  -DENABLE_GRPC=ON \
  -DENABLE_SPDLOG=ON \
  -DENABLE_GTEST=ON \
  -DENABLE_JSON=ON
```

#### 使用Conan安装

```bash
# 1. 安装Conan
pip install conan

# 2. 安装依赖
conan install . --build=missing

# 3. 配置CMake
cmake .. -DCMAKE_TOOLCHAIN_FILE=conan_toolchain.cmake
```

#### 从源码编译（高级用户）

**Protobuf**:
```bash
git clone https://github.com/protocolbuffers/protobuf.git
cd protobuf
git submodule update --init --recursive
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
sudo make install
```

**gRPC**:
```bash
git clone --recurse-submodules https://github.com/grpc/grpc.git
cd grpc
mkdir -p cmake/build
cd cmake/build
cmake ../.. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
sudo make install
```

**spdlog**:
```bash
git clone https://github.com/gabime/spdlog.git
cd spdlog
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
sudo make install
```

#### 验证安装

```bash
# 检查Protobuf
protoc --version

# 检查gRPC
pkg-config --modversion grpc++

# 检查spdlog（如果安装了）
pkg-config --modversion spdlog
```

#### 最小安装（仅核心功能）

如果只需要核心Actor功能，不需要远程通信：

```bash
# 只安装构建工具
sudo apt-get install -y build-essential cmake

# 构建时禁用远程功能
cmake .. \
  -DENABLE_PROTOBUF=OFF \
  -DENABLE_GRPC=OFF
```

## 快速构建

### 方法1: 使用构建脚本（推荐）

```bash
# 构建当前架构
./build.sh

# 构建指定架构
./build.sh --arch x86_64    # Intel/AMD 64位
./build.sh --arch arm64     # ARM 64位

# Debug模式
./build.sh --debug

# 启用Protobuf支持
./build.sh --protobuf

# 清理并重新构建
./build.sh --clean
```

### 方法2: 使用CMake直接构建

```bash
mkdir build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
```

## 跨架构编译

### 支持的架构

ProtoActor C++完全支持在ARM64和x86_64架构的Linux服务器上编译运行：
- ✅ **x86_64** (64位Intel/AMD)
- ✅ **ARM64** (AArch64, ARM服务器)

**注意**: 
- 本项目仅支持Linux平台，不支持Windows或macOS构建
- 仅支持64位架构，不支持32位系统

### 交叉编译

#### 在x86_64服务器上编译ARM64版本

```bash
sudo apt-get install gcc-aarch64-linux-gnu g++-aarch64-linux-gnu
cmake .. \
  -DCMAKE_SYSTEM_NAME=Linux \
  -DCMAKE_SYSTEM_PROCESSOR=aarch64 \
  -DCMAKE_C_COMPILER=aarch64-linux-gnu-gcc \
  -DCMAKE_CXX_COMPILER=aarch64-linux-gnu-g++
make -j$(nproc)
```

#### 在ARM64服务器上编译x86_64版本

```bash
sudo apt-get install gcc-x86-64-linux-gnu g++-x86-64-linux-gnu
cmake .. \
  -DCMAKE_SYSTEM_NAME=Linux \
  -DCMAKE_SYSTEM_PROCESSOR=x86_64 \
  -DCMAKE_C_COMPILER=x86-64-linux-gnu-gcc \
  -DCMAKE_CXX_COMPILER=x86-64-linux-gnu-g++
make -j$(nproc)
```

### 架构特定优化

- **x86_64**: 自动检测 SSE/AVX，Release 下 `-march=native`
- **ARM64**: ARM yield 指令，ARMv8-A 支持

## 构建选项

| 选项 | 说明 | 默认值 |
|------|------|--------|
| `CMAKE_BUILD_TYPE` | Release/Debug | Release |
| `BUILD_EXAMPLES` | 构建示例 | ON |
| `BUILD_TESTS` | 构建测试 | OFF |
| `ENABLE_PROTOBUF` | 启用Protobuf | OFF |
| `ENABLE_GRPC` | 启用gRPC | OFF |
| `BUILD_SHARED_LIBS` | 构建共享库 | OFF |

## 验证构建

```bash
# 检查二进制架构
file build-*/bin/hello_world

# 运行示例
./build-x86_64-Release/bin/hello_world
./build-x86_64-Release/bin/supervision_example
```

**运行单元测试**：详见 [TESTING.md](TESTING.md)。在项目根目录可执行：
```bash
./scripts/run_unit_tests.sh --configure   # 首次配置并运行全部单元测试
./scripts/run_unit_tests.sh              # 之后直接运行单元测试
./scripts/ci_tests.sh                    # CI 用：配置+构建+单元测试

## Conda 环境与运行时库冲突

在一些使用 Conda 的开发环境中，系统可能会优先加载 Conda 提供的 `libstdc++.so.6`，而该库版本与系统编译器预期的符号版本不匹配（例如缺少 `GLIBCXX_3.4.30` / `GLIBCXX_3.4.32`），导致运行测试或示例时出现类似错误：

```
/opt/conda/lib/libstdc++.so.6: version `GLIBCXX_3.4.32' not found
```

为避免此类运行时问题，可以选择下面任一方案：

- 临时在本地运行测试时使用系统库：

```bash
cd build
env LD_LIBRARY_PATH=/usr/lib/x86_64-linux-gnu:/lib/x86_64-linux-gnu ctest --output-on-failure
```

- 在构建脚本/CI 中清理 Conda 路径并禁止 CMake 将非系统路径写入 RUNPATH（推荐在 CI 中使用）。本仓库已在 `build.sh` 与 `scripts/ci_tests.sh` 中增加保护措施：

  - 清理 `LD_LIBRARY_PATH` 中的 `/opt/conda/lib` 条目并 `unset LD_RUN_PATH`。
  - 在 CMake 配置时传入：`-DCMAKE_SKIP_RPATH=ON -DCMAKE_BUILD_WITH_INSTALL_RPATH=OFF -DCMAKE_INSTALL_RPATH=""`，避免将非系统路径嵌入到二进制的 RUNPATH 中。

- 如果你不能修改环境变量，可以在构建产物上用 `patchelf` 修改或移除 RUNPATH（高级选项）：

```bash
sudo apt-get install -y patchelf
patchelf --remove-rpath build/tests/* build/bin/*
```

在 CI 中建议清理环境变量（即脚本中移除或过滤掉 `/opt/conda`），或在构建前设置 `LD_LIBRARY_PATH` 指向系统库以保证运行时使用与编译器一致的 `libstdc++`。
```

## 常见问题

### 构建相关问题

**Q: 编译时出现"找不到编译器"错误？**  
A: 确保已安装GCC或Clang：`sudo apt-get install build-essential`

**Q: 如何指定编译器？**  
A: `cmake .. -DCMAKE_C_COMPILER=gcc -DCMAKE_CXX_COMPILER=g++`

**Q: 如何启用更多优化？**  
A: `cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_FLAGS="-O3 -march=native"`

### 跨架构相关问题

**Q: 如何在ARM服务器上编译？**  
A: 直接在ARM64服务器上运行 `./build.sh --arch arm64`，或从x86_64交叉编译。

**Q: 支持哪些Linux发行版？**  
A: 支持所有主流Linux发行版（Ubuntu、Debian、CentOS、RHEL、Fedora等）。

### 依赖相关问题

**Q: CMake找不到库？**  
A: 设置 `PKG_CONFIG_PATH` 或 `CMAKE_PREFIX_PATH`。

## 生产环境部署

### 最小化依赖

```bash
cmake .. -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_EXE_LINKER_FLAGS="-static-libgcc -static-libstdc++"
```

### Docker部署

```bash
docker build --build-arg TARGET_ARCH=x86_64 -t protoactor-cpp:latest .
docker run protoactor-cpp:latest
```

### 性能调优

```bash
cmake .. -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_CXX_FLAGS="-O3 -march=native -mtune=native"
```
