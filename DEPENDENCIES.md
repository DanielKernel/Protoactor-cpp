# 依赖项说明

## 核心原则

ProtoActor C++采用**"使用成熟开源库，专注核心逻辑"**的策略：
- ✅ **使用开源库**: 网络、序列化、日志、测试等通用功能
- ✅ **自实现**: Actor系统核心逻辑，需要精确控制的部分

## 依赖分类

### 必需依赖（核心功能）

| 库 | 用途 | 状态 |
|---|---|---|
| **pthread** | 线程支持 | ✅ 系统自带 |
| **C++11标准库** | 基础功能 | ✅ 编译器自带 |

### 可选依赖（功能增强）

| 库 | 用途 | 推荐度 | 安装 |
|---|---|---|---|
| **gRPC** | 远程通信 | ⭐⭐⭐ 必需（远程功能） | `sudo apt-get install libgrpc++-dev protobuf-compiler-grpc` |
| **Protobuf** | 消息序列化 | ⭐⭐⭐ 必需（远程功能） | `sudo apt-get install libprotobuf-dev protobuf-compiler` |
| **spdlog** | 日志系统 | ⭐⭐ 强烈推荐 | 使用vcpkg或从源码编译 |
| **Google Test** | 测试框架 | ⭐⭐ 强烈推荐 | `sudo apt-get install libgtest-dev` |
| **nlohmann/json** | JSON支持 | ⭐ 可选 | `sudo apt-get install nlohmann-json3-dev` |

## 快速安装

### 最小安装（仅核心Actor功能）

```bash
sudo apt-get install build-essential cmake
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

## 依赖管理

项目支持多种依赖管理方式：
- ✅ 系统包管理器（apt/yum）
- ✅ vcpkg（推荐用于开发）
- ✅ Conan
- ✅ 从源码编译

详细安装步骤请查看 `INSTALL_DEPENDENCIES.md`

## 为什么使用这些库？

- **gRPC + Protobuf**: Google维护，生产环境验证，高性能，跨语言兼容
- **spdlog**: 高性能异步日志，简单易用
- **Google Test**: C++标准测试框架，功能完整
- **nlohmann/json**: Header-only，现代C++ API

## 不使用开源库的部分

以下部分保持自实现：
1. **Actor系统核心** - 需要精确控制
2. **消息处理循环** - 核心逻辑
3. **监督策略** - 核心逻辑
4. **路由算法** - 相对简单，自实现更灵活
