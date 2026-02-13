# ProtoActor C++ 测试指导说明

本文档说明如何运行自动化测试、使用脚本以及常见场景下的推荐做法。

---

## 一、快速开始

### 1. 使用脚本（推荐）

在**项目根目录**下执行：

```bash
# 首次：配置 + 构建 + 运行全部单元测试
./scripts/run_unit_tests.sh --configure

# 之后仅运行单元测试（需已构建）
./scripts/run_unit_tests.sh

# 只跑核心测试（线程池 + 调度器，适合快速验证）
./scripts/run_unit_tests.sh --quick

# 只跑某一模块
./scripts/run_unit_tests.sh --module thread_pool
./scripts/run_unit_tests.sh --module queue
```

### 2. CI 流水线

使用 `ci_tests.sh`：自动配置、构建并运行全部单元测试，通过时退出码 0，失败时非 0。

```bash
./scripts/ci_tests.sh           # 使用默认 build 目录
./scripts/ci_tests.sh build_ci  # 指定构建目录
```

---

## 二、脚本说明

### run_unit_tests.sh（单元测试自动化）

| 选项 | 说明 |
|------|------|
| `--build-dir DIR` | 构建目录，默认 `build` |
| `--configure` | 若未配置则先执行 cmake |
| `--coverage` | 使用 `build_cov` 并开启 ENABLE_COVERAGE，构建后跑单元测试 |
| `--module NAME` | 只运行指定模块（pid, config, platform, queue, pidset, priority_queue, messages, thread_pool, dispatcher） |
| `--quick` | 只运行 thread_pool_test、dispatcher_test |
| `--list` | 列出所有单元测试名称与模块名后退出 |
| `--verbose` | ctest -V 详细输出 |
| `-h, --help` | 打印帮助 |

**示例：**

```bash
# 带覆盖率构建并跑单元测试，然后可配合 coverage_report.sh 查看覆盖率
./scripts/run_unit_tests.sh --coverage
./scripts/coverage_report.sh build_cov

# 指定构建目录并先配置
./scripts/run_unit_tests.sh --build-dir build_debug --configure

# 只跑 messages 模块并输出详细日志
./scripts/run_unit_tests.sh --module messages --verbose
```

### ci_tests.sh（CI 专用）

- 自动创建构建目录、cmake 配置（Release、BUILD_TESTS=ON、BUILD_EXAMPLES=OFF）、编译、运行 `ctest -L unit`。
- 不跑集成/性能测试，不开启覆盖率。
- 通过：退出码 0；失败：非 0，便于 CI 判断。

### coverage_report.sh（覆盖率汇总）

- 在**已用 ENABLE_COVERAGE 构建并执行过单元测试**的前提下，汇总关键源文件的行覆盖率。
- 用法：`./scripts/coverage_report.sh [build_dir]`，默认 `build_dir=build_cov`。

---

## 三、手动运行（不通过脚本）

### 配置与构建

```bash
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTS=ON
make -j4
```

### 运行测试

```bash
cd build
export PROTOACTOR_TEST=1   # 建议设置，避免默认线程池 atexit 问题

# 全部单元测试
ctest -L unit --output-on-failure

# 按测试名运行单个
ctest -R unit_pid --output-on-failure
ctest -R thread_pool_test --output-on-failure

# 按标签运行（标签为正则）
ctest -L "module:queue" --output-on-failure

# 详细输出
ctest -L unit -V
```

---

## 四、测试分类与适用场景

| 类型 | 标签/方式 | 适用场景 |
|------|-----------|----------|
| 单元测试 | `ctest -L unit` 或 `run_unit_tests.sh` | 日常开发、MR 前、CI |
| 核心测试 | `thread_pool_test`、`dispatcher_test` 或 `--quick` | 快速冒烟、环境受限时 |
| 按模块 | `--module NAME` 或 `ctest -R <name>` | 只改某模块时 |
| 集成测试 | `actor_integration_test` | 需验证 Actor 运行时（部分环境可能异常） |
| 性能测试 | `performance_test` | 吞吐基准，建议 Release 构建 |
| 覆盖率 | `--coverage` + `coverage_report.sh` | 看护关键代码 ≥60% 行覆盖 |

---

## 五、覆盖率（关键代码 ≥60%）

1. **生成覆盖率数据**

   ```bash
   ./scripts/run_unit_tests.sh --coverage
   ```

2. **查看汇总**

   ```bash
   ./scripts/coverage_report.sh build_cov
   ```

3. **说明**  
   关键代码指：pid、config、platform、queue、pidset、priority_queue、messages、thread_pool、dispatcher 对应源文件。目标为单元测试行覆盖率 ≥60%。pid 中依赖 ActorSystem 的路径需集成测试补充。

---

## 六、常见问题

**Q: 未设置 PROTOACTOR_TEST=1 会怎样？**  
A: 默认线程池会注册 atexit，进程退出时可能与静态析构顺序冲突。通过 CTest 或脚本运行时会自动设置该环境变量。

**Q: actor_integration_test / performance_test 崩溃（如 Bus error）？**  
A: 部分环境（如某些 macOS/沙箱）下 Actor 运行时可能异常。可仅跑单元测试：`ctest -L unit` 或 `./scripts/run_unit_tests.sh`。

**Q: 如何只跑某几个模块？**  
A: 用测试名正则，例如：`ctest -R "unit_pid|unit_queue|thread_pool_test" --output-on-failure`，或多次执行 `./scripts/run_unit_tests.sh --module <name>`。

**Q: 脚本没有执行权限？**  
A: `chmod +x scripts/run_unit_tests.sh scripts/ci_tests.sh scripts/coverage_report.sh`

---

## 七、相关文件

- **tests/README.md**：测试分类、单元/功能目录区分、模块表、覆盖率目标说明。
- **tests/unit/**：单元测试用例；**tests/functional/**：功能/集成/性能测试用例。
- **examples/README.md**：使用样例说明（与测试用例区分）。
- **scripts/run_unit_tests.sh**：单元测试自动化入口。
- **scripts/ci_tests.sh**：CI 专用脚本。
- **scripts/coverage_report.sh**：关键代码覆盖率汇总。
- **docs/BUILD_GUIDE.md**：构建与依赖文档。
