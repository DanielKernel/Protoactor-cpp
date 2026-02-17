# ProtoActor-Go 到 ProtoActor-C++ 迁移指南

本指南帮助开发者从 ProtoActor-Go 迁移到 ProtoActor-C++，涵盖概念映射、API 差异和常见模式转换。

## 概念映射

| 概念 | Go | C++ |
|------|-----|-----|
| Actor 接口 | `actor.Actor` | `protoactor::Actor` |
| 进程标识符 | `actor.PID` | `protoactor::PID` |
| 上下文 | `actor.Context` / `actor.RootContext` | `protoactor::Context` |
| 属性配置 | `actor.Props` | `protoactor::Props` |
| Actor 系统 | `actor.ActorSystem` | `protoactor::ActorSystem` |
| 消息 | `interface{}` | `std::shared_ptr<void>` |
| 错误处理 | `error` | `std::error_code` / 异常 |

## 基础 Actor 创建

### Go 版本

```go
package main

import (
    "github.com/asynkron/protoactor-go/actor"
)

type Hello struct{ Who string }
type HelloActor struct{}

func (a *HelloActor) Receive(ctx actor.Context) {
    switch msg := ctx.Message().(type) {
    case *Hello:
        println("Hello", msg.Who)
    }
}

func main() {
    system := actor.NewActorSystem()
    root := system.Root

    props := actor.PropsFromProducer(func() actor.Actor {
        return &HelloActor{}
    })

    pid := root.Spawn(props)
    root.Send(pid, &Hello{Who: "World"})
}
```

### C++ 版本

```cpp
#include "protoactor/actor.h"
#include "protoactor/context.h"
#include "protoactor/actor_system.h"
#include <iostream>
#include <memory>

struct Hello {
    std::string who;
};

class HelloActor : public protoactor::Actor {
public:
    void Receive(std::shared_ptr<protoactor::Context> context) override {
        auto msg = context->Message();
        if (!msg) return;

        void* ptr = msg.get();
        if (Hello* hello = static_cast<Hello*>(ptr)) {
            std::cout << "Hello " << hello->who << std::endl;
        }
    }
};

int main() {
    auto system = protoactor::ActorSystem::New();
    auto root = system->GetRoot();

    auto props = protoactor::Props::FromProducer([]() {
        return std::make_shared<HelloActor>();
    });

    auto pid = root->Spawn(props);
    root->Send(pid, std::make_shared<Hello>(Hello{"World"}));

    system->Shutdown();
    return 0;
}
```

## 消息类型处理

### Go: Type Switch

```go
func (a *MyActor) Receive(ctx actor.Context) {
    switch msg := ctx.Message().(type) {
    case *Hello:
        fmt.Println("Hello:", msg.Who)
    case *Ping:
        ctx.Respond(&Pong{Id: msg.Id})
    case *actor.Started:
        fmt.Println("Actor started")
    case *actor.Stopped:
        fmt.Println("Actor stopped")
    }
}
```

### C++: Static Cast / Dynamic Cast

```cpp
void Receive(std::shared_ptr<protoactor::Context> context) override {
    auto msg = context->Message();
    if (!msg) return;

    void* ptr = msg.get();

    // 方式1: static_cast (性能更好，需确保类型正确)
    if (Hello* hello = static_cast<Hello*>(ptr)) {
        std::cout << "Hello: " << hello->who << std::endl;
        return;
    }

    if (Ping* ping = static_cast<Ping*>(ptr)) {
        context->Respond(std::make_shared<Pong>(ping->id));
        return;
    }

    // 方式2: dynamic_cast (用于生命周期消息)
    if (dynamic_cast<protoactor::Started*>(ptr)) {
        std::cout << "Actor started" << std::endl;
        return;
    }

    if (dynamic_cast<protoactor::Stopped*>(ptr)) {
        std::cout << "Actor stopped" << std::endl;
        return;
    }
}
```

### C++: 类型安全辅助函数

```cpp
// 定义辅助函数简化消息处理
template<typename T, typename Handler>
bool HandleMessage(std::shared_ptr<void> msg, Handler handler) {
    void* ptr = msg.get();
    if (T* typed = static_cast<T*>(ptr)) {
        std::shared_ptr<T> typed_msg(msg, typed);
        handler(typed_msg);
        return true;
    }
    return false;
}

// 使用
void Receive(std::shared_ptr<protoactor::Context> context) override {
    auto msg = context->Message();

    HandleMessage<Hello>(msg, [](std::shared_ptr<Hello> h) {
        std::cout << "Hello: " << h->who << std::endl;
    });

    HandleMessage<Ping>(msg, [context](std::shared_ptr<Ping> p) {
        context->Respond(std::make_shared<Pong>(p->id));
    });
}
```

## 消息传递

### 发送消息

