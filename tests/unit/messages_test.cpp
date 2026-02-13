/**
 * Unit tests for Messages module (lifecycle/control types, envelope, header, Wrap/Unwrap).
 */
#include "protoactor/messages.h"
#include "protoactor/pid.h"
#include "tests/test_common.h"
#include <cstdio>
#include <memory>
#include <string>
#include <tuple>

using namespace protoactor;
using namespace protoactor::test;

static bool test_started_default_construct() {
    Started s;
    (void)s;
    return true;
}

static bool test_stopped_default_construct() {
    Stopped s;
    (void)s;
    return true;
}

static bool test_message_envelope_construct() {
    MessageEnvelope env;
    ASSERT_TRUE(env.message == nullptr);
    ASSERT_TRUE(env.sender == nullptr);
    return true;
}

static bool test_message_envelope_with_args() {
    auto pid = NewPID("a", "1");
    auto msg = std::make_shared<int>(1);
    MessageEnvelope env(nullptr, msg, pid);
    ASSERT_TRUE(env.sender == pid);
    ASSERT_TRUE(env.message == msg);
    return true;
}

static bool test_message_envelope_get_header_null_header() {
    MessageEnvelope env;
    ASSERT_TRUE(env.GetHeader("x").empty());
    return true;
}

static bool test_message_envelope_set_header_creates_header() {
    MessageEnvelope env;
    env.SetHeader("k", "v");
    ASSERT_TRUE(env.GetHeader("k") == "v");
    return true;
}

static bool test_message_envelope_set_header_multiple() {
    MessageEnvelope env;
    env.SetHeader("a", "1");
    env.SetHeader("b", "2");
    ASSERT_TRUE(env.GetHeader("a") == "1");
    ASSERT_TRUE(env.GetHeader("b") == "2");
    return true;
}

static bool test_wrap_envelope_nullptr() {
    auto out = WrapEnvelope(nullptr);
    ASSERT_TRUE(out == nullptr);
    return true;
}

static bool test_wrap_envelope_already_envelope() {
    auto env = std::make_shared<MessageEnvelope>(nullptr, nullptr, nullptr);
    std::shared_ptr<void> v = std::static_pointer_cast<void>(env);
    auto out = WrapEnvelope(v);
    ASSERT_TRUE(out != nullptr);
    ASSERT_TRUE(out.get() == env.get());
    return true;
}

static bool test_unwrap_envelope_nullptr() {
    std::shared_ptr<void> nil;
    auto t = UnwrapEnvelope(nil);
    ASSERT_TRUE(std::get<0>(t) == nullptr);
    ASSERT_TRUE(std::get<1>(t) == nullptr);
    ASSERT_TRUE(std::get<2>(t) == nullptr);
    return true;
}

static bool test_unwrap_envelope_with_content() {
    auto pid = NewPID("addr", "id");
    auto msg = std::make_shared<int>(1);
    auto env = std::make_shared<MessageEnvelope>(nullptr, msg, pid);
    std::shared_ptr<void> v = std::static_pointer_cast<void>(env);
    auto t = UnwrapEnvelope(v);
    ASSERT_TRUE(std::get<1>(t) == msg);
    ASSERT_TRUE(std::get<2>(t) == pid);
    return true;
}

int main() {
    std::fprintf(stdout, "Messages unit tests (module:messages)\n");
    int failed = 0;
#define RUN(name) if (!run_test(#name, name)) ++failed
    RUN(test_started_default_construct);
    RUN(test_stopped_default_construct);
    RUN(test_message_envelope_construct);
    RUN(test_message_envelope_with_args);
    RUN(test_message_envelope_get_header_null_header);
    RUN(test_message_envelope_set_header_creates_header);
    RUN(test_message_envelope_set_header_multiple);
    RUN(test_wrap_envelope_nullptr);
    RUN(test_wrap_envelope_already_envelope);
    RUN(test_unwrap_envelope_nullptr);
    RUN(test_unwrap_envelope_with_content);
#undef RUN
    std::fprintf(stdout, "\nTotal: %d failed\n", failed);
    return failed == 0 ? 0 : 1;
}
