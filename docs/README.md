# 项目文档 (docs)

本目录存放项目文档。

## 文档索引

| 文档 | 说明 |
|------|------|
| [ARCHITECTURE.md](ARCHITECTURE.md) | **架构设计**：系统架构、模块设计、设计原则 |
| [BUILD_AND_REMOTE.md](BUILD_AND_REMOTE.md) | **构建与远程**：依赖安装、构建配置、远程通信 |
| [COMPARISON_AND_MIGRATION.md](COMPARISON_AND_MIGRATION.md) | **对比与迁移**：与 Go 版本对比、API差异、迁移指南 |
| [API_REFERENCE.md](API_REFERENCE.md) | **API参考**：完整的 C++ API 文档 |

## 其他文档

- [测试与性能指南](../tests/TESTING.md)：测试方法、性能测试、覆盖率
- [示例代码](../examples/)：完整的使用示例

## 功能完成度

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

**总体完成度: 100%**
