# 第三方库使用指南

## 概述

为了加速开发和提高代码质量，ProtoActor C++项目使用以下主流开源库，而不是重新实现所有功能。

## 使用的开源库

### 1. gRPC C++ ⭐ 必需（远程通信）

**用途**: 远程Actor通信的底层网络协议

**为什么使用**:
- 高性能的RPC框架
- 支持双向流
- 跨语言兼容
- 生产环境验证

**集成状态**: 
- ✅ Protobuf定义已完成
- ⚠️ 待集成gRPC服务器和客户端

**安装**:
```bash
sudo apt-get install libgrpc++-dev protobuf-compiler-grpc
```

**使用示例**:
```cpp
// 在endpoint_writer.cpp中
#include <grpcpp/grpcpp.h>
#include "generated/remote.grpc.pb.h"

// 创建gRPC客户端
auto channel = grpc::CreateChannel(address, grpc::InsecureChannelCredentials());
auto stub = Remoting::NewStub(channel);
```

---

### 2. Protocol Buffers C++ ⭐ 必需（序列化）

**用途**: 消息序列化和反序列化

**为什么使用**:
- 高效的二进制序列化
- 跨语言支持
- 与gRPC完美集成
- Google维护，稳定可靠

**集成状态**:
- ✅ Protobuf定义文件已完成
- ⚠️ 待生成C++代码
- ⚠️ 待实现序列化器

**安装**:
```bash
sudo apt-get install libprotobuf-dev protobuf-compiler
```

**使用示例**:
```cpp
// 在proto_serializer.cpp中
#include "generated/actor.pb.h"
#include "generated/remote.pb.h"

// 序列化
RemoteMessage msg;
msg.SerializeToString(&output);

// 反序列化
RemoteMessage msg;
msg.ParseFromString(input);
```

---

### 3. spdlog ⭐ 推荐（日志）

**用途**: 高性能日志记录

**为什么使用**:
- 极高性能（异步日志）
- 线程安全
- 多种输出格式（控制台、文件、轮转等）
- 易于使用
- Header-only或静态库

**集成状态**:
- ✅ 日志接口已定义
- ⚠️ 待替换占位符实现

**安装**:
```bash
# 使用vcpkg
vcpkg install spdlog

# 或从源码
git clone https://github.com/gabime/spdlog.git
cd spdlog && mkdir build && cd build
cmake .. && make -j$(nproc) && sudo make install
```

**使用示例**:
```cpp
#include <spdlog/spdlog.h>

// 在代码中
spdlog::info("Actor spawned: {}", pid->Id);
spdlog::error("Failed to connect: {}", error);
```

**替换当前实现**:
- 当前使用占位符日志
- 可以逐步替换为spdlog
- 通过CMake选项控制

---

### 4. Google Test ⭐ 推荐（测试）

**用途**: 单元测试和集成测试框架

**为什么使用**:
- C++标准测试框架
- 丰富的断言
- 测试夹具支持
- Mock支持
- 广泛使用，文档完善

**集成状态**:
- ⚠️ 待创建测试框架
- ⚠️ 待编写测试代码

**安装**:
```bash
sudo apt-get install libgtest-dev libgmock-dev
# 或
vcpkg install gtest
```

**使用示例**:
```cpp
#include <gtest/gtest.h>

TEST(ActorTest, BasicSpawn) {
    auto system = protoactor::ActorSystem::New();
    auto props = protoactor::Props::FromProducer([]() {
        return std::make_shared<MyActor>();
    });
    auto pid = system->Root->Spawn(props);
    ASSERT_NE(pid, nullptr);
}
```

---

### 5. nlohmann/json ⭐ 可选（JSON序列化）

**用途**: JSON序列化支持（如果实现JSON序列化器）

**为什么使用**:
- Header-only，易于集成
- 现代C++ API
- 性能好
- 广泛使用

**集成状态**:
- ✅ JSON序列化器框架已创建
- ⚠️ 待完整实现

**安装**:
```bash
sudo apt-get install nlohmann-json3-dev
# 或
vcpkg install nlohmann-json
```

**使用示例**:
```cpp
#include <nlohmann/json.hpp>
using json = nlohmann::json;

// 序列化
json j = {{"name", "value"}};
std::string str = j.dump();

// 反序列化
json j = json::parse(str);
```

