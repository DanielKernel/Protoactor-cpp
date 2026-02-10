# 依赖项说明

## 核心原则

ProtoActor C++项目采用**"使用成熟开源库，专注核心逻辑"**的策略：

- ✅ **使用开源库**: 网络、序列化、日志、测试等通用功能
- ✅ **自实现**: Actor系统核心逻辑，需要精确控制的部分

## 依赖分类

### 必需依赖（核心功能）

| 库 | 用途 | 状态 |
|---|---|---|
| **pthread** | 线程支持 | ✅ 系统自带 |
| **C++11标准库** | 基础功能 | ✅ 编译器自带 |

### 可选依赖（功能增强）

| 库 | 用途 | 推荐度 | 安装难度 |
|---|---|---|---|
| **gRPC** | 远程通信 | ⭐⭐⭐ 必需（远程功能） | 中等 |
| **Protobuf** | 消息序列化 | ⭐⭐⭐ 必需（远程功能） | 简单 |
| **spdlog** | 日志系统 | ⭐⭐ 强烈推荐 | 简单 |
| **Google Test** | 测试框架 | ⭐⭐ 强烈推荐 | 简单 |
| **nlohmann/json** | JSON支持 | ⭐ 可选 | 简单 |

## 快速开始

### 最小安装（仅核心Actor功能）

```bash
# 只需要构建工具
sudo apt-get install build-essential cmake

# 构建
cmake .. -DENABLE_PROTOBUF=OFF -DENABLE_GRPC=OFF
make
```

### 完整安装（所有功能）

```bash
# 使用系统包管理器
sudo apt-get install \
    libgrpc++-dev \
    libprotobuf-dev \
    protobuf-compiler \
    protobuf-compiler-grpc \
    libgtest-dev \
    nlohmann-json3-dev

# 或使用vcpkg（推荐）
vcpkg install grpc protobuf spdlog gtest nlohmann-json
```

## 为什么使用这些库？

### gRPC + Protobuf
- **成熟稳定**: Google维护，生产环境验证
- **高性能**: 优化的二进制协议
- **跨语言**: 与Go版本兼容
- **标准**: 广泛使用的RPC框架

### spdlog
- **高性能**: 异步日志，对性能影响小
- **易用**: 简单的API
- **功能丰富**: 多种输出格式和级别

### Google Test
- **标准**: C++事实上的测试标准
- **功能完整**: 断言、夹具、Mock等
- **文档完善**: 丰富的文档和示例

## 不使用开源库的部分

以下部分保持自实现，因为：
1. **核心逻辑**: Actor系统核心需要精确控制
2. **简单实现**: 路由等功能相对简单，自实现更灵活
3. **无合适库**: 某些功能没有现成的C++库

## 依赖管理

项目支持多种依赖管理方式：
- ✅ 系统包管理器（apt/yum）
- ✅ vcpkg（推荐）
- ✅ Conan
- ✅ 从源码编译

详细说明请查看 `INSTALL_DEPENDENCIES.md`
