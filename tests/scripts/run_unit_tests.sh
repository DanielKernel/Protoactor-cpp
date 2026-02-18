#!/usr/bin/env bash
#
# 单元测试自动化脚本：配置、构建、运行单元测试，可选按模块或带覆盖率。
# 在项目根目录执行：./tests/scripts/run_unit_tests.sh [选项]
#
# 选项：
#   --build-dir DIR    构建目录，默认 build
#   --configure         若构建目录不存在或为空，先执行 cmake 配置
#   --coverage          使用 build_cov、开启 ENABLE_COVERAGE 构建并跑单元测试（会创建 build_cov）
#   --module NAME       只运行指定模块的测试（如 pid、queue、thread_pool）
#   --quick             只运行核心测试：thread_pool_test、dispatcher_test
#   --list              列出所有单元测试名称后退出
#   --verbose           使用 ctest -V 输出详细日志
#   -h, --help          打印此帮助
#

set -e
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
BUILD_DIR="$ROOT/build"
DO_CONFIGURE=""
DO_COVERAGE=""
MODULE=""
QUICK=""
LIST_ONLY=""
VERBOSE=""

while [ $# -gt 0 ]; do
  case "$1" in
    --build-dir)    BUILD_DIR="$2"; shift 2 ;;
    --configure)    DO_CONFIGURE=1; shift ;;
    --coverage)     DO_COVERAGE=1; BUILD_DIR="$ROOT/build_cov"; shift ;;
    --module)       MODULE="$2"; shift 2 ;;
    --quick)        QUICK=1; shift ;;
    --list)         LIST_ONLY=1; shift ;;
    --verbose)      VERBOSE=1; shift ;;
    -h|--help)
      sed -n '2,22p' "$(dirname "${BASH_SOURCE[0]}")/run_unit_tests.sh" | sed 's/^# \?//'
      exit 0
      ;;
    *) echo "Unknown option: $1" >&2; exit 1 ;;
  esac
done

cd "$ROOT"

if [ -n "$LIST_ONLY" ]; then
  echo "Unit test executables (use with --module NAME):"
  echo "  unit_pid  unit_config  unit_platform  unit_queue  unit_pidset"
  echo "  unit_priority_queue  unit_messages  thread_pool_test  dispatcher_test"
  echo "Module names for --module: pid, config, platform, queue, pidset, priority_queue, messages, thread_pool, dispatcher"
  exit 0
fi

# 需要先配置时
if [ -n "$DO_COVERAGE" ]; then
  DO_CONFIGURE=1
  mkdir -p "$BUILD_DIR"
  (cd "$BUILD_DIR" && cmake .. -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTS=ON -DENABLE_COVERAGE=ON)
elif [ -n "$DO_CONFIGURE" ]; then
  if [ ! -f "$BUILD_DIR/CMakeCache.txt" ]; then
    mkdir -p "$BUILD_DIR"
    (cd "$BUILD_DIR" && cmake .. -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTS=ON)
  fi
fi

if [ ! -f "$BUILD_DIR/CMakeCache.txt" ]; then
  echo "Error: $BUILD_DIR not configured. Run with --configure or --coverage first." >&2
  exit 1
fi

# 构建
make -C "$BUILD_DIR" -j"${MAKE_JOBS:-4}" 2>&1

# 运行测试
CTEST_EXTRA=""
[ -n "$VERBOSE" ] && CTEST_EXTRA="$CTEST_EXTRA -V"
export PROTOACTOR_TEST=1

if [ -n "$QUICK" ]; then
  cd "$BUILD_DIR" && ctest -R "thread_pool_test|dispatcher_test" --output-on-failure $CTEST_EXTRA
elif [ -n "$MODULE" ]; then
  case "$MODULE" in
    pid)              name=unit_pid ;;
    config)           name=unit_config ;;
    platform)         name=unit_platform ;;
    queue)            name=unit_queue ;;
    pidset)           name=unit_pidset ;;
    priority_queue)   name=unit_priority_queue ;;
    messages)         name=unit_messages ;;
    thread_pool)      name=thread_pool_test ;;
    dispatcher)       name=dispatcher_test ;;
    *) echo "Unknown module: $MODULE" >&2; exit 1 ;;
  esac
  cd "$BUILD_DIR" && ctest -R "^${name}$" --output-on-failure $CTEST_EXTRA
else
  cd "$BUILD_DIR" && ctest -L unit --output-on-failure $CTEST_EXTRA
fi