---

### 6. Boost.Asio ⚪ 可选（高级网络功能）

**用途**: 如果需要HTTP客户端或其他网络功能

**为什么使用**:
- 强大的异步网络库
- 跨平台
- 广泛使用

**何时使用**:
- 如果需要HTTP客户端（如Consul API调用）
- 如果需要更复杂的网络操作

**安装**:
```bash
sudo apt-get install libboost-all-dev
```

---

## 依赖管理方案

### 方案1: 系统包管理器（生产环境）

**优点**: 简单，系统级管理
**缺点**: 版本可能较旧

```bash
sudo apt-get install \
    libgrpc++-dev \
    libprotobuf-dev \
    protobuf-compiler \
    protobuf-compiler-grpc \
    libgtest-dev \
    nlohmann-json3-dev
```

### 方案2: vcpkg（推荐用于开发）

**优点**: 
- 版本控制好
- 跨平台
- 易于CI/CD集成

**使用**:
```bash
# 安装vcpkg
git clone https://github.com/Microsoft/vcpkg.git
cd vcpkg
./bootstrap-vcpkg.sh

# 安装依赖
./vcpkg install grpc protobuf spdlog gtest nlohmann-json

# 在CMake中使用
cmake .. -DCMAKE_TOOLCHAIN_FILE=/path/to/vcpkg/scripts/buildsystems/vcpkg.cmake
```

### 方案3: Conan

**优点**: 
- C++专用包管理器
- 依赖解析好

**使用**:
```bash
conan install . --build=missing
cmake .. -DCMAKE_TOOLCHAIN_FILE=conan_toolchain.cmake
```

### 方案4: Git Submodule（简单项目）

**优点**: 
- 完全控制版本
- 无需外部工具

**缺点**: 
- 需要手动管理
- 编译时间可能较长

---

## 当前实现策略

### 使用开源库 ✅
1. **gRPC** - 远程通信网络层
2. **Protobuf** - 消息序列化
3. **spdlog** - 日志（推荐）
4. **Google Test** - 测试框架（推荐）
5. **nlohmann/json** - JSON序列化（可选）

### 保持自实现 ✅
1. **Actor系统核心** - 核心逻辑，需要精确控制
2. **消息处理循环** - 核心逻辑
3. **监督策略** - 核心逻辑
4. **路由算法** - 相对简单，自实现更灵活
5. **事件流** - 核心逻辑

---

## 集成优先级

### 优先级1: gRPC + Protobuf（必需）
- 远程通信的核心
- 必须集成才能实现分布式功能

### 优先级2: spdlog（强烈推荐）
- 当前日志是占位符
- 替换简单，收益大
- 提高调试能力

### 优先级3: Google Test（推荐）
- 需要编写测试
- 标准框架，易于使用

### 优先级4: nlohmann/json（可选）
- 如果支持JSON序列化
- Header-only，易于集成

---

## 更新CMakeLists.txt

项目已更新CMakeLists.txt以支持：
- ✅ 可选依赖检测
- ✅ 条件编译
- ✅ 降级方案（如果依赖不存在）

**使用方式**:
```bash
# 启用所有推荐依赖
cmake .. \
  -DENABLE_PROTOBUF=ON \
  -DENABLE_GRPC=ON \
  -DENABLE_SPDLOG=ON \
  -DENABLE_GTEST=ON \
  -DENABLE_JSON=ON

# 最小配置（仅核心功能）
cmake .. \
  -DENABLE_PROTOBUF=OFF \
  -DENABLE_GRPC=OFF \
  -DENABLE_SPDLOG=OFF
```

---

## 总结

通过使用主流开源库：
- ✅ **加速开发** - 无需重新实现网络、序列化、日志等
- ✅ **提高质量** - 使用经过验证的库
- ✅ **降低维护成本** - 依赖社区维护
- ✅ **提高兼容性** - 使用标准库，易于集成

**建议**: 
1. 优先集成gRPC和Protobuf（必需）
2. 集成spdlog替换日志系统（推荐）
3. 使用Google Test编写测试（推荐）
4. 其他库按需集成
