#!/usr/bin/env bash
#
# CI 用测试脚本：配置、构建、仅运行单元测试，以退出码表示成功/失败。
# 在项目根目录执行：./tests/scripts/ci_tests.sh [build_dir]
# 默认 build_dir = build
#

set -e
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
BUILD_DIR="${1:-$ROOT/build}"
cd "$ROOT"

echo "[ci_tests] Configure and build..."
mkdir -p "$BUILD_DIR"
# Sanitize env to avoid using Conda libraries during CI build/run
 export OLD_LD_LIBRARY_PATH="$LD_LIBRARY_PATH" || true
 if [ -n "${LD_LIBRARY_PATH:-}" ]; then
	 NEW_LD_PATH="$(echo "$LD_LIBRARY_PATH" | awk -v RS=: -v ORS=: '/\/opt\/conda\/lib/ {next} {print}' | sed 's/:$//')"
	 export LD_LIBRARY_PATH="$NEW_LD_PATH"
 else
	 export LD_LIBRARY_PATH=""
 fi
 unset LD_RUN_PATH || true

# Optional cross-build for ARM64: set CROSS_ARM64=1 to enable
if [ "${CROSS_ARM64:-0}" = "1" ]; then
	echo "[ci_tests] CROSS_ARM64=1 detected: preparing cross toolchain and performing cross build"

	# Ensure cross compilers exist or attempt install
	if ! command -v aarch64-linux-gnu-g++ >/dev/null 2>&1; then
		echo "[ci_tests] aarch64 cross-compiler not found"
		if command -v apt-get >/dev/null 2>&1; then
			if [ "$(id -u)" -eq 0 ]; then
				apt-get update && apt-get install -y gcc-aarch64-linux-gnu g++-aarch64-linux-gnu || true
			elif command -v sudo >/dev/null 2>&1; then
				echo "[ci_tests] Attempting to install cross-toolchain with sudo"
				sudo apt-get update && sudo apt-get install -y gcc-aarch64-linux-gnu g++-aarch64-linux-gnu || true
			else
				echo "[ci_tests] Cannot install cross-toolchain automatically; please install 'gcc-aarch64-linux-gnu' and 'g++-aarch64-linux-gnu'"
				exit 1
			fi
		else
			echo "[ci_tests] apt-get not available; cannot auto-install cross-toolchain"
			exit 1
		fi
	fi

	# Configure cross-build (we typically don't run cross-built tests on the host)
	(cd "$BUILD_DIR" && cmake "$ROOT" -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTS=ON -DBUILD_EXAMPLES=OFF -DCMAKE_SYSTEM_NAME=Linux -DCMAKE_SYSTEM_PROCESSOR=aarch64 -DCMAKE_C_COMPILER=aarch64-linux-gnu-gcc -DCMAKE_CXX_COMPILER=aarch64-linux-gnu-g++ -DENABLE_SPDLOG=OFF -DCMAKE_SKIP_RPATH=ON -DCMAKE_BUILD_WITH_INSTALL_RPATH=OFF -DCMAKE_INSTALL_RPATH="")
	make -C "$BUILD_DIR" -j"${MAKE_JOBS:-4}"
	echo "[ci_tests] Cross-build complete. Skipping running cross-built tests on host."
	# restore old env
	export LD_LIBRARY_PATH="$OLD_LD_LIBRARY_PATH" || true
	CROSS_PERFORMED=1

fi

(cd "$BUILD_DIR" && cmake "$ROOT" -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTS=ON -DBUILD_EXAMPLES=OFF -DCMAKE_SKIP_RPATH=ON -DCMAKE_BUILD_WITH_INSTALL_RPATH=OFF -DCMAKE_INSTALL_RPATH="")
make -C "$BUILD_DIR" -j"${MAKE_JOBS:-4}"
# restore old env
export LD_LIBRARY_PATH="$OLD_LD_LIBRARY_PATH" || true

# If we performed a cross-build, we skip running ctest on host (cross-built tests won't run here)
if [ "${CROSS_PERFORMED:-0}" = "1" ]; then
	echo "[ci_tests] Cross-build completed; skipping running tests on host."
	exit 0
fi

echo "[ci_tests] Run unit tests (ctest -L unit)..."
export PROTOACTOR_TEST=1
cd "$BUILD_DIR"
ctest -L unit --output-on-failure

echo "[ci_tests] All unit tests passed."
exit 0
