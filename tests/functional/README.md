# 功能测试 (tests/functional)

本目录为**功能测试用例**，包括集成测试与性能测试，依赖完整 Actor 运行时或较多资源。

| 用例 | 类型 | 说明 |
|------|------|------|
| **actor_integration_test** | 集成测试 | ActorSystem、Spawn、Send、多 Actor；部分环境可能异常（如 Bus error） |
| **performance_test** | 性能测试 | 线程池/Dispatcher/Actor 吞吐基准，建议 Release 构建 |

运行方式：

- 全部功能测试：`ctest -R "actor_integration_test|performance_test"` 或运行对应可执行文件。
- 单元测试与功能测试分开：单元测试用 `ctest -L unit`，不包含本目录。

**与单元测试区分**：单模块、无完整运行时的测试位于 [tests/unit/](../unit/)。
