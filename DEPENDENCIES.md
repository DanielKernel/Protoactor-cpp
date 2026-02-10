# 依赖项管理

## 推荐使用的开源库

为了加速开发和提高代码质量，项目建议使用以下主流开源库：

### 必需依赖

#### 1. gRPC C++ (远程通信)
- **库**: gRPC C++
- **用途**: 远程通信的gRPC服务器和客户端
- **安装**: 
  ```bash
  sudo apt-get install libgrpc++-dev protobuf-compiler-grpc
  ```
- **文档**: https://grpc.io/docs/languages/cpp/

#### 2. Protobuf C++ (序列化)
- **库**: Protocol Buffers C++
- **用途**: 消息序列化/反序列化
- **安装**:
  ```bash
  sudo apt-get install libprotobuf-dev protobuf-compiler
  ```
- **文档**: https://developers.google.com/protocol-buffers/docs/cpptutorial

### 推荐依赖（可选但建议使用）

#### 3. spdlog (日志)
- **库**: spdlog - 快速C++日志库
- **用途**: 替代手写日志系统
- **优势**: 
  - 高性能
  - 线程安全
  - 多种输出格式
  - 异步日志支持
- **安装**:
  ```bash
  # 从源码编译或使用vcpkg/conan
  git clone https://github.com/gabime/spdlog.git
  cd spdlog && mkdir build && cd build
  cmake .. && make -j$(nproc) && sudo make install
  ```
- **文档**: https://github.com/gabime/spdlog

#### 4. Google Test (测试)
- **库**: Google Test (gtest)
- **用途**: 单元测试和集成测试
- **安装**:
  ```bash
  sudo apt-get install libgtest-dev
  # 或从源码编译
  ```
- **文档**: https://google.github.io/googletest/

#### 5. asio (网络库，可选)
- **库**: Boost.Asio 或 standalone asio
- **用途**: 异步网络操作（如果需要HTTP客户端等）
- **安装**:
  ```bash
  sudo apt-get install libboost-all-dev
  # 或使用standalone asio（header-only）
  ```
- **文档**: https://think-async.com/Asio/

#### 6. rapidjson 或 nlohmann/json (JSON支持，可选)
- **库**: rapidjson 或 nlohmann/json
- **用途**: JSON序列化（如果支持JSON序列化器）
- **安装**:
  ```bash
  # nlohmann/json (header-only)
  sudo apt-get install nlohmann-json3-dev
  
  # rapidjson
  git clone https://github.com/Tencent/rapidjson.git
  ```

#### 7. Consul C++ Client (集群发现，可选)
- **库**: 如果有现成的Consul C++客户端
- **用途**: Consul集群提供者
- **替代**: 可以使用HTTP客户端 + Consul HTTP API

## 依赖管理策略

### 方案1: 系统包管理器（推荐用于生产）
```bash
# Ubuntu/Debian
sudo apt-get install \
    libgrpc++-dev \
    libprotobuf-dev \
    protobuf-compiler \
    protobuf-compiler-grpc \
    libgtest-dev
```

### 方案2: vcpkg（推荐用于开发）
```bash
# 安装vcpkg
git clone https://github.com/Microsoft/vcpkg.git
cd vcpkg
./bootstrap-vcpkg.sh

# 安装依赖
./vcpkg install grpc protobuf spdlog gtest
```

### 方案3: Conan（C++包管理器）
```bash
# 安装Conan
pip install conan

# 创建conanfile.txt
# 安装依赖
conan install .
```

## 当前实现策略

### 使用开源库的部分
1. ✅ **gRPC** - 用于远程通信（待集成）
2. ✅ **Protobuf** - 用于消息序列化（待集成）
3. ⚠️ **日志** - 当前使用占位符，建议使用spdlog
4. ⚠️ **测试** - 待实现，建议使用Google Test

### 保持自实现的部分
1. ✅ **Actor系统核心** - 核心逻辑，需要精确控制
2. ✅ **消息处理** - 核心逻辑
3. ✅ **监督策略** - 核心逻辑
4. ✅ **路由功能** - 相对简单，自实现更灵活

## 集成建议

### 优先级1: gRPC和Protobuf
- 这是远程通信的核心，必须使用
- 已经有成熟的C++实现

### 优先级2: 日志库（spdlog）
- 当前日志是占位符
- spdlog性能好，API简单
- 可以快速替换

### 优先级3: 测试框架（Google Test）
- 需要编写测试代码
- Google Test是C++标准测试框架
- 广泛使用，文档完善

### 优先级4: JSON库（如果需要JSON序列化器）
- 如果支持JSON序列化，可以使用nlohmann/json
- Header-only，易于集成

## 更新CMakeLists.txt以支持可选依赖

项目将支持：
- 必需依赖（gRPC, Protobuf）
- 可选依赖（spdlog, gtest等）
- 如果依赖不存在，提供降级方案或明确错误
