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

详细说明请查看 [跨架构编译指南](CROSS_PLATFORM_GUIDE.md)

## 依赖项

### 必需依赖
- pthread（系统自带）
- C++11标准库（编译器自带）

### 可选依赖（功能增强）
- **gRPC + Protobuf**: 远程通信（必需，如需远程功能）
- **spdlog**: 日志系统（推荐）
- **Google Test**: 测试框架（推荐）
- **nlohmann/json**: JSON支持（可选）

详细依赖说明和安装步骤请查看 [依赖项说明](DEPENDENCIES.md) 和 [安装指南](INSTALL_DEPENDENCIES.md)

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

详细说明请查看 [gRPC集成指南](GRPC_INTEGRATION_GUIDE.md)

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

更多示例请查看 `examples/` 目录

## 文档

- [构建指南](BUILD_GUIDE.md) - 详细的构建说明
- [跨架构编译指南](CROSS_PLATFORM_GUIDE.md) - 交叉编译说明
- [依赖项说明](DEPENDENCIES.md) - 依赖库介绍
- [安装指南](INSTALL_DEPENDENCIES.md) - 依赖安装步骤
- [gRPC集成指南](GRPC_INTEGRATION_GUIDE.md) - gRPC集成说明

## 许可证

与原项目保持一致。
