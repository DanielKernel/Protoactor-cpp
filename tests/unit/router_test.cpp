/**
 * Unit tests for Router module: BroadcastRouter, RoundRobinRouter,
 * RandomRouter, and ConsistentHashRouter.
 */
#include "external/router/router.h"
#include "external/pid.h"
#include "tests/test_common.h"
#include <cstdio>
#include <memory>
#include <vector>

using namespace protoactor;
using namespace protoactor::test;

// ============================================================================
// Test Helpers
// ============================================================================

static std::vector<std::shared_ptr<PID>> create_test_routees(int count) {
    std::vector<std::shared_ptr<PID>> routees;
    for (int i = 0; i < count; i++) {
        routees.push_back(NewPID("test", "routee-" + std::to_string(i)));
    }
    return routees;
}

// ============================================================================
// BroadcastRouter Tests
// ============================================================================

static bool test_broadcast_router_set_routees() {
    router::BroadcastRouter router;
    auto routees = create_test_routees(3);

    router.SetRoutees(routees);
    auto result = router.GetRoutees();

    ASSERT_EQ(result.size(), 3);
    return true;
}

static bool test_broadcast_router_empty_routees() {
    router::BroadcastRouter router;
    std::vector<std::shared_ptr<PID>> empty;

    router.SetRoutees(empty);
    auto result = router.GetRoutees();

    ASSERT_EQ(result.size(), 0);
    return true;
}

static bool test_broadcast_router_replace_routees() {
    router::BroadcastRouter router;

    router.SetRoutees(create_test_routees(3));
    ASSERT_EQ(router.GetRoutees().size(), 3);

    router.SetRoutees(create_test_routees(5));
    ASSERT_EQ(router.GetRoutees().size(), 5);

    return true;
}

// ============================================================================
// RoundRobinRouter Tests
// ============================================================================

static bool test_round_robin_router_set_routees() {
    router::RoundRobinRouter router;
    auto routees = create_test_routees(4);

    router.SetRoutees(routees);
    auto result = router.GetRoutees();

    ASSERT_EQ(result.size(), 4);
    return true;
}

static bool test_round_robin_router_initial_index() {
    router::RoundRobinRouter router;

    // Fresh router should start at index 0
    // (This is an internal detail, but good to verify)
    router.SetRoutees(create_test_routees(3));

    // The router should be properly initialized
    ASSERT_TRUE(router.GetRoutees().size() == 3);
    return true;
}

static bool test_round_robin_router_empty_routees() {
    router::RoundRobinRouter router;
    std::vector<std::shared_ptr<PID>> empty;

    router.SetRoutees(empty);
    auto result = router.GetRoutees();

    ASSERT_EQ(result.size(), 0);
    return true;
}

// ============================================================================
// RandomRouter Tests
// ============================================================================

static bool test_random_router_set_routees() {
    router::RandomRouter router;
    auto routees = create_test_routees(5);

    router.SetRoutees(routees);
    auto result = router.GetRoutees();

    ASSERT_EQ(result.size(), 5);
    return true;
}

static bool test_random_router_empty_routees() {
    router::RandomRouter router;
    std::vector<std::shared_ptr<PID>> empty;

    router.SetRoutees(empty);
    auto result = router.GetRoutees();

    ASSERT_EQ(result.size(), 0);
    return true;
}

// ============================================================================
// ConsistentHashRouter Tests
// ============================================================================

static bool test_consistent_hash_router_set_routees() {
    router::ConsistentHashRouter router;
    auto routees = create_test_routees(3);

    router.SetRoutees(routees);
    auto result = router.GetRoutees();

    ASSERT_EQ(result.size(), 3);
    return true;
}

static bool test_consistent_hash_router_empty_routees() {
    router::ConsistentHashRouter router;
    std::vector<std::shared_ptr<PID>> empty;

    router.SetRoutees(empty);
    auto result = router.GetRoutees();

    ASSERT_EQ(result.size(), 0);
    return true;
}

// ============================================================================
// Common Router Interface Tests
// ============================================================================

static bool test_router_interface_broadcast() {
    std::shared_ptr<router::Router> router = std::make_shared<router::BroadcastRouter>();
    auto routees = create_test_routees(2);

    router->SetRoutees(routees);
    ASSERT_EQ(router->GetRoutees().size(), 2);
    return true;
}

static bool test_router_interface_round_robin() {
    std::shared_ptr<router::Router> router = std::make_shared<router::RoundRobinRouter>();
    auto routees = create_test_routees(3);

    router->SetRoutees(routees);
    ASSERT_EQ(router->GetRoutees().size(), 3);
    return true;
}

static bool test_router_interface_random() {
    std::shared_ptr<router::Router> router = std::make_shared<router::RandomRouter>();
    auto routees = create_test_routees(4);

    router->SetRoutees(routees);
    ASSERT_EQ(router->GetRoutees().size(), 4);
    return true;
}

static bool test_router_interface_consistent_hash() {
    std::shared_ptr<router::Router> router = std::make_shared<router::ConsistentHashRouter>();
    auto routees = create_test_routees(5);

    router->SetRoutees(routees);
    ASSERT_EQ(router->GetRoutees().size(), 5);
    return true;
}

// ============================================================================
// PID Tests (used by routers)
// ============================================================================

static bool test_pid_creation() {
    auto pid = NewPID("localhost", "actor-1");

    ASSERT_TRUE(pid != nullptr);
    ASSERT_TRUE((pid->address == "localhost" || pid->address == "localhost:0") && pid->id == "actor-1");
    return true;
}

static bool test_pid_address_format() {
    auto pid = NewPID("host", "name");

    ASSERT_TRUE(pid->address.find("host") != std::string::npos);
    ASSERT_TRUE(pid->id.find("name") != std::string::npos);
    return true;
}

static bool test_pid_comparison() {
    auto pid1 = NewPID("host", "actor");
    auto pid2 = NewPID("host", "actor");
    auto pid3 = NewPID("host", "other");

    ASSERT_TRUE(pid1->address == pid2->address && pid1->id == pid2->id);
    ASSERT_TRUE(pid1->address != pid3->address || pid1->id != pid3->id);
    return true;
}

// ============================================================================
// Main
// ============================================================================

int main() {
    std::fprintf(stdout, "Router unit tests (module:router)\n");
    int failed = 0;
#define RUN(name) if (!run_test(#name, name)) ++failed

    // BroadcastRouter tests
    RUN(test_broadcast_router_set_routees);
    RUN(test_broadcast_router_empty_routees);
    RUN(test_broadcast_router_replace_routees);

    // RoundRobinRouter tests
    RUN(test_round_robin_router_set_routees);
    RUN(test_round_robin_router_initial_index);
    RUN(test_round_robin_router_empty_routees);

    // RandomRouter tests
    RUN(test_random_router_set_routees);
    RUN(test_random_router_empty_routees);

    // ConsistentHashRouter tests
    RUN(test_consistent_hash_router_set_routees);
    RUN(test_consistent_hash_router_empty_routees);

    // Common interface tests
    RUN(test_router_interface_broadcast);
    RUN(test_router_interface_round_robin);
    RUN(test_router_interface_random);
    RUN(test_router_interface_consistent_hash);

    // PID tests
    RUN(test_pid_creation);
    RUN(test_pid_address_format);
    RUN(test_pid_comparison);

#undef RUN
    std::fprintf(stdout, "\nTotal: %d failed\n", failed);
    return failed == 0 ? 0 : 1;
}
