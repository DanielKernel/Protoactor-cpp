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
(# Sanitize env to avoid using Conda libraries during CI build/run
 export OLD_LD_LIBRARY_PATH="$LD_LIBRARY_PATH" || true
 if [ -n "${LD_LIBRARY_PATH:-}" ]; then
	 NEW_LD_PATH="$(echo "$LD_LIBRARY_PATH" | awk -v RS=: -v ORS=: '/\/opt\/conda\/lib/ {next} {print}' | sed 's/:$//')"
	 export LD_LIBRARY_PATH="$NEW_LD_PATH"
 else
	 export LD_LIBRARY_PATH=""
 fi
 unset LD_RUN_PATH || true
 (cd "$BUILD_DIR" && cmake .. -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTS=ON -DBUILD_EXAMPLES=OFF -DCMAKE_SKIP_RPATH=ON -DCMAKE_BUILD_WITH_INSTALL_RPATH=OFF -DCMAKE_INSTALL_RPATH="")
 make -C "$BUILD_DIR" -j"${MAKE_JOBS:-4}"
 # restore old env
 export LD_LIBRARY_PATH="$OLD_LD_LIBRARY_PATH" || true
)

echo "[ci_tests] Run unit tests (ctest -L unit)..."
export PROTOACTOR_TEST=1
cd "$BUILD_DIR"
ctest -L unit --output-on-failure

echo "[ci_tests] All unit tests passed."
exit 0
