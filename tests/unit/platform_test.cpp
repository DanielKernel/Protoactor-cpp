/**
 * Unit tests for Platform module.
 */
#include "internal/platform.h"
#include "tests/test_common.h"
#include <cstdio>

using namespace protoactor;
using namespace protoactor::test;

static bool test_get_cpu_count_positive() {
    int n = platform::GetCPUCount();
    ASSERT_GE(n, 1);
    return true;
}

static bool test_memory_barrier_no_crash() {
    platform::MemoryBarrier();
    return true;
}

static bool test_cpu_pause_no_crash() {
    platform::CPUPause();
    return true;
}

int main() {
    std::fprintf(stdout, "Platform unit tests (module:platform)\n");
    int failed = 0;
#define RUN(name) if (!run_test(#name, name)) ++failed
    RUN(test_get_cpu_count_positive);
    RUN(test_memory_barrier_no_crash);
    RUN(test_cpu_pause_no_crash);
#undef RUN
    std::fprintf(stdout, "\nTotal: %d failed\n", failed);
    return failed == 0 ? 0 : 1;
}
