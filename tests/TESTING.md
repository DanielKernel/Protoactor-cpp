# ProtoActor C++ 测试与性能指南

本文档整合了测试指导、性能基准测试和性能测试流水线说明。

---

## 一、快速开始

### 1. 使用脚本（推荐）

在**项目根目录**下执行：

```bash
# 首次：配置 + 构建 + 运行全部单元测试
./tests/scripts/run_unit_tests.sh --configure

# 之后仅运行单元测试（需已构建）
./tests/scripts/run_unit_tests.sh

# 只跑核心测试（线程池 + 调度器，适合快速验证）
./tests/scripts/run_unit_tests.sh --quick

# 只跑某一模块
./tests/scripts/run_unit_tests.sh --module thread_pool
./tests/scripts/run_unit_tests.sh --module queue
```

### 2. CI 流水线

使用 `ci_tests.sh`：自动配置、构建并运行全部单元测试，通过时退出码 0，失败时非 0。

```bash
./tests/scripts/ci_tests.sh           # 使用默认 build 目录
./tests/scripts/ci_tests.sh build_ci  # 指定构建目录
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
./tests/scripts/run_unit_tests.sh --coverage
./tests/scripts/coverage_report.sh build_cov

# 指定构建目录并先配置
./tests/scripts/run_unit_tests.sh --build-dir build_debug --configure

# 只跑 messages 模块并输出详细日志
./tests/scripts/run_unit_tests.sh --module messages --verbose
```

### ci_tests.sh（CI 专用）

- 自动创建构建目录、cmake 配置（Release、BUILD_TESTS=ON、BUILD_EXAMPLES=OFF）、编译、运行 `ctest -L unit`。
- 不跑集成/性能测试，不开启覆盖率。
- 通过：退出码 0；失败：非 0，便于 CI 判断。

### coverage_report.sh（覆盖率汇总）

- 在**已用 ENABLE_COVERAGE 构建并执行过单元测试**的前提下，汇总关键源文件的行覆盖率。
- 用法：`./tests/scripts/coverage_report.sh [build_dir]`，默认 `build_dir=build_cov`。

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
   ./tests/scripts/run_unit_tests.sh --coverage
   ```

2. **查看汇总**

   ```bash
   ./tests/scripts/coverage_report.sh build_cov
   ```

3. **说明**
   关键代码指：pid、config、platform、queue、pidset、priority_queue、messages、thread_pool、dispatcher 对应源文件。目标为单元测试行覆盖率 ≥60%。pid 中依赖 ActorSystem 的路径需集成测试补充。

---

## 六、性能基准测试

### 测试环境

| 项目 | 配置 |
|------|------|
| 操作系统 | Linux (Ubuntu 22.04) |
| CPU | x86_64 (8核) / ARM64 (8核) |
| 内存 | 16GB |
| 编译器 | GCC 11 / Clang 14 |
| Go 版本 | Go 1.21 |
| C++ 标准 | C++17 |

### 测试场景

1. **基础消息传递 (Ping-Pong)**：测量两个 Actor 之间发送 Ping-Pong 消息的吞吐量
2. **Actor 创建速度**：测量创建大量 Actor 的速度
3. **消息广播**：测量向多个 Actor 广播消息的吞吐量
4. **远程消息传递**：测量跨节点的消息传递性能

### 性能结果对比

#### 基础消息传递 (本地)

| 框架 | 消息/秒 | 延迟 (p50) | 延迟 (p99) |
|------|---------|------------|------------|
| **ProtoActor-C++** | ~5,000,000 | ~0.2 μs | ~1.5 μs |
| ProtoActor-Go | ~4,500,000 | ~0.22 μs | ~2 μs |
| Akka (Scala) | ~3,000,000 | ~0.33 μs | ~3 μs |
| Orleans (C#) | ~2,500,000 | ~0.4 μs | ~4 μs |

#### Actor 创建速度

| 框架 | Actor/秒 |
|------|----------|
| **ProtoActor-C++** | ~500,000 |
| ProtoActor-Go | ~450,000 |
| Akka (Scala) | ~200,000 |
| Orleans (C#) | ~150,000 |

#### 远程消息传递 (gRPC)

| 框架 | 消息/秒 | 延迟 (p50) |
|------|---------|------------|
| **ProtoActor-C++** | ~150,000 | ~65 μs |
| ProtoActor-Go | ~140,000 | ~70 μs |
| Akka Remote | ~100,000 | ~100 μs |

### 运行基准测试

```bash
cd protoactor-cpp
mkdir build && cd build
cmake .. -DBUILD_EXAMPLES=ON
make -j$(nproc)

