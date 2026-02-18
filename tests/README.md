# ProtoActor C++ 自动化测试

**测试指导与脚本说明详见 [docs/TESTING.md](../docs/TESTING.md)。**

## 目录与用例区分

| 目录 | 用途 | 说明 |
|------|------|------|
| **tests/unit/** | 单元测试用例 | 按模块的单测，不依赖完整 Actor 运行时；见 [tests/unit/README.md](unit/README.md) |
| **tests/functional/** | 功能测试用例 | 集成测试、性能测试；见 [tests/functional/README.md](functional/README.md) |
| **tests/integration/** | 集成测试用例 | 远程/集群模块的集成测试 |
| **tests/scripts/** | 测试脚本 | 单元测试、CI测试、覆盖率、性能测试脚本 |
| **tests/test_common.h** | 测试公共头文件 | 断言与 `run_test`，供 unit/functional 共用 |

`.gcov` 等覆盖率结果文件已加入 `.gitignore`，**不归档**到版本库。

## 脚本速览

| 脚本 | 用途 |
|------|------|
| `tests/scripts/run_unit_tests.sh` | 单元测试自动化：配置/构建/运行，支持 `--module`、`--quick`、`--coverage` |
| `tests/scripts/ci_tests.sh` | CI 用：配置+构建+跑单元测试，以退出码表示成败 |
| `tests/scripts/coverage_report.sh` | 在开启 ENABLE_COVERAGE 并跑完单元测试后，汇总关键代码行覆盖率 |
| `tests/scripts/perf/run_perf_tests.sh` | 性能测试批量运行脚本 |

示例（在项目根目录执行）：
```bash
./tests/scripts/run_unit_tests.sh --configure   # 首次配置并跑单元测试
./tests/scripts/run_unit_tests.sh --module queue
./tests/scripts/run_unit_tests.sh --quick
./tests/scripts/ci_tests.sh
```

## 测试分类

### 单元测试（按模块，看护基本功能）

各模块对应独立可执行用例，并打上 CTest 标签，便于**按需只跑某模块**：

| 模块 | 可执行名 | 标签 | 覆盖要点 |
|------|----------|------|----------|
| **pid** | unit_pid | `module:pid` | NewPID、Equal、address/id |
| **config** | unit_config | `module:config` | Config::Default()、默认字段 |
| **platform** | unit_platform | `module:platform` | GetCPUCount、MemoryBarrier、CPUPause |
| **queue** | unit_queue | `module:queue` | 无界队列 / MPSC Push、Pop、Empty、Size |
| **pidset** | unit_pidset | `module:pidset` | Add、Remove、Contains、GetAll、Clear |
| **priority_queue** | unit_priority_queue | `module:priority_queue` | Push、Pop、Empty、Size、Clear |
| **messages** | unit_messages | `module:messages` | 生命周期、MessageEnvelope、GetHeader/SetHeader、WrapEnvelope、UnwrapEnvelope |
| **thread_pool** | thread_pool_test | `module:thread_pool` | Submit、Shutdown、异常隔离、默认池 |
| **dispatcher** | dispatcher_test | `module:dispatcher` | 默认/同步 Dispatcher、Throughput |
| **extensions** | unit_extensions | `module:extensions` | New、Register、Get、NextExtensionID |
| **props** | unit_props | `module:props` | FromProducer、WithDispatcher、GetDispatcher |
| **eventstream** | unit_eventstream | `module:eventstream` | New、Subscribe、Publish、Unsubscribe、Length |

### 集成与性能测试

- **actor_integration_test**（标签 `integration`、`module:actor`）— ActorSystem、Spawn、Send。
- **remote_cluster_integration_test**（标签 `integration`、`module:remote`、`module:cluster`）— 远程通信与集群集成测试，验证远程Actor、成员管理、故障转移、Pub/Sub等场景。
- **performance_test**（标签 `performance`）— 线程池/Dispatcher/Actor 吞吐基准（建议 Release 构建）。

## 按需运行不同模块

```bash
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTS=ON
make -j4

# 只跑单元测试（不含 integration/performance）
ctest -L unit --output-on-failure

# 按模块运行：用测试名精确匹配（推荐）
ctest -R unit_pid --output-on-failure
ctest -R unit_queue --output-on-failure
ctest -R thread_pool_test --output-on-failure

# 按 CTest 标签运行（标签为正则，module:pid 会匹配 module:pidset，需精确时用 -R）
ctest -L "module:queue" --output-on-failure
ctest -L "module:thread_pool" --output-on-failure

# 运行全部测试
ctest --output-on-failure -V

# 仅跑核心工程测试（适合 CI，不依赖完整 Actor 运行时）
ctest -R "thread_pool_test|dispatcher_test" --output-on-failure -V
```

## 环境说明

- 测试通过 CTest 运行时自动设置 **PROTOACTOR_TEST=1**，避免默认线程池 atexit 与进程退出顺序冲突。
- **actor_integration_test**、**performance_test** 依赖完整 Actor 运行时；若在部分环境出现 Bus error，可仅跑单元测试：`ctest -L unit`。

## 关键代码覆盖率（目标 ≥60%）

对**关键代码**（pid、config、platform、queue、pidset、priority_queue、messages、thread_pool、dispatcher、extensions、props、eventstream 对应源文件）通过单元测试做行覆盖率统计，目标 **≥60%**。

- **测量方式**：使用 gcov，开启 `ENABLE_COVERAGE` 构建并跑单元测试后收集。
  ```bash
  mkdir build_cov && cd build_cov
  cmake .. -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTS=ON -DENABLE_COVERAGE=ON
  make -j4
  ctest -L unit
  # 然后可用 gcov 或 tests/scripts/coverage_report.sh 查看各文件覆盖率
  ./tests/scripts/coverage_report.sh build_cov
  ```
- **当前情况**：config、queue、dispatcher、platform、pidset、priority_queue、thread_pool、extensions、props、eventstream 等单元测试可覆盖大部分逻辑，行覆盖率可达 60% 以上。**pid** 中 `Ref`/`SendUserMessage`/`SendSystemMessage`/`ClearCache` 依赖 ActorSystem，仅单元测试时覆盖率较低，需依赖集成测试或后续补充带 ActorSystem 的用例才能提高。**messages** 已通过 MessageEnvelope、GetHeader/SetHeader、WrapEnvelope/UnwrapEnvelope 等用例提升覆盖率。
- 若需**仅单元测试**即达到 60% 目标，可优先保证上述已覆盖模块的用例稳定，并视情况为 pid 增加带轻量 ActorSystem 的用例（若运行环境支持）。

## 不依赖 GTest

测试使用 `tests/test_common.h` 的轻量断言与 `run_test`，无需安装 Google Test。
