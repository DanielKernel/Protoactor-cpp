# ProtoActor C++

这是ProtoActor Go版本的C++11实现，确保功能100%继承。

## 功能完整性

基于与 [ProtoActor-Go](https://github.com/asynkron/protoactor-go) 的详细对比分析，本实现功能完成度达到 **95%+**：

| 核心功能 | 状态 | 远程/集群 | 状态 |
|---------|------|----------|------|
| Actor/PID/Context/Props | 100% | gRPC远程通信 | 100% |
| 消息传递 (Send/Request/Future) | 100% | Protobuf/JSON序列化 | 100% |
| 生命周期管理 | 100% | 集群成员管理 | 100% |
| 行为管理 (Become/Unbecome) | 100% | Gossip协议 | 100% |
| 监督策略 (OneForOne/AllForOne) | 100% | Pub/Sub | 100% |
| 路由系统 (广播/轮询/随机/哈希) | 100% | 虚拟Actor (Grains) | 90% |
| 持久化 (事件溯源/快照) | 100% | 身份查找/PID缓存 | 100% |

**详细对比请参阅 [docs/COMPARISON.md](docs/COMPARISON.md)**

## 项目结构

```
protoactor-cpp/
├── include/protoactor/     # 头文件
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
- C++11或更高版本的编译器（GCC 4.8+, Clang 3.3+）
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
- C++11标准库（编译器自带）

### 可选依赖（功能增强）
- **gRPC + Protobuf**: 远程通信（必需，如需远程功能）
- **spdlog**: 日志系统（推荐）
- **Google Test**: 测试框架（推荐）
- **nlohmann/json**: JSON支持（可选）

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

### 远程通信
- gRPC支持
- 跨节点消息传递
- 远程Actor创建

### 集群支持
- 成员管理
- Gossip协议
- 虚拟Actor (Grains)
- 身份查找

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

## 文档

| 文档 | 说明 |
|------|------|
| [构建指南](docs/BUILD_GUIDE.md) | 完整的构建、依赖安装、跨架构编译和 gRPC 集成说明 |
| [测试指导](docs/TESTING.md) | 单元测试、功能测试、覆盖率统计 |
| [性能测试](docs/PERF_TESTS.md) | 性能基准测试说明 |
| [功能对比](docs/COMPARISON.md) | 与 ProtoActor-Go 的详细功能对比分析 |
| [远程通信指南](docs/REMOTE_GUIDE.md) | 远程通信配置、消息序列化、最佳实践 |
| [迁移指南](docs/MIGRATION_GUIDE.md) | 从 ProtoActor-Go 迁移到 C++ 版本 |

## 许可证

与原项目保持一致。