| 操作 | Go | C++ |
|------|-----|-----|
| 发送（异步） | `ctx.Send(pid, msg)` | `ctx->Send(pid, msg)` |
| 请求（异步） | `ctx.Request(pid, msg)` | `ctx->Request(pid, msg)` |
| 请求（Future） | `ctx.RequestFuture(pid, msg, timeout)` | `ctx->RequestFuture(pid, msg, timeout)` |
| 响应 | `ctx.Respond(msg)` | `ctx->Respond(msg)` |

### Request-Future 模式

**Go:**

```go
future := ctx.RequestFuture(pid, &Ping{Id: 1}, 5*time.Second)
result, err := future.Result()
if err == nil {
    pong := result.(*Pong)
    fmt.Println("Got pong:", pong.Id)
}
```

**C++:**

```cpp
auto future = context->RequestFuture(pid, std::make_shared<Ping>(1), std::chrono::seconds(5));
auto result = future->Result(std::chrono::seconds(5));
if (result) {
    void* ptr = result.get();
    if (Pong* pong = static_cast<Pong*>(ptr)) {
        std::cout << "Got pong: " << pong->id << std::endl;
    }
}
```

## 监督策略

### Go

```go
import "github.com/asynkron/protoactor-go/actor"

func NewSupervisorStrategy() actor.SupervisorStrategy {
    return actor.NewOneForOneStrategy(10, 1000, func(reason interface{}) actor.Directive {
        log.Printf("Actor failed: %v", reason)
        return actor.RestartDirective
    })
}

func main() {
    props := actor.PropsFromProducer(NewChild).
        WithSupervisor(NewSupervisorStrategy())
}
```

### C++

```cpp
#include "protoactor/supervision.h"

auto decider = [](std::shared_ptr<void> reason) -> protoactor::Directive {
    std::cout << "Actor failed" << std::endl;
    return protoactor::Directive::Restart;
};

auto strategy = protoactor::NewOneForOneStrategy(
    10,  // max retries
    std::chrono::milliseconds(1000),  // within time window
    decider
);

auto props = protoactor::Props::FromProducer([](){ return std::make_shared<ChildActor>(); })
    ->WithSupervisor(strategy);
```

## 行为管理

### Go: SetBehavior / PushBehavior

```go
type BehaviorActor struct {
    behavior actor.Behavior
}

func (a *BehaviorActor) Receive(ctx actor.Context) {
    a.behavior.Receive(ctx)
}

func (a *BehaviorActor) Initial(ctx actor.Context) {
    switch msg := ctx.Message().(type) {
    case *Hello:
        fmt.Println("Initial:", msg.Who)
        ctx.SetBehavior(a.Other)
    }
}

func (a *BehaviorActor) Other(ctx actor.Context) {
    switch msg := ctx.Message().(type) {
    case *Hello:
        fmt.Println("Other:", msg.Who)
        ctx.SetBehavior(a.Initial)
    }
}
```

### C++: Behavior 类

```cpp
#include "protoactor/behavior.h"

class BehaviorActor : public protoactor::Actor {
public:
    BehaviorActor() : behavior_(protoactor::Behavior::New()) {}

    void Receive(std::shared_ptr<protoactor::Context> context) override {
        if (behavior_->Length() > 0) {
            behavior_->Receive(context);
            return;
        }
        // Initial behavior
        // ...
    }

private:
    std::shared_ptr<protoactor::Behavior> behavior_;
};
```

## 远程通信

### Go

```go
import "github.com/asynkron/protoactor-go/remote"

func main() {
    remote.Start("localhost:8090")

    props := actor.PropsFromProducer(NewActor)
    remote.Register("myactor", props)

    // 远程创建
    resp, err := remote.SpawnNamed("localhost:8091", "myactor", "remote-1", time.Second)
    if err == nil {
        ctx.Send(resp.Pid, &Hello{Who: "Remote"})
    }
}
```

### C++

```cpp
#include "protoactor/remote/remote.h"

int main() {
    auto system = protoactor::ActorSystem::New();
    auto remote = protoactor::remote::Remote::Start(system, "localhost", 8090);

    auto props = protoactor::Props::FromProducer([](){ return std::make_shared<MyActor>(); });
    remote->Register("myactor", props);

    // 远程创建
    auto [pid, err] = remote->SpawnNamed(
        "localhost:8091",
        "myactor",
        "remote-1",
        std::chrono::seconds(5)
    );

    if (!err) {
        auto root = system->GetRoot();
        root->Send(pid, std::make_shared<Hello>("Remote"));
    }
}
```

## 集群支持

### Go

```go
import "github.com/asynkron/protoactor-go/cluster"

func main() {
    config := cluster.Configure("mycluster", "localhost", 8090, ...)
    c := cluster.New(actorSystem, config)
    c.Start()

    // 获取虚拟 Actor
    grain := c.Get("my-grain", "grain-id")
    ctx.Send(grain, &Hello{Who: "Cluster"})
}
```

### C++

