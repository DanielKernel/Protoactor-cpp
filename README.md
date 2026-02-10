# ProtoActor C++

这是ProtoActor Go版本的C++11实现，确保功能100%继承。

## 项目结构

```
protoactor-cpp/
├── include/protoactor/     # 头文件
│   ├── actor.h             # Actor接口
│   ├── pid.h               # Process Identifier
│   ├── context.h           # Actor上下文
│   ├── props.h             # Actor属性配置
│   ├── actor_system.h      # Actor系统
│   ├── process.h           # 进程接口
│   ├── process_registry.h  # 进程注册表
│   ├── mailbox.h           # 邮箱
│   ├── dispatcher.h        # 调度器
│   ├── future.h            # Future异步结果
│   ├── supervision.h      # 监督策略
│   └── messages.h          # 消息定义
├── src/                    # 实现文件
│   ├── actor/              # 核心actor模块
│   ├── remote/             # 远程通信模块
│   ├── cluster/            # 集群模块
│   ├── router/             # 路由模块
│   ├── persistence/        # 持久化模块
│   ├── scheduler/          # 调度器模块
│   ├── stream/             # 流处理模块
│   ├── eventstream/        # 事件流模块
│   └── metrics/            # 指标模块
├── proto/                  # Protobuf定义
├── examples/               # 示例代码
├── tests/                  # 测试代码
└── CMakeLists.txt         # CMake构建文件
```

## 构建要求

- **Linux操作系统** (Ubuntu, Debian, CentOS, RHEL等)
- C++11或更高版本的编译器（GCC 4.8+, Clang 3.3+）
- CMake 3.10+
- pthread库
- Protobuf (可选，用于消息序列化)
- gRPC (可选，用于远程通信)

## 快速构建

### Linux服务器

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

# 交叉编译（例如在x86上编译ARM）
./build.sh --arch arm64
```

### 使用Makefile

```bash
make                    # 默认构建
make ARCH=arm64         # ARM64架构
make BUILD_TYPE=Debug   # Debug模式
```

### 手动构建

```bash
mkdir build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . -j$(nproc)
```

## 跨架构支持（Linux 64位）

项目完全支持在ARM64和x86_64架构的Linux服务器上编译运行：

- ✅ **x86_64** (64位Intel/AMD)
- ✅ **ARM64** (AArch64, ARM服务器)

**注意**: 仅支持64位架构，不支持32位系统。

支持交叉编译，可以在x86_64服务器上编译ARM64版本，反之亦然。

详细说明请查看 [跨架构编译指南](CROSS_PLATFORM_GUIDE.md)

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

### 邮箱和调度
- 无界邮箱
- 有界邮箱
- 默认调度器
- 同步调度器

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

## 状态

这是一个正在进行中的项目，正在系统地将Go版本转换为C++11。当前已完成：

- [x] 项目结构
- [x] 核心头文件定义
- [ ] 核心实现文件
- [ ] 远程通信模块
- [ ] 集群模块
- [ ] 路由模块
- [ ] 持久化模块
- [ ] 测试和示例

## 许可证

与原项目保持一致。
