# 依赖安装指南

## 快速安装（Ubuntu/Debian）

### 必需依赖

```bash
# 更新包列表
sudo apt-get update

# 安装必需依赖
sudo apt-get install -y \
    build-essential \
    cmake \
    libprotobuf-dev \
    protobuf-compiler \
    libgrpc++-dev \
    protobuf-compiler-grpc \
    pkg-config
```

### 推荐依赖

```bash
# 安装推荐依赖
sudo apt-get install -y \
    libgtest-dev \
    libgmock-dev \
    nlohmann-json3-dev

# spdlog需要从源码编译或使用vcpkg
```

## 使用vcpkg安装（推荐）

### 1. 安装vcpkg

```bash
cd ~
git clone https://github.com/Microsoft/vcpkg.git
cd vcpkg
./bootstrap-vcpkg.sh
```

### 2. 安装依赖

```bash
./vcpkg install \
    grpc \
    protobuf \
    spdlog \
    gtest \
    nlohmann-json
```

### 3. 配置CMake使用vcpkg

```bash
cmake .. \
  -DCMAKE_TOOLCHAIN_FILE=~/vcpkg/scripts/buildsystems/vcpkg.cmake \
  -DENABLE_PROTOBUF=ON \
  -DENABLE_GRPC=ON \
  -DENABLE_SPDLOG=ON \
  -DENABLE_GTEST=ON \
  -DENABLE_JSON=ON
```

## 使用Conan安装

### 1. 安装Conan

```bash
pip install conan
```

### 2. 安装依赖

```bash
conan install . --build=missing
```

### 3. 配置CMake

```bash
cmake .. -DCMAKE_TOOLCHAIN_FILE=conan_toolchain.cmake
```

## 从源码编译（高级用户）

### Protobuf

```bash
git clone https://github.com/protocolbuffers/protobuf.git
cd protobuf
git submodule update --init --recursive
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
sudo make install
```

### gRPC

```bash
git clone --recurse-submodules https://github.com/grpc/grpc.git
cd grpc
mkdir -p cmake/build
cd cmake/build
cmake ../.. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
sudo make install
```

### spdlog

```bash
git clone https://github.com/gabime/spdlog.git
cd spdlog
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
sudo make install
```

## 验证安装

```bash
# 检查Protobuf
protoc --version

# 检查gRPC
pkg-config --modversion grpc++

# 检查spdlog（如果安装了）
pkg-config --modversion spdlog
```

## 最小安装（仅核心功能）

如果只需要核心Actor功能，不需要远程通信：

```bash
# 只安装构建工具
sudo apt-get install -y build-essential cmake

# 构建时禁用远程功能
cmake .. \
  -DENABLE_PROTOBUF=OFF \
  -DENABLE_GRPC=OFF
```

## 完整安装（所有功能）

```bash
# 使用vcpkg（推荐）
./vcpkg install grpc protobuf spdlog gtest nlohmann-json

# 或使用系统包管理器
sudo apt-get install -y \
    libgrpc++-dev \
    libprotobuf-dev \
    protobuf-compiler \
    protobuf-compiler-grpc \
    libgtest-dev \
    nlohmann-json3-dev

# spdlog需要单独安装
```

## 故障排查

### 问题: 找不到gRPC

```bash
# 检查是否安装
pkg-config --modversion grpc++

# 如果未安装，使用vcpkg或从源码编译
```

### 问题: Protobuf版本不匹配

```bash
# 确保protobuf-compiler和libprotobuf-dev版本一致
apt-cache policy libprotobuf-dev protobuf-compiler
```

### 问题: CMake找不到库

```bash
# 设置PKG_CONFIG_PATH
export PKG_CONFIG_PATH=/usr/local/lib/pkgconfig:$PKG_CONFIG_PATH

# 或使用CMAKE_PREFIX_PATH
cmake .. -DCMAKE_PREFIX_PATH=/usr/local
```
