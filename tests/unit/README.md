# 单元测试 (tests/unit)

本目录为**单元测试用例**，按模块划分，用于看护各模块基本功能，不依赖完整 Actor 运行时或外部服务。

- 每个文件对应一个可执行测试（如 `unit_pid`、`thread_pool_test`）。
- 使用 `tests/test_common.h` 中的断言与 `run_test`。
- 通过 CTest 标签 `unit` 与 `module:<name>` 可按需运行，参见 [tests/README.md](../README.md) 与 [docs/TESTING.md](../../docs/TESTING.md)。

**与功能测试区分**：功能/集成测试、性能测试位于 [tests/functional/](../functional/)。
