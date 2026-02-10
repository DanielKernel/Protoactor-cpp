# 构建指南

## 系统要求

- **操作系统**: Linux (Ubuntu, Debian, CentOS, RHEL, Fedora等)
- **架构**: x86_64 或 ARM64 (仅支持64位)
- **编译器**: GCC 4.8+ 或 Clang 3.3+
- **CMake**: 3.10+
- **依赖库**: pthread (系统自带)

## 快速构建

### 方法1: 使用构建脚本（推荐）

```bash
# 构建当前架构
./build.sh

# 构建指定架构
./build.sh --arch x86_64    # Intel/AMD 64位
./build.sh --arch arm64     # ARM 64位

# Debug模式
./build.sh --debug

# 启用Protobuf支持
./build.sh --protobuf

# 清理并重新构建
./build.sh --clean
```

### 方法2: 使用CMake直接构建

```bash
mkdir build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
```

## 构建选项

| 选项 | 说明 | 默认值 |
|------|------|--------|
| `CMAKE_BUILD_TYPE` | Release/Debug | Release |
| `BUILD_EXAMPLES` | 构建示例 | ON |
| `BUILD_TESTS` | 构建测试 | OFF |
| `ENABLE_PROTOBUF` | 启用Protobuf | OFF |
| `ENABLE_GRPC` | 启用gRPC | OFF |
| `BUILD_SHARED_LIBS` | 构建共享库 | OFF |

## 验证构建

```bash
# 检查二进制架构
file build-*/bin/hello_world

# 运行示例
./build-x86_64-Release/bin/hello_world
./build-x86_64-Release/bin/supervision_example
```

## 常见问题

**Q: 编译时出现"找不到编译器"错误？**  
A: 确保已安装GCC或Clang：`sudo apt-get install build-essential`

**Q: 如何指定编译器？**  
A: `cmake .. -DCMAKE_C_COMPILER=gcc -DCMAKE_CXX_COMPILER=g++`

**Q: 如何启用更多优化？**  
A: `cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_FLAGS="-O3 -march=native"`

## 生产环境部署

### 最小化依赖

项目只依赖系统标准库，可以静态链接：

```bash
cmake .. -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_EXE_LINKER_FLAGS="-static-libgcc -static-libstdc++"
```

### Docker部署

```bash
docker build --build-arg TARGET_ARCH=x86_64 -t protoactor-cpp:latest .
docker run protoactor-cpp:latest
```

详细交叉编译说明请查看 `CROSS_PLATFORM_GUIDE.md`
