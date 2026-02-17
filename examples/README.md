# 使用样例 (examples)

本目录为**使用样例**，用于演示如何在应用中使用 ProtoActor C++，不作为测试用例。

## 现有示例

| 样例 | 说明 | 对应Go示例 | 依赖 |
|------|------|-----------|------|
| [hello_world.cpp](hello_world.cpp) | 最小示例：创建 ActorSystem、Spawn 一个 Actor、发送一条消息 | Hello World | 无 |
| [supervision_example.cpp](supervision_example.cpp) | 监督与错误处理：OneForOne策略 | Supervision | 无 |
| [behavior_example.cpp](behavior_example.cpp) | 行为管理：Become/Unbecome、行为栈、生命周期事件 | Behavior/Lifecycle | 无 |
| [router_example.cpp](router_example.cpp) | 路由策略：广播/轮询/随机/一致性哈希 | Router | 无 |
| [middleware_example.cpp](middleware_example.cpp) | 中间件：Receiver/Sender/Spawn中间件、Context装饰器 | Middleware | 无 |
| [persistence_example.cpp](persistence_example.cpp) | 持久化：事件溯源和快照 | Persistence | Protobuf |
| [remote_example.cpp](remote_example.cpp) | 远程通信：节点间消息传递、远程Actor创建 | Remote | gRPC |
| [cluster_example.cpp](cluster_example.cpp) | 集群：成员管理、Gossip协议、虚拟Actor | Cluster | gRPC |
| [perf_benchmark.cpp](perf_benchmark.cpp) | 性能基准测试 | - | 无 |

## 构建运行

```bash
# 构建示例（基础示例）
mkdir build && cd build
cmake .. -DBUILD_EXAMPLES=ON
make -j$(nproc)

# 运行基础示例
./bin/hello_world
./bin/supervision_example
./bin/behavior_example
./bin/router_example
./bin/middleware_example
./bin/perf_benchmark

# 构建远程/集群/持久化示例（需要 gRPC/Protobuf 支持）
cmake .. -DBUILD_EXAMPLES=ON -DENABLE_GRPC=ON -DENABLE_PROTOBUF=ON
make -j$(nproc)

# 运行持久化示例
./bin/persistence_example

# 运行远程示例
# 终端 1 - 启动服务器
./bin/remote_example server localhost 8090
# 终端 2 - 启动客户端
./bin/remote_example client localhost:8090

# 运行集群示例
./bin/cluster_example node mycluster localhost 8090
```

## 待补充示例

以下示例可作为未来工作参考：

### 低优先级

| 示例文件 | 功能 | 参考Go代码 |
|---------|------|-----------|
| `pubsub_example.cpp` | 发布订阅模式 | protoactor-go/cluster/pubsub |
| `metrics_example.cpp` | 指标收集 | - |
| `stream_example.cpp` | 流处理 | - |

## 示例模板

### 基础Actor示例

```cpp
#include "protoactor/actor.h"
#include "protoactor/context.h"
#include "protoactor/actor_system.h"
#include "protoactor/props.h"
#include <iostream>
#include <memory>

// 定义消息类型
struct MyMessage {
    std::string content;
};

// 实现Actor
class MyActor : public protoactor::Actor {
public:
    void Receive(std::shared_ptr<protoactor::Context> context) override {
        if (auto msg = context->MessageAs<MyMessage>()) {
            std::cout << "Received: " << msg->content << std::endl;
        }
    }
};

int main() {
    auto system = protoactor::ActorSystem::New();
    auto root = system->GetRoot();

    auto props = protoactor::Props::FromProducer([]() {
        return std::make_shared<MyActor>();
    });

    auto pid = root->Spawn(props);
    root->Send(pid, std::make_shared<MyMessage>(MyMessage{"Hello!"}));

    system->Shutdown();
    return 0;
}
```

## 与Go版本API对比

| 特性 | Go | C++ |
|------|-----|-----|
| 消息定义 | `struct{ Who string }` | `struct { std::string who; }` |
| Actor定义 | `type HelloActor struct{}` | `class HelloActor : public Actor` |
| 消息处理 | `Receive(context actor.Context)` | `Receive(Context context) override` |
| 类型匹配 | `switch msg := context.Message().(type)` | `context->MessageAs<T>()` |
| 创建Actor | `context.Spawn(props)` | `root->Spawn(props)` |
| 发送消息 | `context.Send(pid, msg)` | `root->Send(pid, msg)` |

---

测试用例请参见 [tests/](../tests/) 目录。
