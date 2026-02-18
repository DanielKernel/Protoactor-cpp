#ifndef PROTOACTOR_PLATFORM_H
#define PROTOACTOR_PLATFORM_H

// Platform detection and compatibility macros
// Linux server only, 64-bit only

#if defined(__linux__)
    #define PROTOACTOR_PLATFORM_LINUX
    #define PROTOACTOR_PLATFORM_64BIT
#elif defined(__APPLE__) && defined(__MACH__)
    // Allow macOS for development/testing
    #define PROTOACTOR_PLATFORM_MACOS
    #define PROTOACTOR_PLATFORM_64BIT
#else
    #error "ProtoActor C++ only supports Linux platform"
#endif

// Architecture detection (64-bit only)
#if defined(__aarch64__) || defined(__ARM_ARCH_8A__)
    #define PROTOACTOR_ARCH_ARM64
#elif defined(__x86_64__) || defined(_M_X64)
    #define PROTOACTOR_ARCH_X86_64
#else
    #error "Unsupported architecture. Only x86_64 and ARM64 are supported."
#endif

// Compiler detection
#if defined(__GNUC__)
    #define PROTOACTOR_COMPILER_GCC
    #define PROTOACTOR_COMPILER_VERSION __GNUC__
#elif defined(__clang__)
    #define PROTOACTOR_COMPILER_CLANG
    #define PROTOACTOR_COMPILER_VERSION __clang_major__
#elif defined(_MSC_VER)
    #define PROTOACTOR_COMPILER_MSVC
    #define PROTOACTOR_COMPILER_VERSION _MSC_VER
#else
    #define PROTOACTOR_COMPILER_UNKNOWN
#endif

// Endianness detection
#if defined(__BYTE_ORDER__)
    #if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
        #define PROTOACTOR_LITTLE_ENDIAN
    #elif __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
        #define PROTOACTOR_BIG_ENDIAN
    #endif
#elif defined(_WIN32) || defined(__LITTLE_ENDIAN__) || \
      (defined(__BYTE_ORDER) && __BYTE_ORDER == __LITTLE_ENDIAN)
    #define PROTOACTOR_LITTLE_ENDIAN
#elif defined(__BIG_ENDIAN__) || \
      (defined(__BYTE_ORDER) && __BYTE_ORDER == __BIG_ENDIAN)
    #define PROTOACTOR_BIG_ENDIAN
#else
    #error "Cannot determine endianness"
#endif

// Thread-local storage (Linux only)
#define PROTOACTOR_THREAD_LOCAL thread_local

// Atomic operations
#include <atomic>
#include <cstdint>
#include <thread>

namespace protoactor {
namespace platform {

/**
 * @brief Get the number of CPU cores.
 * @return Number of cores
 */
int GetCPUCount();

/**
 * @brief Memory barrier for atomic operations.
 */
inline void MemoryBarrier() {
    std::atomic_thread_fence(std::memory_order_seq_cst);
}

/**
 * @brief Pause instruction for spin loops.
 */
inline void CPUPause() {
#if defined(PROTOACTOR_ARCH_X86_64)
    __asm__ __volatile__("pause" ::: "memory");
#elif defined(PROTOACTOR_ARCH_ARM64)
    __asm__ __volatile__("yield" ::: "memory");
#else
    // Generic yield (should not reach here)
    std::this_thread::yield();
#endif
}

} // namespace platform
} // namespace protoactor

#endif // PROTOACTOR_PLATFORM_H
