# 远程通信指南

本指南详细介绍 ProtoActor C++ 的远程通信功能，包括配置、使用方法和最佳实践。

## 概述

ProtoActor 的远程通信基于 gRPC 和 Protocol Buffers，提供：

- **透明远程调用** - 本地和远程消息传递使用相同的 API
- **高性能** - 基于流式 gRPC，支持批量消息
- **跨语言互操作** - 与 ProtoActor-Go、ProtoActor-C# 完全兼容

## 架构

```
┌─────────────────────┐                    ┌─────────────────────┐
│      Node 1         │                    │      Node 2         │
│                     │                    │                     │
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

## 快速开始

### 1. 构建要求

远程功能需要启用 gRPC 和 Protobuf：

```bash
cmake .. -DENABLE_GRPC=ON -DENABLE_PROTOBUF=ON
make -j$(nproc)
```

### 2. 服务器端

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

    // 注册 Actor 类型，允许远程节点创建
    auto props = protoactor::Props::FromProducer([]() {
        return std::make_shared<MyActor>();
    });
    remote->Register("myactor", props);

    // 保持运行...
    std::cin.get();

    remote->Shutdown(true);
    system->Shutdown();
    return 0;
}
```

### 3. 客户端

```cpp
#include "protoactor/actor.h"
#include "protoactor/remote/remote.h"

int main() {
    auto system = protoactor::ActorSystem::New();
    auto root = system->GetRoot();

    // 启动远程（客户端也需要，用于接收响应）
    auto remote = protoactor::remote::Remote::Start(system, "localhost", 8091);

    // 在远程节点创建 Actor
    auto [pid, err] = remote->SpawnNamed(
        "localhost:8090",    // 服务器地址
        "myactor",           // 注册的类型名
        "instance-1",        // 实例名
        std::chrono::seconds(5)  // 超时
    );

    if (!err) {
        // 像本地 Actor 一样发送消息
        root->Send(pid, std::make_shared<Hello>("World"));
    }

    remote->Shutdown(true);
    system->Shutdown();
    return 0;
}
```

## 配置选项

### Remote Config

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

// 方式1：使用配置函数
auto remote = Remote::Start(system, "localhost", 8090, {
    [](std::shared_ptr<Config> c) {
        c->endpoint_writer_batch_size = 2000;
        c->max_retry_count = 10;
    }
});

// 方式2：手动配置
auto config = std::make_shared<Config>();
config->host = "0.0.0.0";
config->port = 8090;
config->advertised_host = "my-server.example.com";
```

## 消息序列化

### Protobuf 消息（推荐）

定义 `.proto` 文件：

```protobuf
syntax = "proto3";
package messages;

import "actor.proto";  // 包含 PID 类型

message Hello {
    actor.PID sender = 1;
    string message = 2;
}

message Response {
    string value = 1;
}
```

### JSON 序列化

```cpp
#include "protoactor/remote/json_serializer.h"

// 注册 JSON 序列化器
auto serializer = remote->GetSerializer();
serializer->RegisterSerializer(std::make_shared<JsonSerializer>());
```

## 高级功能

### 1. Endpoint 管理

```cpp
// 获取 Endpoint 管理器
auto endpoint_manager = remote->GetEndpointManager();

// 获取已连接的端点
auto endpoints = endpoint_manager->GetEndpoints();
```

### 2. 黑名单

```cpp
// 添加到黑名单
remote->GetBlockList()->Block("malicious-host:8090");

// 检查是否在黑名单
if (remote->GetBlockList()->IsBlocked("some-host:8090")) {
    // ...
}
```

### 3. 优雅关闭

```cpp
// 优雅关闭（等待进行中的请求完成）
remote->Shutdown(true);

// 立即关闭
remote->Shutdown(false);
```

## 与 Go 版本对比

| 功能 | Go | C++ |
|------|-----|-----|
| 启动远程 | `remote.Start("localhost:8090")` | `Remote::Start(system, "localhost", 8090)` |
| 注册类型 | `remote.Register("kind", props)` | `remote->Register("kind", props)` |
| 远程创建 | `remote.SpawnNamed(...)` | `remote->SpawnNamed(...)` |
| 发送消息 | 相同的 `context.Send()` | 相同的 `context->Send()` |

## 性能优化

### 1. 批量发送

```cpp
// 增加批量大小提高吞吐量
config->endpoint_writer_batch_size = 5000;
```

### 2. 连接复用

ProtoActor 自动复用连接，无需手动管理。

### 3. 消息大小

- 保持消息小于 4MB（gRPC 默认限制）
- 大数据使用流式传输或分块

## 故障排查

### 常见问题

1. **连接超时**
   - 检查防火墙设置
   - 确认端口正确
   - 验证 `advertised_host` 配置

2. **序列化失败**
   - 确保消息类型已注册
   - 检查 Protobuf 定义是否同步

3. **消息丢失**
   - 检查接收方 Actor 是否正常运行
   - 验证消息类型匹配

### 调试日志

```cpp
// 启用 spdlog 日志（如果可用）
#ifdef ENABLE_SPDLOG
spdlog::set_level(spdlog::level::debug);
#endif
```

## 完整示例

参考 `examples/remote_example.cpp` 获取完整代码。

运行步骤：

```bash
# 终端 1 - 启动服务器
./bin/remote_example server localhost 8090

# 终端 2 - 启动客户端
./bin/remote_example client localhost:8090
```

## 参考

- [ProtoActor-Go Remote](https://github.com/asynkron/protoactor-go#networking--remoting)
- [gRPC Documentation](https://grpc.io/docs/)
- [Protocol Buffers](https://protobuf.dev/)
