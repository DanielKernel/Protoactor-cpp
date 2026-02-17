# 项目文档 (docs)

本目录存放构建与测试相关文档，**不归档**覆盖率等测试结果文件（如 `.gcov`），参见项目根目录 `.gitignore`。

## 文档索引

| 文档 | 说明 |
|------|------|
| [BUILD_GUIDE.md](BUILD_GUIDE.md) | 构建指南：依赖、编译、跨架构、选项、验证、常见问题 |
| [TESTING.md](TESTING.md) | 测试指导：脚本用法、单元/功能测试、覆盖率、常见问题 |
| [PERF_TESTS.md](PERF_TESTS.md) | 性能测试：基准测试说明、运行方法 |
| [COMPARISON.md](COMPARISON.md) | **功能对比**：与 ProtoActor-Go 的详细功能对比分析 |
| [REMOTE_GUIDE.md](REMOTE_GUIDE.md) | **远程通信指南**：gRPC配置、消息序列化、最佳实践 |
| [MIGRATION_GUIDE.md](MIGRATION_GUIDE.md) | **迁移指南**：从 ProtoActor-Go 迁移到 C++ 版本 |

## 功能完成度概览

基于 [COMPARISON.md](COMPARISON.md) 的详细分析：

| 模块 | 完成度 |
|------|--------|
| 核心功能 (Actor/PID/Context/Props) | 100% |
| 消息传递 (Send/Request/Future) | 100% |
| 生命周期管理 | 100% |
| 监督策略 (OneForOne/AllForOne) | 100% |
| 远程通信 (gRPC/Protobuf) | 100% |
| 集群支持 (Gossip/PubSub) | 100% |
| 路由系统 | 100% |
| 持久化 | 100% |

**总体完成度: 95%+** - 核心功能与 ProtoActor-Go 完全对齐
