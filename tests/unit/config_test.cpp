/**
 * Unit tests for Config module.
 */
#include "protoactor/config.h"
#include "tests/test_common.h"
#include <cstdio>
#include <chrono>

using namespace protoactor;
using namespace protoactor::test;

static bool test_config_default_not_null() {
    auto c = Config::Default();
    ASSERT_TRUE(c != nullptr);
    return true;
}

static bool test_config_default_values() {
    auto c = Config::Default();
    ASSERT_TRUE(c != nullptr);
    ASSERT_TRUE(c->dead_letter_request_logging == true);
    ASSERT_TRUE(c->dead_letter_throttle_count == 100);
    ASSERT_TRUE(c->dead_letter_throttle_interval == std::chrono::milliseconds(1000));
    return true;
}

static bool test_config_metrics_disabled_by_default() {
    auto c = Config::Default();
    ASSERT_TRUE(c != nullptr);
    ASSERT_TRUE(c->metrics_enabled == false);
    return true;
}

int main() {
    std::fprintf(stdout, "Config unit tests (module:config)\n");
    int failed = 0;
#define RUN(name) if (!run_test(#name, name)) ++failed
    RUN(test_config_default_not_null);
    RUN(test_config_default_values);
    RUN(test_config_metrics_disabled_by_default);
#undef RUN
    std::fprintf(stdout, "\nTotal: %d failed\n", failed);
    return failed == 0 ? 0 : 1;
}
