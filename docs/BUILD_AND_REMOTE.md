# 构建指南与远程通信配置

本文档整合了构建配置和远程通信指南，帮助开发者快速构建项目并配置远程通信功能。

---

## 一、系统要求

- **操作系统**: Linux (Ubuntu, Debian, CentOS, RHEL, Fedora等)
- **架构**: x86_64 或 ARM64 (仅支持64位)
- **依赖库**: pthread (系统自带)

### 核心原则

ProtoActor C++采用**"使用成熟开源库，专注核心逻辑"**的策略：
- ✅ **使用开源库**: 网络、序列化、日志、测试等通用功能
- ✅ **自实现**: Actor系统核心逻辑，需要精确控制的部分

---

## 二、依赖管理

### 必需依赖（核心功能）

| 库 | 用途 | 状态 |
|---|---|---|
| **pthread** | 线程支持 | ✅ 系统自带 |
| **C++11标准库** | 基础功能 | ✅ 编译器自带 |

### 可选依赖（功能增强）

| 库 | 用途 | 推荐度 | 安装 |
|---|---|---|---|
| **gRPC** | 远程通信 | ⭐⭐⭐ 必需（远程功能） | `sudo apt-get install libgrpc++-dev protobuf-compiler-grpc` |
| **Protobuf** | 消息序列化 | ⭐⭐⭐ 必需（远程功能） | `sudo apt-get install libprotobuf-dev protobuf-compiler` |
| **spdlog** | 日志系统 | ⭐⭐ 强烈推荐 | 使用vcpkg或从源码编译 |
| **Google Test** | 测试框架 | ⭐⭐ 强烈推荐 | `sudo apt-get install libgtest-dev` |
| **nlohmann/json** | JSON支持 | ⭐ 可选 | `sudo apt-get install nlohmann-json3-dev` |

### 快速安装（Ubuntu/Debian）

```bash
# 必需依赖
sudo apt-get update
sudo apt-get install -y \
    build-essential \
    cmake \
    libprotobuf-dev \
    protobuf-compiler \
    libgrpc++-dev \
    protobuf-compiler-grpc \
    pkg-config

# 推荐依赖
sudo apt-get install -y \
    libgtest-dev \
    libgmock-dev \
    nlohmann-json3-dev
```

### 使用vcpkg安装（推荐）

```bash
# 1. 安装vcpkg
cd ~
git clone https://github.com/Microsoft/vcpkg.git
cd vcpkg
./bootstrap-vcpkg.sh

# 2. 安装依赖
./vcpkg install grpc protobuf spdlog gtest nlohmann-json

# 3. 配置CMake
cmake .. \
  -DCMAKE_TOOLCHAIN_FILE=~/vcpkg/scripts/buildsystems/vcpkg.cmake \
  -DENABLE_PROTOBUF=ON \
  -DENABLE_GRPC=ON \
  -DENABLE_SPDLOG=ON
```

### 最小安装（仅核心功能）

```bash
# 只安装构建工具
sudo apt-get install -y build-essential cmake

# 构建时禁用远程功能
cmake .. -DENABLE_PROTOBUF=OFF -DENABLE_GRPC=OFF
```

---

## 三、快速构建

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
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
```

---

## 四、构建选项

| 选项 | 说明 | 默认值 |
|------|------|--------|
| `CMAKE_BUILD_TYPE` | Release/Debug | Release |
| `BUILD_EXAMPLES` | 构建示例 | ON |
| `BUILD_TESTS` | 构建测试 | OFF |
| `ENABLE_PROTOBUF` | 启用Protobuf | OFF |
| `ENABLE_GRPC` | 启用gRPC | OFF |
| `BUILD_SHARED_LIBS` | 构建共享库 | OFF |

---

## 五、跨架构编译

### 支持的架构

- ✅ **x86_64** (64位Intel/AMD)
- ✅ **ARM64** (AArch64, ARM服务器)

**注意**: 本项目仅支持Linux平台，不支持Windows或macOS构建

### 交叉编译示例

```bash
# 在x86_64上编译ARM64
sudo apt-get install gcc-aarch64-linux-gnu g++-aarch64-linux-gnu
cmake .. \
  -DCMAKE_SYSTEM_NAME=Linux \
  -DCMAKE_SYSTEM_PROCESSOR=aarch64 \
  -DCMAKE_C_COMPILER=aarch64-linux-gnu-gcc \
  -DCMAKE_CXX_COMPILER=aarch64-linux-gnu-g++
make -j$(nproc)
```

### CI交叉构建

```bash
CROSS_ARM64=1 ./tests/scripts/ci_tests.sh build/ci_arm64
```

---

## 六、验证构建

```bash
# 检查二进制架构
file build-*/bin/hello_world

# 运行示例
./build-x86_64-Release/bin/hello_world