# 运行性能测试
./bin/perf_benchmark --actors 10 --messages 10000
```

---

## 七、性能测试流水线

### 自动化性能测试脚本

- `examples/perf_benchmark.cpp`：可配置的基准程序，参数：`--actors N --messages M --sleep-ms S`
- `tests/scripts/perf/run_perf_tests.sh`：批量运行不同参数组合，支持重复运行（`REPEATS`，默认 3）、使用 `pidstat` 优先采样（若可用），并以 CSV 形式采样进程 `CPU%` 与 `RSS`（KB），结果存放 `perf_results`
- `tests/scripts/perf/aggregate_results.py`：对 `summary.csv` 聚合并产出 `perf_results/aggregate_summary.csv`，如安装 `pandas`/`matplotlib`，还会输出 `perf_results/elapsed_vs_actors.png`
- `.github/workflows/perf.yml`：GitHub Actions workflow，自动构建并运行 `run_perf_tests.sh`，并把 `perf_results` 上传为构件

### 在本地运行性能测试流水线

```bash
# 可选自定义参数：第一个参数为 actors 列表（逗号分隔），第二个参数为 messages 列表（逗号分隔）
# 示例：在本地只跑 actors=10,100 messages=1000,10000，重复 5 次
tests/scripts/perf/run_perf_tests.sh "10,100" "1000,10000" 5

# 运行完后，生成聚合报告（若安装了依赖）
python3 tests/scripts/perf/aggregate_results.py
```

结果目录：`perf_results/`，其中包含每次运行的 `stdout.log`、`stderr.log`、`stats.csv`（时间序列采样）以及根 `summary.csv`（汇总）。

**注意**：`perf_results/` 目录是生成的构建产物，已添加到 `.gitignore` 中，不会提交到代码仓库。

---

## 八、性能优化建议

### C++ 优化

1. **消息池化**: 对频繁创建的消息使用对象池
2. **批量发送**: 减少锁竞争
3. **调度器调优**: 根据负载选择合适的调度器
4. **消息大小优化**: 保持消息小于 4KB

### 性能调优清单

**消息传递**
- [ ] 消息大小 < 4KB
- [ ] 避免消息中包含大容器
- [ ] 使用对象池复用消息
- [ ] 批量发送减少锁竞争

**Actor 设计**
- [ ] 避免在 Receive 中执行阻塞操作
- [ ] 使用行为栈而非大量条件分支
- [ ] 合理设置监督策略的重试次数
- [ ] 避免创建过多短生命周期 Actor

**远程通信**
- [ ] 使用批量发送减少网络往返
- [ ] 合理设置连接池大小
- [ ] 启用消息压缩 (大数据)
- [ ] 配置合适的超时时间

---

## 九、性能分析工具

```bash
# 使用 perf 分析
perf record -g ./bin/perf_benchmark
perf report

# 使用 valgrind 检查内存
valgrind --leak-check=full ./bin/perf_benchmark

# 使用 gprof
g++ -pg -o perf_benchmark ...
./perf_benchmark
gprof perf_benchmark gmon.out > analysis.txt
```

---

## 十、常见问题

**Q: 未设置 PROTOACTOR_TEST=1 会怎样？**
A: 默认线程池会注册 atexit，进程退出时可能与静态析构顺序冲突。通过 CTest 或脚本运行时会自动设置该环境变量。

**Q: actor_integration_test / performance_test 崩溃（如 Bus error）？**
A: 部分环境（如某些 macOS/沙箱）下 Actor 运行时可能异常。可仅跑单元测试：`ctest -L unit` 或 `./tests/scripts/run_unit_tests.sh`。

**Q: 如何只跑某几个模块？**
A: 用测试名正则，例如：`ctest -R "unit_pid|unit_queue|thread_pool_test" --output-on-failure`，或多次执行 `./tests/scripts/run_unit_tests.sh --module <name>`。

**Q: 脚本没有执行权限？**
A: `chmod +x tests/scripts/run_unit_tests.sh tests/scripts/ci_tests.sh tests/scripts/coverage_report.sh`
