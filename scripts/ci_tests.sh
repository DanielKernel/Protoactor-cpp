#!/usr/bin/env bash
#
# CI 用测试脚本：配置、构建、仅运行单元测试，以退出码表示成功/失败。
# 在项目根目录执行：./scripts/ci_tests.sh [build_dir]
# 默认 build_dir = build
#

set -e
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${1:-$ROOT/build}"
cd "$ROOT"

echo "[ci_tests] Configure and build..."
mkdir -p "$BUILD_DIR"
(cd "$BUILD_DIR" && cmake .. -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTS=ON -DBUILD_EXAMPLES=OFF)
make -C "$BUILD_DIR" -j"${MAKE_JOBS:-4}"

echo "[ci_tests] Run unit tests (ctest -L unit)..."
export PROTOACTOR_TEST=1
cd "$BUILD_DIR"
ctest -L unit --output-on-failure

echo "[ci_tests] All unit tests passed."
exit 0