# 运行单元测试
./tests/scripts/run_unit_tests.sh --configure
```

---

## 七、远程通信配置

### 概述

ProtoActor 的远程通信基于 gRPC 和 Protocol Buffers，提供：
- **透明远程调用** - 本地和远程消息传递使用相同的 API
- **高性能** - 基于流式 gRPC，支持批量消息
- **跨语言互操作** - 与 ProtoActor-Go、ProtoActor-C# 完全兼容

### 架构

```
┌─────────────────────┐                    ┌─────────────────────┐
│      Node 1         │                    │      Node 2         │
│  ┌───────────────┐  │     gRPC/HTTP2     │  ┌───────────────┐  │
│  │  ActorSystem  │  │◄──────────────────►│  │  ActorSystem  │  │
│  │    Remote     │  │                    │  │    Remote     │  │
│  └───────────────┘  │                    │  └───────────────┘  │
│         │           │                    │         │           │
│    ┌────▼────┐      │                    │    ┌────▼────┐      │
│    │ Actor A │      │                    │    │ Actor B │      │
│    └─────────┘      │                    │    └─────────┘      │
└─────────────────────┘                    └─────────────────────┘
```

### 服务器端示例

```cpp
#include "protoactor/actor.h"
#include "protoactor/remote/remote.h"

class MyActor : public protoactor::Actor {
public:
    void Receive(std::shared_ptr<protoactor::Context> context) override {
        // Handle messages...
    }
};

int main() {
    auto system = protoactor::ActorSystem::New();

    // 启动远程服务
    auto remote = protoactor::remote::Remote::Start(system, "localhost", 8090);

    // 注册 Actor 类型
    auto props = protoactor::Props::FromProducer([]() {
        return std::make_shared<MyActor>();
    });
    remote->Register("myactor", props);

    std::cin.get();  // 保持运行

    remote->Shutdown(true);
    system->Shutdown();
    return 0;
}
```

### 客户端示例

```cpp
#include "protoactor/actor.h"
#include "protoactor/remote/remote.h"

int main() {
    auto system = protoactor::ActorSystem::New();
    auto root = system->GetRoot();

    auto remote = protoactor::remote::Remote::Start(system, "localhost", 8091);

    // 在远程节点创建 Actor
    auto [pid, err] = remote->SpawnNamed(
        "localhost:8090",    // 服务器地址
        "myactor",           // 注册的类型名
        "instance-1",        // 实例名
        std::chrono::seconds(5)
    );

    if (!err) {
        root->Send(pid, std::make_shared<Hello>("World"));
    }

    remote->Shutdown(true);
    system->Shutdown();
    return 0;
}
```

### Remote 配置选项

| 选项 | 类型 | 默认值 | 说明 |
|------|------|--------|------|
| `host` | string | "localhost" | 监听地址 |
| `port` | int | 0 | 监听端口（0=自动分配） |
| `advertised_host` | string | "" | 对外广播地址 |
| `endpoint_writer_batch_size` | int | 1000 | 批量发送大小 |
| `endpoint_writer_queue_size` | int | 100000 | 发送队列大小 |
| `max_retry_count` | int | 5 | 最大重试次数 |

### 配置示例

```cpp
using namespace protoactor::remote;

// 使用配置函数
auto remote = Remote::Start(system, "localhost", 8090, {
    [](std::shared_ptr<Config> c) {
        c->endpoint_writer_batch_size = 2000;
        c->max_retry_count = 10;
    }
});

// 手动配置
auto config = std::make_shared<Config>();
config->host = "0.0.0.0";
config->port = 8090;
config->advertised_host = "my-server.example.com";
```

---

## 八、消息序列化

### Protobuf 消息（推荐）

```protobuf
syntax = "proto3";
package messages;

import "actor.proto";

message Hello {
    actor.PID sender = 1;
    string message = 2;
}
```

### JSON 序列化

```cpp
#include "protoactor/remote/json_serializer.h"

auto serializer = remote->GetSerializer();
serializer->RegisterSerializer(std::make_shared<JsonSerializer>());
```

---

## 九、高级功能

### Endpoint 管理

```cpp
auto endpoint_manager = remote->GetEndpointManager();
auto endpoints = endpoint_manager->GetEndpoints();
```

### 黑名单

```cpp
remote->GetBlocklist()->Block("malicious-host:8090");

if (remote->GetBlocklist()->IsBlocked("some-host:8090")) {
    // ...
}
```

### 优雅关闭

```cpp
remote->Shutdown(true);   // 优雅关闭
remote->Shutdown(false);  // 立即关闭
```

---

## 十、性能优化

### 远程通信优化

- **批量发送**: 增加批量大小提高吞吐量
  ```cpp
  config->endpoint_writer_batch_size = 5000;
  ```
- **消息大小**: 保持消息小于 4MB（gRPC 默认限制）
- **连接复用**: ProtoActor 自动复用连接

### 编译优化

```bash
cmake .. -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_CXX_FLAGS="-O3 -march=native -mtune=native"
```

---

## 十一、故障排查

### 常见问题

**Q: 连接超时**
- 检查防火墙设置
- 确认端口正确
- 验证 `advertised_host` 配置

**Q: 序列化失败**
- 确保消息类型已注册
- 检查 Protobuf 定义是否同步

**Q: Conda环境冲突**
```bash
env LD_LIBRARY_PATH=/usr/lib/x86_64-linux-gnu:/lib/x86_64-linux-gnu ctest --output-on-failure
```

---

## 十二、生产环境部署

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

---

## 十三、参考资源

- [ProtoActor-Go Remote](https://github.com/asynkron/protoactor-go#networking--remoting)
- [gRPC Documentation](https://grpc.io/docs/)
- [Protocol Buffers](https://protobuf.dev/)
- [API参考文档](API_REFERENCE.md)
- [对比与迁移指南](COMPARISON_AND_MIGRATION.md)
- [测试与性能指南](../tests/TESTING.md)
