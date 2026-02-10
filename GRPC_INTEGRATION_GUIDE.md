# gRPC集成指南

## 概述

本文档说明如何完成ProtoActor C++的gRPC集成，实现完整的远程通信功能。

## 当前状态

✅ **已完成**:
- Protobuf定义文件 (`proto/remote.proto`)
- 基础接口定义（Serializer, EndpointManager, EndpointWriter, EndpointReader）
- RemoteProcess实现
- SerializerRegistry实现

⚠️ **待完成**:
- gRPC服务器集成
- gRPC客户端集成
- Protobuf代码生成
- EndpointManager完整实现
- EndpointWriter完整实现
- EndpointReader完整实现
- Protobuf序列化器实现

## 依赖项

### 必需依赖

1. **Protobuf C++**
   ```bash
   # Ubuntu/Debian
   sudo apt-get install libprotobuf-dev protobuf-compiler
   
   # 或从源码编译
   git clone https://github.com/protocolbuffers/protobuf.git
   cd protobuf
   git submodule update --init --recursive
   mkdir build && cd build
   cmake .. -DCMAKE_BUILD_TYPE=Release
   make -j$(nproc)
   sudo make install
   ```

2. **gRPC C++**
   ```bash
   # Ubuntu/Debian
   sudo apt-get install libgrpc++-dev protobuf-compiler-grpc
   
   # 或从源码编译
   git clone --recurse-submodules https://github.com/grpc/grpc.git
   cd grpc
   mkdir -p cmake/build
   cd cmake/build
   cmake ../.. -DCMAKE_BUILD_TYPE=Release
   make -j$(nproc)
   sudo make install
   ```

## 实现步骤

### 1. 生成Protobuf代码

创建 `scripts/generate_proto.sh`:

```bash
#!/bin/bash
set -e

PROTO_DIR="proto"
OUT_DIR="generated"

mkdir -p ${OUT_DIR}

# Generate C++ code from protobuf
protoc --cpp_out=${OUT_DIR} \
       --grpc_out=${OUT_DIR} \
       --plugin=protoc-gen-grpc=`which grpc_cpp_plugin` \
       -I${PROTO_DIR} \
       ${PROTO_DIR}/actor.proto \
       ${PROTO_DIR}/remote.proto

echo "Protobuf code generated in ${OUT_DIR}/"
```

运行脚本生成代码:
```bash
chmod +x scripts/generate_proto.sh
./scripts/generate_proto.sh
```

### 2. 更新CMakeLists.txt

添加Protobuf和gRPC支持:

```cmake
# Find Protobuf
find_package(Protobuf REQUIRED)
find_package(gRPC REQUIRED)

# Generate protobuf files
set(PROTO_FILES
    proto/actor.proto
    proto/remote.proto
)

protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS ${PROTO_FILES})

# Add generated files to library
target_sources(protoactor-cpp PRIVATE ${PROTO_SRCS})
target_include_directories(protoactor-cpp PUBLIC ${CMAKE_CURRENT_BINARY_DIR})

# Link libraries
target_link_libraries(protoactor-cpp
    ${Protobuf_LIBRARIES}
    gRPC::grpc++
    gRPC::grpc++_reflection
)
```

### 3. 实现Protobuf序列化器

创建 `src/remote/proto_serializer.cpp`:

```cpp
#include "protoactor/remote/serializer.h"
#include "generated/actor.pb.h"
#include "generated/remote.pb.h"
#include <google/protobuf/any.pb.h>
#include <google/protobuf/message.h>

class ProtoSerializer : public protoactor::remote::Serializer {
public:
    std::vector<uint8_t> Serialize(const std::any& message) override {
        // 将message转换为protobuf Message并序列化
        // 需要类型注册机制
    }
    
    std::any Deserialize(const std::string& type_name, const std::vector<uint8_t>& bytes) override {
        // 根据type_name反序列化
    }
    
    std::string GetTypeName(const std::any& message) override {
        // 获取消息类型名
    }
    
    int32_t GetSerializerID() const override { return 0; }
};
```

### 4. 实现gRPC服务器

在 `src/remote/remote.cpp` 中添加:

```cpp
#include <grpcpp/grpcpp.h>
#include "generated/remote.grpc.pb.h"

class RemotingServiceImpl : public protoactor::remote::Remoting::Service {
    grpc::Status Receive(
        grpc::ServerContext* context,
        grpc::ServerReaderWriter<RemoteMessage, RemoteMessage>* stream) override {
        
        // 使用EndpointReader处理流
        auto reader = std::make_shared<EndpointReader>(remote_);
        return reader->HandleStream(stream);
    }
    
    // 实现其他RPC方法...
};

void Remote::Start() {
    // 创建gRPC服务器
    grpc::ServerBuilder builder;
    builder.AddListeningPort(config_->Address(), grpc::InsecureServerCredentials());
    
    auto service = std::make_shared<RemotingServiceImpl>(this);
    builder.RegisterService(service.get());
    
    server_ = builder.BuildAndStart();
    
    // 启动EndpointManager
    endpoint_manager_->Start();
}
```

### 5. 实现gRPC客户端

在 `src/remote/endpoint_writer.cpp` 中:

```cpp
#include <grpcpp/grpcpp.h>
#include "generated/remote.grpc.pb.h"

bool EndpointWriter::InitializeInternal() {
    // 创建gRPC客户端连接
    auto channel = grpc::CreateChannel(address_, grpc::InsecureChannelCredentials());
    auto stub = Remoting::NewStub(channel);
    
    // 创建双向流
    grpc::ClientContext context;
    stream_ = stub->Receive(&context);
    
    // 发送连接请求
    RemoteMessage connect_msg;
    // ... 设置连接请求
    stream_->Write(connect_msg);
    
    // 接收连接响应
    RemoteMessage response;
    stream_->Read(&response);
    
    // 启动接收线程
    receive_thread_ = std::thread([this]() {
        RemoteMessage msg;
        while (stream_->Read(&msg)) {
            // 处理接收到的消息
        }
    });
    
    return true;
}
```

## 测试

创建测试文件 `tests/remote_test.cpp`:

```cpp
#include <gtest/gtest.h>
#include "protoactor/remote/remote.h"

TEST(RemoteTest, BasicConnection) {
    auto system = protoactor::ActorSystem::New();
    auto remote = protoactor::remote::Remote::Start(system, "localhost", 8080);
    
    // 测试连接
    // ...
}
```

## 注意事项

1. **线程安全**: gRPC操作是线程安全的，但需要确保消息处理的线程安全
2. **错误处理**: 需要处理连接失败、超时等错误情况
3. **资源管理**: 确保正确关闭gRPC连接和流
4. **性能优化**: 使用消息批处理提高性能

## 参考资源

- [gRPC C++文档](https://grpc.io/docs/languages/cpp/)
- [Protobuf C++文档](https://developers.google.com/protocol-buffers/docs/cpptutorial)
- Go版本的实现: `remote/` 目录

## 完成检查清单

- [ ] Protobuf代码生成
- [ ] CMake配置更新
- [ ] Protobuf序列化器实现
- [ ] gRPC服务器实现
- [ ] gRPC客户端实现
- [ ] EndpointManager完整实现
- [ ] EndpointWriter完整实现
- [ ] EndpointReader完整实现
- [ ] 单元测试
- [ ] 集成测试
