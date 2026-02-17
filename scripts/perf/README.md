# 性能测试脚本 (Performance Test Scripts)

本目录包含用于运行和分析性能测试的脚本。

## 文件说明

### run_perf_tests.sh
自动化性能测试脚本，执行以下操作：
- 构建 `perf_benchmark` 可执行文件（如果需要）
- 使用不同的参数组合运行多次测试
- 收集 CPU 和内存使用数据
- 生成结果到 `perf_results/` 目录

**使用方法**:
```bash
# 使用默认参数
./scripts/perf/run_perf_tests.sh

# 自定义参数: actors=1,10 messages=100,1000 重复2次
./scripts/perf/run_perf_tests.sh "1,10" "100,1000" 2
```

### aggregate_results.py
结果聚合脚本，执行以下操作：
- 读取 `perf_results/summary.csv`
- 计算每个配置的平均值
- 生成 `perf_results/aggregate_summary.csv`
- 如果安装了 matplotlib/pandas，生成性能图表

**使用方法**:
```bash
python3 scripts/perf/aggregate_results.py
```

### requirements.txt
Python 依赖项（用于绘图功能）：
```bash
pip3 install -r scripts/perf/requirements.txt
```

## 输出目录

所有结果保存在 `perf_results/` 目录中：
- `summary.csv` - 所有运行的汇总数据
- `aggregate_summary.csv` - 聚合的平均值
- `elapsed_vs_actors.png` - 性能图表（如果生成）
- `run_*` - 每次运行的详细日志和统计数据

**重要**: `perf_results/` 目录已添加到 `.gitignore` 中，不会提交到代码仓库。这些是生成的构建产物，每次运行测试时会自动重新生成。

## CI/CD 集成

GitHub Actions 工作流 (`.github/workflows/perf.yml`) 会自动：
1. 运行性能测试
2. 生成聚合报告
3. 将 `perf_results` 上传为构件供下载

详细信息请参阅 [PERF_TESTS.md](../../docs/PERF_TESTS.md)
