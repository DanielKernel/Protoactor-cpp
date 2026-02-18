/**
 * Unit tests for PIDSet module.
 */
#include "protoactor/internal/pidset.h"
#include "protoactor/pid.h"
#include "tests/test_common.h"
#include <cstdio>
#include <memory>

using namespace protoactor;
using namespace protoactor::test;

static bool test_pidset_new_empty() {
    auto s = PIDSet::New();
    ASSERT_TRUE(s != nullptr);
    ASSERT_TRUE(s->Empty());
    ASSERT_EQ(s->Len(), 0u);
    return true;
}

static bool test_pidset_add_contains() {
    auto s = PIDSet::New();
    auto p = NewPID("addr", "id1");
    s->Add(p);
    ASSERT_TRUE(!s->Empty());
    ASSERT_EQ(s->Len(), 1u);
    ASSERT_TRUE(s->Contains(p));
    return true;
}

static bool test_pidset_add_twice_same_pid() {
    auto s = PIDSet::New();
    auto p = NewPID("addr", "id1");
    s->Add(p);
    s->Add(p);
    ASSERT_EQ(s->Len(), 1u);
    ASSERT_TRUE(s->Contains(p));
    return true;
}

static bool test_pidset_remove() {
    auto s = PIDSet::New();
    auto p = NewPID("addr", "id1");
    s->Add(p);
    ASSERT_TRUE(s->Contains(p));
    s->Remove(p);
    ASSERT_TRUE(!s->Contains(p));
    ASSERT_TRUE(s->Empty());
    ASSERT_EQ(s->Len(), 0u);
    return true;
}

static bool test_pidset_get_all() {
    auto s = PIDSet::New();
    auto p1 = NewPID("a", "1");
    auto p2 = NewPID("a", "2");
    s->Add(p1);
    s->Add(p2);
    auto all = s->GetAll();
    ASSERT_EQ(all.size(), 2u);
    ASSERT_EQ(s->Len(), 2u);
    return true;
}

static bool test_pidset_clear() {
    auto s = PIDSet::New();
    s->Add(NewPID("a", "1"));
    s->Add(NewPID("a", "2"));
    ASSERT_EQ(s->Len(), 2u);
    s->Clear();
    ASSERT_TRUE(s->Empty());
    ASSERT_EQ(s->Len(), 0u);
    return true;
}

static bool test_pidset_contains_different_pid_same_address_id() {
    auto s = PIDSet::New();
    auto p1 = NewPID("addr", "id");
    s->Add(p1);
    auto p2 = NewPID("addr", "id");
    ASSERT_TRUE(s->Contains(p2));
    return true;
}

int main() {
    std::fprintf(stdout, "PIDSet unit tests (module:pidset)\n");
    int failed = 0;
#define RUN(name) if (!run_test(#name, name)) ++failed
    RUN(test_pidset_new_empty);
    RUN(test_pidset_add_contains);
    RUN(test_pidset_add_twice_same_pid);
    RUN(test_pidset_remove);
    RUN(test_pidset_get_all);
    RUN(test_pidset_clear);
    RUN(test_pidset_contains_different_pid_same_address_id);
#undef RUN
    std::fprintf(stdout, "\nTotal: %d failed\n", failed);
    return failed == 0 ? 0 : 1;
}
