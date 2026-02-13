/**
 * Unit tests for PID module.
 */
#include "protoactor/pid.h"
#include "tests/test_common.h"
#include <cstdio>
#include <memory>

using namespace protoactor;
using namespace protoactor::test;

static bool test_new_pid_empty() {
    auto p = NewPID("", "");
    ASSERT_TRUE(p != nullptr);
    ASSERT_TRUE(p->address.empty());
    ASSERT_TRUE(p->id.empty());
    ASSERT_EQ(p->request_id, 0u);
    return true;
}

static bool test_new_pid_with_values() {
    auto p = NewPID("sys1", "actor/1");
    ASSERT_TRUE(p != nullptr);
    ASSERT_TRUE(p->address == "sys1");
    ASSERT_TRUE(p->id == "actor/1");
    return true;
}

static bool test_pid_equal_same() {
    auto a = NewPID("addr", "id1");
    auto b = NewPID("addr", "id1");
    ASSERT_TRUE(a->Equal(b));
    ASSERT_TRUE(b->Equal(a));
    return true;
}

static bool test_pid_equal_different_address() {
    auto a = NewPID("addr1", "id");
    auto b = NewPID("addr2", "id");
    ASSERT_TRUE(!a->Equal(b));
    return true;
}

static bool test_pid_equal_different_id() {
    auto a = NewPID("addr", "id1");
    auto b = NewPID("addr", "id2");
    ASSERT_TRUE(!a->Equal(b));
    return true;
}

static bool test_pid_equal_nullptr() {
    auto a = NewPID("addr", "id");
    ASSERT_TRUE(!a->Equal(nullptr));
    return true;
}

static bool test_pid_equal_different_request_id() {
    auto a = NewPID("addr", "id");
    auto b = NewPID("addr", "id");
    a->request_id = 1;
    b->request_id = 2;
    ASSERT_TRUE(!a->Equal(b));
    return true;
}

static bool test_pid_equal_same_request_id() {
    auto a = NewPID("addr", "id");
    auto b = NewPID("addr", "id");
    a->request_id = 99;
    b->request_id = 99;
    ASSERT_TRUE(a->Equal(b));
    return true;
}

int main() {
    std::fprintf(stdout, "PID unit tests (module:pid)\n");
    int failed = 0;
#define RUN(name) if (!run_test(#name, name)) ++failed
    RUN(test_new_pid_empty);
    RUN(test_new_pid_with_values);
    RUN(test_pid_equal_same);
    RUN(test_pid_equal_different_address);
    RUN(test_pid_equal_different_id);
    RUN(test_pid_equal_nullptr);
    RUN(test_pid_equal_different_request_id);
    RUN(test_pid_equal_same_request_id);
#undef RUN
    std::fprintf(stdout, "\nTotal: %d failed\n", failed);
    return failed == 0 ? 0 : 1;
}
