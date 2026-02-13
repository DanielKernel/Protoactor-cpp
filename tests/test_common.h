#ifndef PROTOACTOR_TESTS_TEST_COMMON_H
#define PROTOACTOR_TESTS_TEST_COMMON_H

#include <cstdio>
#include <cstdlib>
#include <string>

namespace protoactor {
namespace test {

inline int& test_fail_count() {
    static int n = 0;
    return n;
}

inline void test_fail(const char* file, int line, const char* expr) {
    std::fprintf(stderr, "  FAIL %s:%d: %s\n", file, line, expr);
    ++test_fail_count();
}

#define ASSERT_TRUE(c) do { \
    if (!(c)) { protoactor::test::test_fail(__FILE__, __LINE__, #c); return false; } \
} while(0)

#define ASSERT_EQ(a, b) do { \
    if ((a) != (b)) { \
        std::fprintf(stderr, "  FAIL %s:%d: %s == %s (got %s)\n", __FILE__, __LINE__, #a, #b, "?"); \
        ++protoactor::test::test_fail_count(); \
        return false; \
    } \
} while(0)

#define ASSERT_GE(a, b) do { \
    if ((a) < (b)) { \
        std::fprintf(stderr, "  FAIL %s:%d: %s >= %s\n", __FILE__, __LINE__, #a, #b); \
        ++protoactor::test::test_fail_count(); \
        return false; \
    } \
} while(0)

#define EXPECT_TRUE(c) do { \
    if (!(c)) { protoactor::test::test_fail(__FILE__, __LINE__, #c); } \
} while(0)

inline bool run_test(const char* name, bool (*fn)()) {
    test_fail_count() = 0;
    std::fprintf(stdout, "  [ RUN  ] %s\n", name);
    bool ok = fn();
    if (ok && test_fail_count() == 0) {
        std::fprintf(stdout, "  [  OK  ] %s\n", name);
        return true;
    }
    std::fprintf(stdout, "  [ FAIL ] %s\n", name);
    return false;
}

} // namespace test
} // namespace protoactor

#endif // PROTOACTOR_TESTS_TEST_COMMON_H
