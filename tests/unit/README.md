# 单元测试 (tests/unit)

本目录为**单元测试用例**，按模块划分，用于看护各模块基本功能，不依赖完整 Actor 运行时或外部服务。

- 每个文件对应一个可执行测试（如 `unit_pid`、`thread_pool_test`）。
- 使用 `tests/test_common.h` 中的断言与 `run_test`。
- 通过 CTest 标签 `unit` 与 `module:<name>` 可按需运行，参见 [tests/README.md](../README.md) 与 [docs/TESTING.md](../../docs/TESTING.md)。

## 测试文件

| 文件 | 模块 | 测试数 |
|------|------|--------|
| `config_test.cpp` | 配置 | 3 |
| `dispatcher_test.cpp` | 调度器 | 4 |
| `eventstream_test.cpp` | 事件流 | 5 |
| `extensions_test.cpp` | 扩展 | 3 |
| `messages_test.cpp` | 消息 | 6 |
| `middleware_test.cpp` | 中间件 | 15 |
| `persistence_test.cpp` | 持久化 | 16 |
| `pid_test.cpp` | PID | 5 |
| `pidset_test.cpp` | PID集合 | 4 |
| `platform_test.cpp` | 平台 | 2 |
| `priority_queue_test.cpp` | 优先队列 | 4 |
| `props_test.cpp` | Props | 3 |
| `queue_test.cpp` | 队列 | 5 |
| `router_test.cpp` | 路由 | 18 |
| `thread_pool_test.cpp` | 线程池 | 8 |
| `cluster_test.cpp` | 集群 | 14 |
| `remote_test.cpp` | 远程 | 17 |

**与功能测试区分**：功能/集成测试、性能测试位于 [tests/functional/](../functional/)。
