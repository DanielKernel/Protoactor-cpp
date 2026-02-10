# Multi-stage Dockerfile for cross-platform builds
# Supports ARM and x86 architectures

# Stage 1: Build environment
FROM ubuntu:22.04 AS builder

# Avoid interactive prompts
ENV DEBIAN_FRONTEND=noninteractive

# Install build dependencies
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    git \
    pkg-config \
    && rm -rf /var/lib/apt/lists/*

# Set working directory
WORKDIR /build

# Copy source code
COPY . .

# Build arguments
ARG TARGET_ARCH=x86_64
ARG BUILD_TYPE=Release

# Configure and build
RUN mkdir build && \
    cd build && \
    cmake .. \
        -DCMAKE_BUILD_TYPE=${BUILD_TYPE} \
        -DBUILD_EXAMPLES=ON \
        -DBUILD_TESTS=OFF \
        -DENABLE_PROTOBUF=OFF && \
    cmake --build . -j$(nproc)

# Stage 2: Runtime (minimal)
FROM ubuntu:22.04 AS runtime

RUN apt-get update && apt-get install -y \
    libstdc++6 \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app

# Copy binaries from builder
COPY --from=builder /build/build/bin/* /app/

# Default command
CMD ["./hello_world"]
