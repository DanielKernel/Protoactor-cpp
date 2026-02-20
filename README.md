# ProtoActor C++

这是 ProtoActor-Go 的 C++17 实现，**本地核心 Actor 能力**与 Go 版对齐；**远程通信、集群与虚拟 Actor 当前暂不支持**。

## 功能完整性

基于与 [ProtoActor-Go](https://github.com/asynkron/protoactor-go) 的对比及代码实现，当前支持情况如下：

| 核心功能 | 状态 | 远程/集群 | 状态 |
|---------|------|----------|------|
| Actor/PID/Context/Props | ✅ 支持 | gRPC 远程通信 | ⛔ 暂不支持 |
| 消息传递 (Send/Request/Future) | ✅ 支持 | Protobuf/JSON 序列化 | ⛔ 暂不支持 |
| 生命周期管理 | ✅ 支持 | 集群成员管理 | ⛔ 暂不支持 |
| 行为管理 (Become/Unbecome) | ✅ 支持 | Gossip 协议 | ⛔ 暂不支持 |
| 监督策略 (OneForOne/AllForOne) | ✅ 支持 | Pub/Sub（本地） | ✅ 支持 |
| 路由系统 (广播/轮询/随机/哈希) | ✅ 支持 | **虚拟 Actor (Grains)** | ⛔ 暂不支持 |
| 持久化 (事件溯源/快照) | ✅ 支持 | 身份查找/PID 缓存 | ⛔ 暂不支持 |

**详细对比与差异分析**：[docs/COMPARISON_AND_MIGRATION.md](docs/COMPARISON_AND_MIGRATION.md)、[docs/GAP_ANALYSIS_VS_PROTOACTOR_GO.md](docs/GAP_ANALYSIS_VS_PROTOACTOR_GO.md)

## 项目结构

```
protoactor-cpp/
├── include/external/     # 头文件
│   ├── actor.h             # Actor接口
│   ├── pid.h               # Process Identifier
│   ├── context.h           # Actor上下文
│   ├── props.h             # Actor属性配置
│   ├── actor_system.h      # Actor系统
│   └── ...
├── src/                    # 实现文件
│   ├── actor/              # 核心actor模块
│   ├── remote/             # 远程通信模块
│   ├── cluster/            # 集群模块
│   ├── router/             # 路由模块
│   └── ...
├── proto/                  # Protobuf定义
├── examples/               # 示例代码
└── CMakeLists.txt         # CMake构建文件
```

## 快速开始

### 构建要求

- **Linux操作系统** (Ubuntu, Debian, CentOS, RHEL等)
- C++17 编译器（GCC 7+, Clang 5+）
- CMake 3.10+
- pthread库（系统自带）

### 快速构建

```bash
# 使用构建脚本（推荐）
./build.sh

# 指定架构
./build.sh --arch x86_64    # x86_64架构
./build.sh --arch arm64     # ARM64架构

# Debug模式
./build.sh --debug

# 启用Protobuf支持
./build.sh --protobuf
```

### 手动构建

```bash
mkdir build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . -j$(nproc)
```

## 跨架构支持

项目完全支持在ARM64和x86_64架构的Linux服务器上编译运行：
- ✅ **x86_64** (64位Intel/AMD)
- ✅ **ARM64** (AArch64, ARM服务器)

**注意**: 仅支持64位架构，不支持32位系统。

## 依赖项

### 必需依赖
- pthread（系统自带）
- C++17 标准库（编译器自带）

### 可选依赖（功能增强）
- **gRPC + Protobuf**: 预留远程通信构建选项（当前远程功能暂不支持）
- **spdlog**: 日志系统（推荐）
- **rapidjson**: JSON 支持（可选）

## 核心功能

### Actor模型
- Actor接口和实现
- PID (Process Identifier)
- Context (上下文)
- Props (属性配置)

### 消息传递
- 异步消息发送
- Request-Response模式
- Future异步结果
- 消息信封和头部

### 监督策略
- OneForOne策略
- AllForOne策略
- Restarting策略
- 自定义Decider

### 远程通信（⛔ 当前暂不支持）
- gRPC 与跨节点消息传递、远程 Actor 创建等接口存在但未实现端到端，详见 [功能差异分析](docs/GAP_ANALYSIS_VS_PROTOACTOR_GO.md)。

### 集群支持（⛔ 当前暂不支持）
- 成员管理、Gossip 协议、虚拟 Actor (Grains)、身份查找等接口存在但跨节点能力暂未打通。

### 路由
- 广播路由
- 轮询路由
- 随机路由
- 一致性哈希路由

### 持久化
- 事件溯源
- 快照
- 内存提供者
- Couchbase提供者

## 示例

### Hello World

```cpp
#include "protoactor/actor.h"
#include "protoactor/context.h"
#include "protoactor/actor_system.h"

class HelloActor : public protoactor::Actor {
public:
    void Receive(std::shared_ptr<protoactor::Context> context) override {
        // 处理消息
    }
};

int main() {
    auto system = protoactor::ActorSystem::New();
    auto root = system->GetRoot();

    auto props = protoactor::Props::FromProducer([]() {
        return std::make_shared<HelloActor>();
    });

    auto pid = root->Spawn(props);
    // 发送消息...

    return 0;
}
```

更多示例请查看 [examples/](examples/) 目录：

| 示例 | 说明 |
|------|------|
| [hello_world.cpp](examples/hello_world.cpp) | 最小示例 |
| [supervision_example.cpp](examples/supervision_example.cpp) | 监督策略 |
| [behavior_example.cpp](examples/behavior_example.cpp) | 行为管理 |
| [router_example.cpp](examples/router_example.cpp) | 路由策略 |
| [middleware_example.cpp](examples/middleware_example.cpp) | 中间件 |
| [persistence_example.cpp](examples/persistence_example.cpp) | 持久化 |
| [remote_example.cpp](examples/remote_example.cpp) | 远程通信 |
| [cluster_example.cpp](examples/cluster_example.cpp) | 集群支持 |
| [pubsub_example.cpp](examples/pubsub_example.cpp) | 发布订阅 |

## 当前限制与路线图

- **平台**：仅支持 Linux，64 位架构（x86_64 / ARM64），详见 [架构文档](docs/ARCHITECTURE.md)。
- **⛔ 远程通信**：当前**暂不支持**。gRPC/Protobuf/序列化/远程 Spawn 等为骨架或占位，无法用于跨节点通信。
- **⛔ 集群**：当前**暂不支持**。MemberList/Gossip/PubSub 等有本地实现，但跨节点 Gossip、GetCluster、BlockList 联动等未打通。
- **⛔ 虚拟 Actor (Grains)**：当前**暂不支持**。身份查找与 PID 缓存逻辑存在，但依赖未实现的 `SpawnNamed`，无法使用。
- 更多缺口说明见 [功能差异分析](docs/GAP_ANALYSIS_VS_PROTOACTOR_GO.md)。

## 文档

| 文档 | 说明 |
|------|------|
| [构建指南](docs/BUILD_AND_REMOTE.md) | 完整的构建、依赖安装、跨架构编译和 gRPC 集成说明 |
| [测试指导](../tests/TESTING.md) | 单元测试、功能测试、覆盖率统计 |
| [性能测试](docs/PERF_TESTS.md) | 性能基准测试说明 |
| [性能对比](docs/BENCHMARK.md) | 与 ProtoActor-Go 及其他框架的性能对比 |
| [功能对比](docs/COMPARISON_AND_MIGRATION.md) | 与 ProtoActor-Go 的详细功能对比分析 |
| [远程通信指南](docs/REMOTE_GUIDE.md) | 远程通信配置、消息序列化、最佳实践 |
| [迁移指南](docs/MIGRATION_GUIDE.md) | 从 ProtoActor-Go 迁移到 C++ 版本 |
| [API参考](docs/API_REFERENCE.md) | 完整的 C++ API 文档 |

## 许可证

与原项目保持一致。
