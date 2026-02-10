#!/bin/bash

# Linux build script for ProtoActor C++
# Supports ARM and x86 architectures on Linux servers

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Default values
BUILD_TYPE="Release"
TARGET_ARCH=""
CLEAN=false
EXAMPLES=true
TESTS=false
PROTOBUF=false

# Parse arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        --clean)
            CLEAN=true
            shift
            ;;
        --debug)
            BUILD_TYPE="Debug"
            shift
            ;;
        --arch)
            TARGET_ARCH="$2"
            # Validate architecture
            case $TARGET_ARCH in
                x86_64|arm64)
                    ;;
                *)
                    echo -e "${RED}Error: Unsupported architecture $TARGET_ARCH. Only x86_64 and arm64 are supported.${NC}"
                    exit 1
                    ;;
            esac
            shift 2
            ;;
        --examples)
            EXAMPLES=true
            shift
            ;;
        --no-examples)
            EXAMPLES=false
            shift
            ;;
        --tests)
            TESTS=true
            shift
            ;;
        --protobuf)
            PROTOBUF=true
            shift
            ;;
        --help)
            echo "Usage: $0 [OPTIONS]"
            echo "Options:"
            echo "  --clean          Clean build directory"
            echo "  --debug          Build in Debug mode (default: Release)"
            echo "  --arch ARCH      Target architecture (x86_64, arm64)"
            echo "  --examples       Build examples (default: on)"
            echo "  --no-examples    Don't build examples"
            echo "  --tests          Build tests"
            echo "  --protobuf       Enable Protobuf support"
            echo "  --help           Show this help message"
            exit 0
            ;;
        *)
            echo -e "${RED}Unknown option: $1${NC}"
            exit 1
            ;;
    esac
done

# Detect current architecture if not specified (64-bit only)
if [ -z "$TARGET_ARCH" ]; then
    ARCH=$(uname -m)
    case $ARCH in
        aarch64|arm64)
            TARGET_ARCH="arm64"
            ;;
        x86_64|amd64)
            TARGET_ARCH="x86_64"
            ;;
        *)
            echo -e "${RED}Error: Unsupported architecture $ARCH. Only x86_64 and ARM64 are supported.${NC}"
            exit 1
            ;;
    esac
fi

# Verify we're on Linux
if [ "$(uname -s)" != "Linux" ]; then
    echo -e "${RED}Error: This build script only supports Linux${NC}"
    exit 1
fi

echo -e "${GREEN}Building ProtoActor C++${NC}"
echo "Build type: $BUILD_TYPE"
echo "Target architecture: $TARGET_ARCH"
echo "Build examples: $EXAMPLES"
echo "Build tests: $TESTS"
echo "Protobuf: $PROTOBUF"
echo ""

# Create build directory
BUILD_DIR="build-${TARGET_ARCH}-${BUILD_TYPE}"
mkdir -p "$BUILD_DIR"

# Clean if requested
if [ "$CLEAN" = true ]; then
    echo -e "${YELLOW}Cleaning build directory...${NC}"
    rm -rf "$BUILD_DIR"
    mkdir -p "$BUILD_DIR"
fi

cd "$BUILD_DIR"

# Configure CMake
CMAKE_ARGS=(
    -DCMAKE_BUILD_TYPE="$BUILD_TYPE"
    -DBUILD_EXAMPLES="$EXAMPLES"
    -DBUILD_TESTS="$TESTS"
    -DENABLE_PROTOBUF="$PROTOBUF"
)

# Set architecture-specific flags (64-bit only)
case $TARGET_ARCH in
    arm64)
        CMAKE_ARGS+=(-DCMAKE_SYSTEM_PROCESSOR=aarch64)
        ;;
    x86_64)
        CMAKE_ARGS+=(-DCMAKE_SYSTEM_PROCESSOR=x86_64)
        ;;
    *)
        echo -e "${RED}Error: Invalid architecture $TARGET_ARCH${NC}"
        exit 1
        ;;
esac

echo -e "${GREEN}Configuring CMake...${NC}"
cmake .. "${CMAKE_ARGS[@]}"

# Build
echo -e "${GREEN}Building...${NC}"
cmake --build . -j$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)

echo -e "${GREEN}Build completed successfully!${NC}"
echo "Build directory: $BUILD_DIR"
if [ "$EXAMPLES" = true ]; then
    echo "Examples are in: $BUILD_DIR/bin"
fi
