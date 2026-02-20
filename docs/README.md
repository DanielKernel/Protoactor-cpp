# 项目文档 (docs)

本目录存放项目文档。

## 文档索引

| 文档 | 说明 |
|------|------|
| [ARCHITECTURE.md](ARCHITECTURE.md) | **架构设计**：系统架构、模块设计、设计原则 |
| [BUILD_AND_REMOTE.md](BUILD_AND_REMOTE.md) | **构建与远程**：依赖安装、构建配置、远程通信（当前暂不支持） |
| [COMPARISON_AND_MIGRATION.md](COMPARISON_AND_MIGRATION.md) | **对比与迁移**：与 Go 版本对比、API 差异、迁移指南 |
| [GAP_ANALYSIS_VS_PROTOACTOR_GO.md](GAP_ANALYSIS_VS_PROTOACTOR_GO.md) | **功能差异分析**：与 ProtoActor-Go 的实际差距、暂不支持项说明 |
| [API_REFERENCE.md](API_REFERENCE.md) | **API 参考**：完整的 C++ API 文档 |

## 其他文档

- [测试与性能指南](../tests/TESTING.md)：测试方法、性能测试、覆盖率
- [示例代码](../examples/)：完整的使用示例

## 功能支持状态

| 模块 | 状态 |
|------|--------|
| 核心功能 (Actor/PID/Context/Props) | ✅ 支持 |
| 消息传递 (Send/Request/Future) | ✅ 支持 |
| 生命周期管理 | ✅ 支持 |
| 监督策略 (OneForOne/AllForOne) | ✅ 支持 |
| 路由系统 | ✅ 支持 |
| 持久化（内存实现） | ✅ 支持 |
| 远程通信 (gRPC/Protobuf/跨节点) | ⛔ **暂不支持** |
| 集群 (Gossip/跨节点/GetCluster) | ⛔ **暂不支持** |
| 虚拟 Actor (Grains) | ⛔ **暂不支持** |

本地核心能力与 ProtoActor-Go 对齐；远程/集群/虚拟 Actor 为暂不支持，详见 [功能差异分析](GAP_ANALYSIS_VS_PROTOACTOR_GO.md)。
