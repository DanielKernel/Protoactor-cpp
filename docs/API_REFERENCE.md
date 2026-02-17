# ProtoActor C++ API 参考文档

本文档提供 ProtoActor C++ 的完整 API 参考。

## 目录

- [核心模块](#核心模块)
  - [Actor](#actor)
  - [PID](#pid)
  - [Context](#context)
  - [Props](#props)
  - [ActorSystem](#actorsystem)
- [消息传递](#消息传递)
  - [MessageEnvelope](#messageenvelope)
  - [Future](#future)
- [生命周期](#生命周期)
  - [Started](#started)
  - [Stopping](#stopping)
  - [Stopped](#stopped)
  - [Restarting](#restarting)
- [行为管理](#行为管理)
  - [Behavior](#behavior)
- [监督策略](#监督策略)
  - [SupervisorStrategy](#supervisorstrategy)
  - [Directive](#directive)
- [中间件](#中间件)
  - [ReceiverMiddleware](#receivermiddleware)
  - [SenderMiddleware](#sendermiddleware)
  - [SpawnMiddleware](#spawnmiddleware)
  - [ContextDecorator](#contextdecorator)
- [路由](#路由)
  - [Router](#router)
  - [BroadcastRouter](#broadcastrouter)
  - [RoundRobinRouter](#roundrobinrouter)
  - [RandomRouter](#randomrouter)
  - [ConsistentHashRouter](#consistenthashrouter)
- [远程通信](#远程通信)
  - [Remote](#remote)
- [集群](#集群)
  - [Cluster](#cluster)
  - [MemberList](#memberlist)
- [持久化](#持久化)
  - [Provider](#provider)
  - [EventStore](#eventstore)
  - [SnapshotStore](#snapshotstore)
- [事件流](#事件流)
  - [EventStream](#eventstream)

---

## 核心模块

### Actor

Actor 接口是所有 Actor 的基类。

```cpp
namespace protoactor {

class Actor {
public:
    virtual ~Actor() = default;

    /**
     * @brief 处理接收到的消息
     * @param context Actor 上下文，包含消息和操作方法
     */
    virtual void Receive(std::shared_ptr<Context> context) = 0;
};

} // namespace protoactor
```

**使用示例：**

```cpp
class MyActor : public protoactor::Actor {
public:
    void Receive(std::shared_ptr<protoactor::Context> context) override {
        auto msg = context->Message();
        // 处理消息...
    }
};
```

---

### PID

PID (Process Identifier) 是 Actor 的唯一标识符。

```cpp
namespace protoactor {

class PID : public std::enable_shared_from_this<PID> {
public:
    /**
     * @brief 创建新的 PID
     * @param address 节点地址 (如 "localhost:8090")
     * @param id Actor 标识符
     * @return PID 共享指针
     */
    static std::shared_ptr<PID> New(const std::string& address, const std::string& id);

    /**
     * @brief 获取完整地址
     * @return 格式: "address/id"
     */
    std::string Address() const;

    /**
     * @brief 获取节点地址
     */
    const std::string& Host() const;

    /**
     * @brief 获取 Actor ID
     */
    const std::string& Id() const;

    /**
     * @brief 向此 Actor 发送消息
     * @param message 消息指针
     */
    void Tell(std::shared_ptr<void> message);

    /**
     * @brief 向此 Actor 发送消息并期望响应
     * @param message 消息指针
     * @param sender 发送者 PID
     */
    void Request(std::shared_ptr<void> message, std::shared_ptr<PID> sender);
};

} // namespace protoactor
```

---

### Context

Context 提供 Actor 运行时上下文和操作方法。

```cpp
namespace protoactor {

class Context {
public:
    virtual ~Context() = default;

    // === 基本信息 ===

    /**
     * @brief 获取当前 Actor 的 PID
     */
    virtual std::shared_ptr<PID> Self() const = 0;

    /**
     * @brief 获取父 Actor 的 PID
     */
    virtual std::shared_ptr<PID> Parent() const = 0;

    /**
     * @brief 获取消息发送者的 PID
     */
    virtual std::shared_ptr<PID> Sender() const = 0;

    /**
     * @brief 获取当前消息
     */
    virtual std::shared_ptr<void> Message() const = 0;

    /**
     * @brief 获取消息信封
     */
    virtual std::shared_ptr<MessageEnvelope> MessageEnvelope() const = 0;

    /**
     * @brief 获取消息头部
     */
    virtual MessageHeader* Headers() const = 0;

    // === 消息发送 ===

    /**
     * @brief 发送消息给目标 Actor
     * @param target 目标 PID
     * @param message 消息
     */
    virtual void Send(std::shared_ptr<PID> target, std::shared_ptr<void> message) = 0;

    /**
     * @brief 发送消息并期望响应
     * @param target 目标 PID
     * @param message 消息
     */
    virtual void Request(std::shared_ptr<PID> target, std::shared_ptr<void> message) = 0;

    /**
     * @brief 发送消息并返回 Future
     * @param target 目标 PID
     * @param message 消息
     * @param timeout 超时时间
     * @return Future 对象
     */
    virtual std::shared_ptr<Future> RequestFuture(
        std::shared_ptr<PID> target,
        std::shared_ptr<void> message,
        std::chrono::milliseconds timeout) = 0;

    /**
     * @brief 响应当前消息
     * @param message 响应消息
     */
    virtual void Respond(std::shared_ptr<void> message) = 0;

    // === Actor 生命周期 ===

    /**
     * @brief 创建子 Actor
     * @param props Actor 配置
     * @return 子 Actor 的 PID
     */
    virtual std::shared_ptr<PID> Spawn(std::shared_ptr<Props> props) = 0;

    /**
     * @brief 创建具名子 Actor
     * @param props Actor 配置
     * @param name Actor 名称
     * @return 子 Actor 的 PID
     */
    virtual std::shared_ptr<PID> SpawnNamed(std::shared_ptr<Props> props, const std::string& name) = 0;

    /**
     * @brief 停止 Actor
     * @param pid 要停止的 Actor PID
     */
    virtual void Stop(std::shared_ptr<PID> pid) = 0;

    /**
     * @brief 毒杀 Actor (处理完当前消息后停止)
     * @param pid 要毒杀的 Actor PID
     */
    virtual void Poison(std::shared_ptr<PID> pid) = 0;

    // === 行为管理 ===

    /**
     * @brief 设置新行为 (替换当前)
     * @param behavior 新行为函数
     */
    virtual void SetBehavior(std::function<void(std::shared_ptr<Context>)> behavior) = 0;

    /**
     * @brief 推入行为 (压栈)
     * @param behavior 新行为函数
     */
    virtual void PushBehavior(std::function<void(std::shared_ptr<Context>)> behavior) = 0;

    /**
     * @brief 弹出行为 (恢复上一个)
     */
    virtual void PopBehavior() = 0;

    // === 子 Actor 管理 ===

    /**
     * @brief 获取所有子 Actor
     * @return 子 Actor PID 集合
     */
    virtual std::vector<std::shared_ptr<PID>> Children() const = 0;

    /**
     * @brief 重新启动 Actor
     * @param pid 要重启的 Actor PID
     */
    virtual void Restart(std::shared_ptr<PID> pid) = 0;
};

} // namespace protoactor
```

---

### Props

Props 用于配置 Actor 的创建方式。

```cpp
namespace protoactor {

class Props : public std::enable_shared_from_this<Props> {
public:
    /**
     * @brief 从生产者函数创建 Props
     * @param producer 返回 Actor 实例的函数
     * @return Props 实例
     */
    static std::shared_ptr<Props> FromProducer(Producer producer);

    /**
     * @brief 从接收函数创建 Props
     * @param receive 接收消息的函数
     * @return Props 实例
     */
    static std::shared_ptr<Props> FromFunc(std::function<void(std::shared_ptr<Context>)> receive);

    // === 配置方法 (返回新 Props，支持链式调用) ===

    /**
     * @brief 设置调度器
     */
    std::shared_ptr<Props> WithDispatcher(std::shared_ptr<Dispatcher> dispatcher);

    /**
     * @brief 设置邮箱生产者
     */
    std::shared_ptr<Props> WithMailboxProducer(std::function<std::shared_ptr<Mailbox>()> producer);

    /**
     * @brief 设置监督策略
     */
    std::shared_ptr<Props> WithSupervisor(std::shared_ptr<SupervisorStrategy> supervisor);

    /**
     * @brief 设置接收中间件
     */
    std::shared_ptr<Props> WithReceiverMiddleware(std::vector<ReceiverMiddleware> middleware);

    /**
     * @brief 设置发送中间件
     */
    std::shared_ptr<Props> WithSenderMiddleware(std::vector<SenderMiddleware> middleware);

    /**
     * @brief 设置创建中间件
     */
    std::shared_ptr<Props> WithSpawnMiddleware(std::vector<SpawnMiddleware> middleware);

    /**
     * @brief 设置上下文装饰器
     */
    std::shared_ptr<Props> WithContextDecorator(std::vector<ContextDecorator> decorators);

    // === Getter 方法 ===

    std::shared_ptr<Dispatcher> GetDispatcher() const;
    std::shared_ptr<SupervisorStrategy> GetSupervisor() const;
    std::shared_ptr<Mailbox> ProduceMailbox() const;
    ProducerWithActorSystem GetProducer() const;
};

// 生产者函数类型
using Producer = std::function<std::shared_ptr<Actor>()>;
using ProducerWithActorSystem = std::function<std::shared_ptr<Actor>(std::shared_ptr<ActorSystem>)>;

} // namespace protoactor
```

**使用示例：**

```cpp
auto props = protoactor::Props::FromProducer([]() {
    return std::make_shared<MyActor>();
})
->WithDispatcher(protoactor::NewDefaultDispatcher())
->WithSupervisor(protoactor::NewOneForOneStrategy(10, std::chrono::seconds(10), decider))
->WithReceiverMiddleware({LoggingMiddleware()});
```

---

### ActorSystem

ActorSystem 是 Actor 运行的容器。

```cpp
namespace protoactor {

class ActorSystem : public std::enable_shared_from_this<ActorSystem> {
public:
    /**
     * @brief 创建新的 ActorSystem
     * @return ActorSystem 共享指针
     */
    static std::shared_ptr<ActorSystem> New();

    /**
     * @brief 获取根上下文
     * @return 根上下文，用于创建顶级 Actor
     */
    std::shared_ptr<Context> GetRoot();

    /**
     * @brief 获取事件流
     * @return 事件流实例
     */
    std::shared_ptr<EventStream> EventStream();

    /**
     * @brief 关闭 ActorSystem
     */
    void Shutdown();

    /**
     * @brief 获取进程注册表
     */
    std::shared_ptr<ProcessRegistry> ProcessRegistry();
};

} // namespace protoactor
```

---

## 消息传递

### MessageEnvelope

消息信封，包装消息和元数据。

```cpp
namespace protoactor {

struct MessageEnvelope {
    std::shared_ptr<void> message;
    std::shared_ptr<PID> sender;
    MessageHeader* headers;

    /**
     * @brief 创建消息信封
     */
    static std::shared_ptr<MessageEnvelope> Wrap(
        std::shared_ptr<void> message,
        std::shared_ptr<PID> sender = nullptr,
        MessageHeader* headers = nullptr);

    /**
     * @brief 获取消息头部值
     * @param key 头部键
     * @return 头部值，不存在返回空字符串
     */
    std::string GetHeader(const std::string& key) const;

    /**
     * @brief 设置消息头部
     */
    void SetHeader(const std::string& key, const std::string& value);
};

} // namespace protoactor
```

---

### Future

Future 用于异步获取 Request-Response 结果。

```cpp
namespace protoactor {

class Future {
public:
    /**
     * @brief 等待并获取结果
     * @param timeout 超时时间
     * @return 结果消息，超时返回 nullptr
     */
    std::shared_ptr<void> Result(std::chrono::milliseconds timeout);

    /**
     * @brief 检查是否已完成
     */
    bool IsCompleted() const;

    /**
     * @brief 等待完成
     */
    void Wait(std::chrono::milliseconds timeout);
};

} // namespace protoactor
```

**使用示例：**

```cpp
auto future = context->RequestFuture(target, std::make_shared<Ping>(), std::chrono::seconds(5));
auto result = future->Result(std::chrono::seconds(5));
if (result) {
    // 处理结果
}
```

---

## 生命周期

### Started

Actor 启动时接收的消息。

```cpp
namespace protoactor {

struct Started {};

} // namespace protoactor
```

### Stopping

Actor 即将停止时接收的消息。

```cpp
namespace protoactor {

struct Stopping {};

} // namespace protoactor
```

### Stopped

Actor 已停止后接收的消息。

```cpp
namespace protoactor {

struct Stopped {};

} // namespace protoactor
```

### Restarting

Actor 即将重启时接收的消息。

```cpp
namespace protoactor {

struct Restarting {};

} // namespace protoactor
```

**使用示例：**

```cpp
void Receive(std::shared_ptr<protoactor::Context> context) override {
    void* ptr = context->Message().get();

    if (dynamic_cast<protoactor::Started*>(ptr)) {
        std::cout << "Actor started" << std::endl;
    }
    else if (dynamic_cast<protoactor::Stopping*>(ptr)) {
        std::cout << "Actor stopping" << std::endl;
    }
    else if (dynamic_cast<protoactor::Stopped*>(ptr)) {
        std::cout << "Actor stopped" << std::endl;
    }
}
```

---

## 行为管理

### Behavior

Behavior 类用于实现行为栈。

```cpp
namespace protoactor {

class Behavior {
public:
    /**
     * @brief 创建 Behavior 实例
     */
    static std::shared_ptr<Behavior> New();

    /**
     * @brief 设置行为 (替换当前)
     * @param behavior 新行为函数
     */
    void Become(std::function<void(std::shared_ptr<Context>)> behavior);

    /**
     * @brief 推入行为 (压栈)
     * @param behavior 新行为函数
     */
    void BecomeStacked(std::function<void(std::shared_ptr<Context>)> behavior);

    /**
     * @brief 弹出行为 (恢复上一个)
     */
    void UnbecomeStacked();

    /**
     * @brief 获取行为栈深度
     */
    int Length() const;

    /**
     * @brief 使用当前行为处理消息
     */
    void Receive(std::shared_ptr<Context> context);
};

} // namespace protoactor
```

---

## 监督策略

### SupervisorStrategy

监督策略接口。

```cpp
namespace protoactor {

class SupervisorStrategy {
public:
    virtual ~SupervisorStrategy() = default;

    /**
     * @brief 处理子 Actor 失败
     * @param pid 失败的 Actor PID
     * @param reason 失败原因
     * @param context 上下文
     */
    virtual void HandleFailure(
        std::shared_ptr<PID> pid,
        std::shared_ptr<void> reason,
        std::shared_ptr<Context> context) = 0;
};

/**
 * @brief 创建 OneForOne 策略
 * @param maxRetries 最大重试次数
 * @param withinTimeWindow 时间窗口
 * @param decider 决策函数
 */
std::shared_ptr<SupervisorStrategy> NewOneForOneStrategy(
    int maxRetries,
    std::chrono::milliseconds withinTimeWindow,
    std::function<Directive(std::shared_ptr<void>)> decider);

/**
 * @brief 创建 AllForOne 策略
 */
std::shared_ptr<SupervisorStrategy> NewAllForOneStrategy(
    int maxRetries,
    std::chrono::milliseconds withinTimeWindow,
    std::function<Directive(std::shared_ptr<void>)> decider);

} // namespace protoactor
```

### Directive

监督指令枚举。

```cpp
namespace protoactor {

enum class Directive {
    Resume,     // 继续处理下一条消息
    Restart,    // 重启 Actor
    Stop,       // 停止 Actor
    Escalate    // 上报给父 Actor
};

} // namespace protoactor
```

**使用示例：**

```cpp
auto decider = [](std::shared_ptr<void> reason) -> protoactor::Directive {
    std::cout << "Actor failed" << std::endl;
    return protoactor::Directive::Restart;
};

auto strategy = protoactor::NewOneForOneStrategy(
    10,  // 最多重启 10 次
    std::chrono::milliseconds(60000),  // 60 秒窗口
    decider
);

auto props = protoactor::Props::FromProducer([](){ return std::make_shared<MyActor>(); })
    ->WithSupervisor(strategy);
```

---

## 中间件

### ReceiverMiddleware

接收消息中间件类型。

```cpp
namespace protoactor {

using ReceiverMiddleware = std::function<
    std::function<void(std::shared_ptr<Context>, std::shared_ptr<MessageEnvelope>)>(
        std::function<void(std::shared_ptr<Context>, std::shared_ptr<MessageEnvelope>)>)>;

} // namespace protoactor
```

### SenderMiddleware

发送消息中间件类型。

```cpp
namespace protoactor {

using SenderMiddleware = std::function<
    std::function<void(std::shared_ptr<Context>, std::shared_ptr<PID>, std::shared_ptr<MessageEnvelope>)>(
        std::function<void(std::shared_ptr<Context>, std::shared_ptr<PID>, std::shared_ptr<MessageEnvelope>)>)>;

} // namespace protoactor
```

### SpawnMiddleware

创建 Actor 中间件类型。

```cpp
namespace protoactor {

using SpawnMiddleware = std::function<SpawnFunc(SpawnFunc)>;

} // namespace protoactor
```

### ContextDecorator

上下文装饰器类型。

```cpp
namespace protoactor {

using ContextDecorator = std::function<
    std::function<std::shared_ptr<Context>(std::shared_ptr<Context>)>(
        std::function<std::shared_ptr<Context>(std::shared_ptr<Context>)>)>;

} // namespace protoactor
```

**使用示例：**

```cpp
// 日志中间件
protoactor::ReceiverMiddleware LoggingMiddleware() {
    return [](auto next) {
        return [next](auto ctx, auto env) {
            std::cout << "Received message" << std::endl;
            next(ctx, env);
        };
    };
}

// 使用
auto props = protoactor::Props::FromProducer([](){ return std::make_shared<MyActor>(); })
    ->WithReceiverMiddleware({LoggingMiddleware()});
```

---

## 路由

### Router

路由器基类接口。

```cpp
namespace protoactor {
namespace router {

class Router {
public:
    virtual ~Router() = default;

    /**
     * @brief 路由消息
     */
    virtual void RouteMessage(std::shared_ptr<void> message) = 0;

    /**
     * @brief 设置路由目标
     */
    virtual void SetRoutees(const std::vector<std::shared_ptr<PID>>& routees) = 0;

    /**
     * @brief 获取路由目标
     */
    virtual std::vector<std::shared_ptr<PID>> GetRoutees() const = 0;

    /**
     * @brief 设置发送者上下文
     */
    virtual void SetSender(std::shared_ptr<Context> sender) = 0;
};

} // namespace router
} // namespace protoactor
```

### BroadcastRouter

广播路由器 - 发送消息到所有目标。

```cpp
namespace protoactor {
namespace router {

class BroadcastRouter : public Router {
    // 发送到所有 routees
};

} // namespace router
} // namespace protoactor
```

### RoundRobinRouter

轮询路由器 - 依次分发消息。

```cpp
namespace protoactor {
namespace router {

class RoundRobinRouter : public Router {
    // 按顺序分发给每个 routee
};

} // namespace router
} // namespace protoactor
```

### RandomRouter

随机路由器 - 随机选择目标。

```cpp
namespace protoactor {
namespace router {

class RandomRouter : public Router {
    // 随机选择一个 routee
};

} // namespace router
} // namespace protoactor
```

### ConsistentHashRouter

一致性哈希路由器 - 基于消息哈希路由。

```cpp
namespace protoactor {
namespace router {

class ConsistentHashRouter : public Router {
    // 相同 key 的消息路由到相同 routee
};

} // namespace router
} // namespace protoactor
```

---

## 远程通信

### Remote

远程通信模块。

```cpp
namespace protoactor {
namespace remote {

class Remote {
public:
    /**
     * @brief 启动远程服务
     * @param system ActorSystem
     * @param host 监听地址
     * @param port 监听端口
     * @return Remote 实例
     */
    static std::shared_ptr<Remote> Start(
        std::shared_ptr<ActorSystem> system,
        const std::string& host,
        int port);

    /**
     * @brief 注册 Actor 类型
     * @param kind 类型名称
     * @param props Actor 配置
     */
    void Register(const std::string& kind, std::shared_ptr<Props> props);

    /**
     * @brief 远程创建 Actor
     * @param address 远程节点地址
     * @param kind Actor 类型
     * @param name Actor 名称
     * @param timeout 超时时间
     * @return PID 和错误信息
     */
    std::pair<std::shared_ptr<PID>, std::error_code> SpawnNamed(
        const std::string& address,
        const std::string& kind,
        const std::string& name,
        std::chrono::milliseconds timeout);

    /**
     * @brief 关闭远程服务
     * @param wait 等待进行中的请求完成
     */
    void Shutdown(bool wait = true);

    /**
     * @brief 获取端点管理器
     */
    std::shared_ptr<EndpointManager> GetEndpointManager();

    /**
     * @brief 获取黑名单
     */
    std::shared_ptr<Blocklist> GetBlockList();
};

} // namespace remote
} // namespace protoactor
```

---

## 集群

### Cluster

集群模块。

```cpp
namespace protoactor {
namespace cluster {

class Cluster {
public:
    /**
     * @brief 创建集群实例
     * @param system ActorSystem
     * @param config 集群配置
     * @return Cluster 实例
     */
    static std::shared_ptr<Cluster> New(
        std::shared_ptr<ActorSystem> system,
        std::shared_ptr<Config> config);

    /**
     * @brief 启动集群
     */
    void Start();

    /**
     * @brief 关闭集群
     */
    void Shutdown();

    /**
     * @brief 获取虚拟 Actor (Grain)
     * @param kind Actor 类型
     * @param identity 唯一标识
     * @return Grain PID
     */
    std::shared_ptr<PID> Get(const std::string& kind, const std::string& identity);

    /**
     * @brief 获取成员列表
     */
    std::shared_ptr<MemberList> GetMemberList();

    /**
     * @brief 获取远程模块
     */
    std::shared_ptr<remote::Remote> GetRemote();

    /**
     * @brief 获取 Gossiper
     */
    std::shared_ptr<Gossiper> GetGossiper();
};

} // namespace cluster
} // namespace protoactor
```

### MemberList

集群成员列表。

```cpp
namespace protoactor {
namespace cluster {

class MemberList {
public:
    /**
     * @brief 获取所有成员
     */
    std::vector<std::shared_ptr<Member>> GetMembers() const;

    /**
     * @brief 获取成员数量
     */
    int MemberCount() const;

    /**
     * @brief 检查成员是否存在
     */
    bool MemberExists(const std::string& memberId) const;

    /**
     * @brief 获取特定类型的成员
     */
    std::vector<std::shared_ptr<Member>> GetMembersByKind(const std::string& kind) const;
};

} // namespace cluster
} // namespace protoactor
```

---

## 持久化

### Provider

持久化提供者接口。

```cpp
namespace protoactor {
namespace persistence {

class Provider {
public:
    virtual ~Provider() = default;

    /**
     * @brief 获取提供者状态
     */
    virtual std::shared_ptr<ProviderState> GetState() = 0;
};

} // namespace persistence
} // namespace protoactor
```

### EventStore

事件存储接口。

```cpp
namespace protoactor {
namespace persistence {

class EventStore {
public:
    virtual ~EventStore() = default;

    /**
     * @brief 获取事件
     * @param actorName Actor 名称
     * @param eventIndexStart 起始索引
     * @param eventIndexEnd 结束索引 (-1 表示全部)
     * @param callback 事件回调
     */
    virtual void GetEvents(
        const std::string& actorName,
        int eventIndexStart,
        int eventIndexEnd,
        std::function<void(std::shared_ptr<void>)> callback) = 0;

    /**
     * @brief 持久化事件
     */
    virtual void PersistEvent(
        const std::string& actorName,
        int eventIndex,
        std::shared_ptr<void> event) = 0;

    /**
     * @brief 删除事件
     */
    virtual void DeleteEvents(const std::string& actorName, int inclusiveToIndex) = 0;
};

} // namespace persistence
} // namespace protoactor
```

### SnapshotStore

快照存储接口。

```cpp
namespace protoactor {
namespace persistence {

class SnapshotStore {
public:
    virtual ~SnapshotStore() = default;

    /**
     * @brief 获取快照
     * @return 快照、索引、是否存在的元组
     */
    virtual std::tuple<std::shared_ptr<void>, int, bool>
        GetSnapshot(const std::string& actorName) = 0;

    /**
     * @brief 持久化快照
     */
    virtual void PersistSnapshot(
        const std::string& actorName,
        int snapshotIndex,
        std::shared_ptr<void> snapshot) = 0;

    /**
     * @brief 删除快照
     */
    virtual void DeleteSnapshots(const std::string& actorName, int inclusiveToIndex) = 0;
};

} // namespace persistence
} // namespace protoactor
```

### InMemoryProvider

内存持久化提供者 (用于测试)。

```cpp
namespace protoactor {
namespace persistence {

class InMemoryProvider : public Provider {
public:
    std::shared_ptr<ProviderState> GetState() override;
};

} // namespace persistence
} // namespace protoactor
```

---

## 事件流

### EventStream

发布-订阅事件流。

```cpp
namespace protoactor {

class EventStream {
public:
    /**
     * @brief 订阅事件
     * @param handler 事件处理函数
     * @return 订阅 ID
     */
    int Subscribe(std::function<void(std::shared_ptr<void>)> handler);

    /**
     * @brief 订阅特定类型事件
     * @param handler 事件处理函数
     * @param filter 类型过滤器
     * @return 订阅 ID
     */
    int Subscribe(std::function<void(std::shared_ptr<void>)> handler,
                  std::function<bool(std::shared_ptr<void>)> filter);

    /**
     * @brief 取消订阅
     * @param subscriptionId 订阅 ID
     */
    void Unsubscribe(int subscriptionId);

    /**
     * @brief 发布事件
     * @param event 事件对象
     */
    void Publish(std::shared_ptr<void> event);

    /**
     * @brief 获取订阅数量
     */
    int SubscriptionCount() const;
};

} // namespace protoactor
```

**使用示例：**

```cpp
auto eventStream = system->EventStream();

// 订阅事件
int subId = eventStream->Subscribe([](std::shared_ptr<void> event) {
    std::cout << "Received event" << std::endl;
});

// 发布事件
eventStream->Publish(std::make_shared<MyEvent>());

// 取消订阅
eventStream->Unsubscribe(subId);
```

---

## 调度器

### Dispatcher

调度器接口。

```cpp
namespace protoactor {

class Dispatcher {
public:
    virtual ~Dispatcher() = default;

    /**
     * @brief 调度执行
     */
    virtual void Schedule(std::function<void()> action) = 0;

    /**
     * @brief 获取调度器 ID
     */
    virtual int Id() const = 0;
};

/**
 * @brief 创建默认调度器
 */
std::shared_ptr<Dispatcher> NewDefaultDispatcher();

/**
 * @brief 创建同步调度器
 * @param throughput 每次处理的消息数
 */
std::shared_ptr<Dispatcher> NewSynchronizedDispatcher(int throughput);

} // namespace protoactor
```

---

## 测试工具

### TestProbe

测试探针，用于测试 Actor 行为。

```cpp
namespace protoactor {
namespace testkit {

class TestProbe {
public:
    /**
     * @brief 创建测试探针
     * @param system ActorSystem
     */
    static std::shared_ptr<TestProbe> New(std::shared_ptr<ActorSystem> system);

    /**
     * @brief 获取探针 PID
     */
    std::shared_ptr<PID>Pid();

    /**
     * @brief 发送消息
     */
    void Send(std::shared_ptr<PID> target, std::shared_ptr<void> message);

    /**
     * @brief 等待接收消息
     * @param timeout 超时时间
     * @return 接收到的消息
     */
    std::shared_ptr<void> Receive(std::chrono::milliseconds timeout);

    /**
     * @brief 期望收到特定类型消息
     */
    template<typename T>
    std::shared_ptr<T> ExpectMsg(std::chrono::milliseconds timeout);

    /**
     * @brief 期望没有消息
     */
    bool ExpectNoMsg(std::chrono::milliseconds timeout);
};

} // namespace testkit
} // namespace protoactor
```

---

## 类型定义汇总

```cpp
namespace protoactor {

// 生产者函数
using Producer = std::function<std::shared_ptr<Actor>()>;
using ProducerWithActorSystem = std::function<std::shared_ptr<Actor>(std::shared_ptr<ActorSystem>)>;

// 中间件类型
using ReceiverMiddleware = std::function<...>;
using SenderMiddleware = std::function<...>;
using SpawnMiddleware = std::function<...>;
using ContextDecorator = std::function<...>;

// 监督指令
enum class Directive { Resume, Restart, Stop, Escalate };

} // namespace protoactor
```

---

## 参考

- [ProtoActor-Go 文档](https://github.com/asynkron/protoactor-go)
- [迁移指南](MIGRATION_GUIDE.md)
- [示例代码](../examples/)
