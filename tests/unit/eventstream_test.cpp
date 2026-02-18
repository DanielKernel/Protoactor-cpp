/**
 * Unit tests for EventStream module: New, Subscribe, Publish, Unsubscribe, Length.
 */
#include "external/eventstream.h"
#include "tests/test_common.h"
#include <atomic>
#include <cstdio>
#include <memory>

using namespace protoactor;
using namespace protoactor::test;
using namespace protoactor::eventstream;

static bool test_eventstream_new() {
    auto es = EventStream::New();
    ASSERT_TRUE(es != nullptr);
    ASSERT_TRUE(es->Length() == 0);
    return true;
}

static bool test_eventstream_subscribe_publish() {
    auto es = EventStream::New();
    std::atomic<int> received(0);
    auto sub = es->Subscribe([&received](std::shared_ptr<void> evt) {
        if (evt) received.fetch_add(1, std::memory_order_relaxed);
    });
    ASSERT_TRUE(sub != nullptr);
    ASSERT_TRUE(es->Length() == 1);
    es->Publish(std::make_shared<int>(1));
    ASSERT_TRUE(received.load() == 1);
    es->Publish(std::make_shared<int>(2));
    ASSERT_TRUE(received.load() == 2);
    return true;
}

static bool test_eventstream_unsubscribe() {
    auto es = EventStream::New();
    auto sub = es->Subscribe([](std::shared_ptr<void>) {});
    ASSERT_TRUE(es->Length() == 1);
    es->Unsubscribe(sub);
    ASSERT_TRUE(es->Length() == 0);
    return true;
}

static bool test_eventstream_multiple_subscribers() {
    auto es = EventStream::New();
    std::atomic<int> c1(0), c2(0);
    es->Subscribe([&c1](std::shared_ptr<void>) { c1.fetch_add(1, std::memory_order_relaxed); });
    es->Subscribe([&c2](std::shared_ptr<void>) { c2.fetch_add(1, std::memory_order_relaxed); });
    es->Publish(nullptr);
    ASSERT_TRUE(c1.load() == 1);
    ASSERT_TRUE(c2.load() == 1);
    return true;
}

int main() {
    std::fprintf(stdout, "EventStream unit tests (module:eventstream)\n");
    int failed = 0;
#define RUN(name) if (!run_test(#name, name)) ++failed
    RUN(test_eventstream_new);
    RUN(test_eventstream_subscribe_publish);
    RUN(test_eventstream_unsubscribe);
    RUN(test_eventstream_multiple_subscribers);
#undef RUN
    std::fprintf(stdout, "\nTotal: %d failed\n", failed);
    return failed == 0 ? 0 : 1;
}
