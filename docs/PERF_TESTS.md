# 性能测试流水线说明

本文档说明如何在 CI/CD 中运行性能测试，以收集不同 `actors` 与 `messages` 配置下的 CPU 与内存使用数据。

已有内容：

- `examples/perf_benchmark.cpp`：可配置的基准程序，参数：`--actors N --messages M --sleep-ms S`。
. `scripts/perf/run_perf_tests.sh`：批量运行不同参数组合，支持重复运行（`REPEATS`，默认 3）、使用 `pidstat` 优先采样（若可用），并以 CSV 形式采样进程 `CPU%` 与 `RSS`（KB），结果存放 `perf_results`。
 - `scripts/perf/aggregate_results.py`：对 `summary.csv` 聚合并产出 `perf_results/aggregate_summary.csv`，如安装 `pandas`/`matplotlib`，还会输出 `perf_results/elapsed_vs_actors.png`。
- `.github/workflows/perf.yml`：GitHub Actions workflow，自动构建并运行 `run_perf_tests.sh`，并把 `perf_results` 上传为构件。

在本地运行：

```bash
# 可选自定义参数：第一个参数为 actors 列表（逗号分隔），第二个参数为 messages 列表（逗号分隔）
# 示例：在本地只跑 actors=10,100 messages=1000,10000，重复 5 次
scripts/perf/run_perf_tests.sh "10,100" "1000,10000" 5

# 运行完后，生成聚合报告（若安装了依赖）
python3 scripts/perf/aggregate_results.py
```

结果目录：`perf_results/`，其中包含每次运行的 `stdout.log`、`stderr.log`、`stats.csv`（时间序列采样）以及根 `summary.csv`（汇总）。

**注意**：`perf_results/` 目录是生成的构建产物，已添加到 `.gitignore` 中，不会提交到代码仓库。每次运行性能测试时，脚本会自动生成这些结果文件。

在 GitHub Actions 中，运行完成后会把 `perf_results` 上传为名为 `perf-results` 的构件，方便下载与分析。
