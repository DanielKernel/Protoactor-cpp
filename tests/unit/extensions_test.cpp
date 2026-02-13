/**
 * Unit tests for Extensions module: New, Register, Get, NextExtensionID.
 */
#include "protoactor/extensions.h"
#include "tests/test_common.h"
#include <cstdio>
#include <memory>

using namespace protoactor;
using namespace protoactor::test;

static bool test_extensions_new() {
    auto ext = Extensions::New();
    ASSERT_TRUE(ext != nullptr);
    return true;
}

static bool test_extensions_register_get() {
    auto ext = Extensions::New();
    auto obj = std::make_shared<int>(42);
    ext->Register(1, obj);
    auto got = ext->Get(1);
    ASSERT_TRUE(got != nullptr);
    ASSERT_TRUE(got == obj);
    return true;
}

static bool test_extensions_get_missing_returns_null() {
    auto ext = Extensions::New();
    auto got = ext->Get(999);
    ASSERT_TRUE(got == nullptr);
    return true;
}

static bool test_extensions_next_extension_id_increments() {
    ExtensionID a = Extensions::NextExtensionID();
    ExtensionID b = Extensions::NextExtensionID();
    ASSERT_TRUE(b == a + 1);
    return true;
}

int main() {
    std::fprintf(stdout, "Extensions unit tests (module:extensions)\n");
    int failed = 0;
#define RUN(name) if (!run_test(#name, name)) ++failed
    RUN(test_extensions_new);
    RUN(test_extensions_register_get);
    RUN(test_extensions_get_missing_returns_null);
    RUN(test_extensions_next_extension_id_increments);
#undef RUN
    std::fprintf(stdout, "\nTotal: %d failed\n", failed);
    return failed == 0 ? 0 : 1;
}