```cpp
#include "protoactor/cluster/cluster.h"

int main() {
    auto system = protoactor::ActorSystem::New();

    auto config = protoactor::cluster::Config::New("mycluster", "localhost", 8090);
    auto cluster = protoactor::cluster::Cluster::New(system, config);
    cluster->Start();

    // 获取虚拟 Actor
    auto grain = cluster->Get("my-grain", "grain-id");
    auto root = system->GetRoot();
    root->Send(grain, std::make_shared<Hello>("Cluster"));
}
```

## 中间件

### Go

```go
func LoggingMiddleware(next actor.ReceiverFunc) actor.ReceiverFunc {
    return func(ctx actor.Context, envelope *actor.MessageEnvelope) {
        log.Println("Received message")
        next(ctx, envelope)
    }
}

func main() {
    props := actor.PropsFromProducer(NewActor).
        WithReceiverMiddleware(LoggingMiddleware)
}
```

### C++

```cpp
protoactor::ReceiverMiddleware LoggingMiddleware() {
    return [](std::function<void(std::shared_ptr<protoactor::Context>, std::shared_ptr<protoactor::MessageEnvelope>)> next) {
        return [next](std::shared_ptr<protoactor::Context> ctx, std::shared_ptr<protoactor::MessageEnvelope> env) {
            std::cout << "Received message" << std::endl;
            next(ctx, env);
        };
    };
}

int main() {
    auto props = protoactor::Props::FromProducer([](){ return std::make_shared<MyActor>(); })
        ->WithReceiverMiddleware({LoggingMiddleware()});
}
```

## 持久化

### Go

```go
import "github.com/asynkron/protoactor-go/persistence"

type PersistentActor struct {
    persistence.Mixin
    state int
}

func (a *PersistentActor) Receive(ctx actor.Context) {
    switch msg := ctx.Message().(type) {
    case *persistence.RequestSnapshot:
        ctx.PersistSnapshot(&StateSnapshot{State: a.state})
    case *persistence.ReplayComplete:
        fmt.Println("Replay complete")
    case *Increment:
        ctx.PersistEvent(&Incremented{})
    case *Incremented:
        a.state++
    }
}
```

### C++

```cpp
#include "protoactor/persistence/persistence.h"

class PersistentActor : public protoactor::Actor {
public:
    PersistentActor(std::shared_ptr<protoactor::persistence::ProviderState> provider)
        : provider_(provider) {}

    void Receive(std::shared_ptr<protoactor::Context> context) override {
        auto msg = context->Message();

        HandleMessage<Increment>(msg, [this, context](auto) {
            // 持久化事件
            provider_->PersistEvent("my-actor", event_index_++, event);
            // 应用事件
            state_++;
        });

        HandleMessage<Incremented>(msg, [this](auto) {
            state_++;
        });
    }

private:
    std::shared_ptr<protoactor::persistence::ProviderState> provider_;
    int state_ = 0;
    int event_index_ = 0;
};
```

## 常见陷阱

### 1. 消息类型匹配

**问题**: Go 的 type switch 是类型安全的，C++ 的 static_cast 不是。

**解决方案**: 使用模板辅助函数或确保类型正确。

```cpp
// 推荐：使用辅助函数
template<typename T>
std::shared_ptr<T> MessageAs(std::shared_ptr<void> msg) {
    return std::static_pointer_cast<T>(msg);
}
```

### 2. 智能指针

**问题**: Go 有 GC，C++ 需要手动管理内存。

**解决方案**: 始终使用 `std::shared_ptr` 管理消息和 Actor 生命周期。

```cpp
// 正确
root->Send(pid, std::make_shared<Hello>("World"));

// 错误（会导致悬空指针）
// Hello* msg = new Hello{"World"};  // 不要这样做
```

### 3. 并发

**问题**: Go 有 goroutine，C++ 需要线程池。

**解决方案**: ProtoActor-C++ 内置线程池和调度器，无需手动管理。

### 4. 错误处理

**问题**: Go 使用 error 返回值，C++ 使用异常或 std::error_code。

**解决方案**: 检查返回值中的错误。

```cpp
auto [pid, err] = root->Spawn(props);
if (err) {
    std::cerr << "Spawn failed: " << err.message() << std::endl;
    return;
}
```

### 5. 字符串处理

**问题**: Go 字符串是值类型，C++ std::string 也是值类型但需要更多注意。

**解决方案**: 使用 const 引用传递，移动语义优化。

```cpp
struct Message {
    std::string data;
    // 优化：移动构造
    explicit Message(std::string d) : data(std::move(d)) {}
};
```

## 性能建议

1. **消息池**: 对频繁创建的消息使用对象池
2. **避免拷贝**: 使用 `std::move` 和智能指针
3. **批量处理**: 批量发送消息减少锁竞争
4. **线程亲和**: 考虑使用同步调度器减少上下文切换

## 参考资源

- [ProtoActor-Go GitHub](https://github.com/asynkron/protoactor-go)
- [ProtoActor-C++ 功能对比](COMPARISON.md)
- [远程通信指南](REMOTE_GUIDE.md)
- [示例代码](../examples/)
