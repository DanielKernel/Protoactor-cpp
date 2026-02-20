/**
 * Unit tests for Supervision module (Directive, strategies, RestartStatistics).
 */
#include "external/supervision.h"
#include "tests/test_common.h"
#include <cstdio>
#include <chrono>

using namespace protoactor;
using namespace protoactor::test;

static bool test_directive_enum_values() {
    ASSERT_EQ(static_cast<int>(Directive::Resume), 0);
    ASSERT_EQ(static_cast<int>(Directive::Restart), 1);
    ASSERT_EQ(static_cast<int>(Directive::Stop), 2);
    ASSERT_EQ(static_cast<int>(Directive::Escalate), 3);
    return true;
}

static bool test_new_one_for_one_strategy() {
    auto decider = [](std::shared_ptr<void>) { return Directive::Restart; };
    auto s = NewOneForOneStrategy(5, std::chrono::milliseconds(1000), decider);
    ASSERT_TRUE(s != nullptr);
    return true;
}

static bool test_new_all_for_one_strategy() {
    auto decider = [](std::shared_ptr<void>) { return Directive::Stop; };
    auto s = NewAllForOneStrategy(3, std::chrono::milliseconds(500), decider);
    ASSERT_TRUE(s != nullptr);
    return true;
}

static bool test_new_restarting_strategy() {
    auto s = NewRestartingStrategy();
    ASSERT_TRUE(s != nullptr);
    return true;
}

static bool test_default_supervisor_strategy() {
    auto s = DefaultSupervisorStrategy();
    ASSERT_TRUE(s != nullptr);
    return true;
}

static bool test_default_decider_returns_restart() {
    auto d = DefaultDecider(nullptr);
    ASSERT_TRUE(d == Directive::Restart);
    return true;
}

static bool test_restart_statistics() {
    RestartStatistics rs;
    ASSERT_TRUE(rs.failure_count == 0);
    rs.Fail();
    ASSERT_TRUE(rs.failure_count == 1);
    rs.Fail();
    ASSERT_TRUE(rs.failure_count == 2);
    rs.Reset();
    ASSERT_TRUE(rs.failure_count == 0);
    return true;
}

int main() {
    std::fprintf(stdout, "Supervision unit tests (module:supervision)\n");
    int failed = 0;
#define RUN(name) if (!run_test(#name, name)) ++failed
    RUN(test_directive_enum_values);
    RUN(test_new_one_for_one_strategy);
    RUN(test_new_all_for_one_strategy);
    RUN(test_new_restarting_strategy);
    RUN(test_default_supervisor_strategy);
    RUN(test_default_decider_returns_restart);
    RUN(test_restart_statistics);
#undef RUN
    std::fprintf(stdout, "\nTotal: %d failed\n", failed);
    return failed == 0 ? 0 : 1;
}
