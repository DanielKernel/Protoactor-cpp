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

#### 在ARM64服务器上编译x86_64版本

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

### 架构特定优化

#### x86_64优化
- 自动检测并启用SSE/AVX指令
- 使用x86特定的pause指令优化自旋循环
- Release模式下使用`-march=native`优化

#### ARM64优化
- 使用ARM特定的yield指令
- 优化的内存对齐
- 支持ARMv8-A指令集

## gRPC集成

### 概述

gRPC集成用于实现完整的远程通信功能。

### 当前状态

✅ **已完成**:
- Protobuf定义文件 (`proto/remote.proto`)
- 基础接口定义（Serializer, EndpointManager, EndpointWriter, EndpointReader）
- RemoteProcess实现
- SerializerRegistry实现

⚠️ **待完成**:
- gRPC服务器集成
- gRPC客户端集成
- Protobuf代码生成
- EndpointManager完整实现
- EndpointWriter完整实现
- EndpointReader完整实现
- Protobuf序列化器实现

### 实现步骤

#### 1. 生成Protobuf代码

创建 `scripts/generate_proto.sh`:

```bash
#!/bin/bash
set -e

PROTO_DIR="proto"
OUT_DIR="generated"

mkdir -p ${OUT_DIR}

# Generate C++ code from protobuf
protoc --cpp_out=${OUT_DIR} \
       --grpc_out=${OUT_DIR} \
       --plugin=protoc-gen-grpc=`which grpc_cpp_plugin` \
       -I${PROTO_DIR} \
       ${PROTO_DIR}/actor.proto \
       ${PROTO_DIR}/remote.proto

echo "Protobuf code generated in ${OUT_DIR}/"
```

运行脚本生成代码:
```bash
chmod +x scripts/generate_proto.sh
./scripts/generate_proto.sh
```

#### 2. 更新CMakeLists.txt

添加Protobuf和gRPC支持:

```cmake
# Find Protobuf
find_package(Protobuf REQUIRED)
find_package(gRPC REQUIRED)

# Generate protobuf files
set(PROTO_FILES
    proto/actor.proto
    proto/remote.proto
)

protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS ${PROTO_FILES})

# Add generated files to library
target_sources(protoactor-cpp PRIVATE ${PROTO_SRCS})
target_include_directories(protoactor-cpp PUBLIC ${CMAKE_CURRENT_BINARY_DIR})

# Link libraries
target_link_libraries(protoactor-cpp
    ${Protobuf_LIBRARIES}
    gRPC::grpc++
    gRPC::grpc++_reflection
)
```

#### 3. 实现Protobuf序列化器

创建 `src/remote/proto_serializer.cpp`:

```cpp
#include "protoactor/remote/serializer.h"
#include "generated/actor.pb.h"
#include "generated/remote.pb.h"
#include <google/protobuf/any.pb.h>
#include <google/protobuf/message.h>

class ProtoSerializer : public protoactor::remote::Serializer {
public:
    std::vector<uint8_t> Serialize(const std::any& message) override {
        // 将message转换为protobuf Message并序列化
        // 需要类型注册机制
    }
    
    std::any Deserialize(const std::string& type_name, const std::vector<uint8_t>& bytes) override {
        // 根据type_name反序列化
    }
    
    std::string GetTypeName(const std::any& message) override {
        // 获取消息类型名
    }
    
    int32_t GetSerializerID() const override { return 0; }
};
```

#### 4. 实现gRPC服务器

在 `src/remote/remote.cpp` 中添加:

```cpp
#include <grpcpp/grpcpp.h>
#include "generated/remote.grpc.pb.h"

class RemotingServiceImpl : public protoactor::remote::Remoting::Service {
    grpc::Status Receive(
        grpc::ServerContext* context,
        grpc::ServerReaderWriter<RemoteMessage, RemoteMessage>* stream) override {
        
        // 使用EndpointReader处理流
        auto reader = std::make_shared<EndpointReader>(remote_);
        return reader->HandleStream(stream);
    }
    
    // 实现其他RPC方法...
};

void Remote::Start() {
    // 创建gRPC服务器
    grpc::ServerBuilder builder;
    builder.AddListeningPort(config_->Address(), grpc::InsecureServerCredentials());
    
    auto service = std::make_shared<RemotingServiceImpl>(this);
    builder.RegisterService(service.get());
    
    server_ = builder.BuildAndStart();
    
    // 启动EndpointManager
    endpoint_manager_->Start();
}
```

#### 5. 实现gRPC客户端

在 `src/remote/endpoint_writer.cpp` 中:

```cpp
#include <grpcpp/grpcpp.h>
#include "generated/remote.grpc.pb.h"

bool EndpointWriter::InitializeInternal() {
    // 创建gRPC客户端连接
    auto channel = grpc::CreateChannel(address_, grpc::InsecureChannelCredentials());
    auto stub = Remoting::NewStub(channel);
    
    // 创建双向流
    grpc::ClientContext context;
    stream_ = stub->Receive(&context);
    
    // 发送连接请求
    RemoteMessage connect_msg;
    // ... 设置连接请求
    stream_->Write(connect_msg);
    
    // 接收连接响应
    RemoteMessage response;
    stream_->Read(&response);
    
    // 启动接收线程
    receive_thread_ = std::thread([this]() {
        RemoteMessage msg;
        while (stream_->Read(&msg)) {
            // 处理接收到的消息
        }
    });
    
    return true;
}
```

### 注意事项

1. **线程安全**: gRPC操作是线程安全的，但需要确保消息处理的线程安全
2. **错误处理**: 需要处理连接失败、超时等错误情况
3. **资源管理**: 确保正确关闭gRPC连接和流
4. **性能优化**: 使用消息批处理提高性能

### 参考资源

- [gRPC C++文档](https://grpc.io/docs/languages/cpp/)
- [Protobuf C++文档](https://developers.google.com/protocol-buffers/docs/cpptutorial)

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

**Q: 性能在不同架构上有差异吗？**  
A: 核心逻辑相同，性能差异主要来自CPU本身的性能差异。

**Q: 支持哪些Linux发行版？**  
A: 支持所有主流Linux发行版（Ubuntu、Debian、CentOS、RHEL、Fedora等）。

### 依赖相关问题

**Q: 找不到gRPC？**  
A: 
```bash
# 检查是否安装
pkg-config --modversion grpc++

# 如果未安装，使用vcpkg或从源码编译
```

**Q: Protobuf版本不匹配？**  
A: 
```bash
# 确保protobuf-compiler和libprotobuf-dev版本一致
apt-cache policy libprotobuf-dev protobuf-compiler
```

**Q: CMake找不到库？**  
A: 
```bash
# 设置PKG_CONFIG_PATH
export PKG_CONFIG_PATH=/usr/local/lib/pkgconfig:$PKG_CONFIG_PATH

# 或使用CMAKE_PREFIX_PATH
cmake .. -DCMAKE_PREFIX_PATH=/usr/local
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

### 性能调优

```bash
# 启用所有优化
cmake .. -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_CXX_FLAGS="-O3 -march=native -mtune=native"

# 启用链接时优化
cmake .. -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_CXX_FLAGS="-flto" \
  -DCMAKE_EXE_LINKER_FLAGS="-flto"
```
