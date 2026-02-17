/**
 * Unit tests for Persistence module: InMemoryProvider, EventStore,
 * and SnapshotStore interfaces.
 */
#include "protoactor/persistence/persistence.h"
#include "tests/test_common.h"
#include <cstdio>
#include <memory>
#include <string>

using namespace protoactor;
using namespace protoactor::test;

// ============================================================================
// InMemoryProvider Tests
// ============================================================================

static bool test_in_memory_provider_creation() {
    persistence::InMemoryProvider provider;
    auto state = provider.GetState();

    ASSERT_TRUE(state != nullptr);
    return true;
}

static bool test_in_memory_provider_state_not_null() {
    persistence::InMemoryProvider provider;
    auto state = provider.GetState();

    // State should be valid
    ASSERT_TRUE(state != nullptr);

    // Multiple calls should return same or equivalent state
    auto state2 = provider.GetState();
    ASSERT_TRUE(state2 != nullptr);
    return true;
}

// ============================================================================
// ProviderState Interface Tests
// ============================================================================

static bool test_provider_state_snapshot_interval() {
    persistence::InMemoryProvider provider;
    auto state = provider.GetState();

    // Default snapshot interval should be defined
    int interval = state->GetSnapshotInterval();
    ASSERT_TRUE(interval > 0);
    return true;
}

static bool test_provider_state_restart() {
    persistence::InMemoryProvider provider;
    auto state = provider.GetState();

    // Restart should not throw
    state->Restart();
    return true;
}

// ============================================================================
// EventStore Interface Tests
// ============================================================================

static bool test_event_store_get_events_empty() {
    persistence::InMemoryProvider provider;
    auto state = provider.GetState();

    int call_count = 0;
    state->GetEvents("nonexistent-actor", 0, -1, [&call_count](auto) {
        call_count++;
    });

    // Should not call callback for empty event store
    ASSERT_EQ(call_count, 0);
    return true;
}

static bool test_event_store_persist_and_get_event() {
    persistence::InMemoryProvider provider;
    auto state = provider.GetState();

    // Persist an event (using void* since no protobuf)
    auto event = std::make_shared<int>(42);
    state->PersistEvent("test-actor", 0, event);

    // Retrieve the event
    int found_count = 0;
    state->GetEvents("test-actor", 0, 1, [&found_count](auto e) {
        found_count++;
    });

    ASSERT_EQ(found_count, 1);
    return true;
}

static bool test_event_store_multiple_events() {
    persistence::InMemoryProvider provider;
    auto state = provider.GetState();

    // Persist multiple events
    for (int i = 0; i < 5; i++) {
        state->PersistEvent("multi-actor", i, std::make_shared<int>(i * 10));
    }

    // Retrieve all events
    int found_count = 0;
    state->GetEvents("multi-actor", 0, -1, [&found_count](auto) {
        found_count++;
    });

    ASSERT_EQ(found_count, 5);
    return true;
}

static bool test_event_store_delete_events() {
    persistence::InMemoryProvider provider;
    auto state = provider.GetState();

    // Persist events
    for (int i = 0; i < 3; i++) {
        state->PersistEvent("delete-actor", i, std::make_shared<int>(i));
    }

    // Delete first two events
    state->DeleteEvents("delete-actor", 1);

    // Only third event should remain
    int found_count = 0;
    state->GetEvents("delete-actor", 0, -1, [&found_count](auto) {
        found_count++;
    });

    ASSERT_EQ(found_count, 1);
    return true;
}

// ============================================================================
// SnapshotStore Interface Tests
// ============================================================================

static bool test_snapshot_store_get_empty() {
    persistence::InMemoryProvider provider;
    auto state = provider.GetState();

    auto [snapshot, index, found] = state->GetSnapshot("no-snapshot-actor");

    ASSERT_TRUE(!found);
    return true;
}

static bool test_snapshot_store_persist_and_get() {
    persistence::InMemoryProvider provider;
    auto state = provider.GetState();

    // Persist a snapshot
    auto snapshot = std::make_shared<std::string>("snapshot-data");
    state->PersistSnapshot("snapshot-actor", 5, snapshot);

    // Retrieve the snapshot
    auto [retrieved, index, found] = state->GetSnapshot("snapshot-actor");

    ASSERT_TRUE(found);
    ASSERT_EQ(index, 5);
    return true;
}

static bool test_snapshot_store_overwrite() {
    persistence::InMemoryProvider provider;
    auto state = provider.GetState();

    // Persist initial snapshot
    state->PersistSnapshot("overwrite-actor", 1, std::make_shared<std::string>("v1"));

    // Overwrite with new snapshot
    state->PersistSnapshot("overwrite-actor", 2, std::make_shared<std::string>("v2"));

    // Should get latest snapshot
    auto [snapshot, index, found] = state->GetSnapshot("overwrite-actor");

    ASSERT_TRUE(found);
    ASSERT_EQ(index, 2);
    return true;
}

static bool test_snapshot_store_delete_snapshots() {
    persistence::InMemoryProvider provider;
    auto state = provider.GetState();

    // Persist snapshot
    state->PersistSnapshot("delete-snap-actor", 10, std::make_shared<std::string>("data"));

    // Delete it
    state->DeleteSnapshots("delete-snap-actor", 10);

    // Should not find it
    auto [snapshot, index, found] = state->GetSnapshot("delete-snap-actor");

    ASSERT_TRUE(!found);
    return true;
}

// ============================================================================
// Provider Interface Tests
// ============================================================================

static bool test_provider_interface() {
    std::shared_ptr<persistence::Provider> provider = std::make_shared<persistence::InMemoryProvider>();

    auto state = provider->GetState();
    ASSERT_TRUE(state != nullptr);
    return true;
}

// ============================================================================
// Multi-Actor Tests
// ============================================================================

static bool test_multiple_actors_independent() {
    persistence::InMemoryProvider provider;
    auto state = provider.GetState();

    // Persist events for different actors
    state->PersistEvent("actor-1", 0, std::make_shared<int>(1));
    state->PersistEvent("actor-2", 0, std::make_shared<int>(2));
    state->PersistEvent("actor-1", 1, std::make_shared<int>(11));

    // Verify actor-1 has 2 events
    int count1 = 0;
    state->GetEvents("actor-1", 0, -1, [&count1](auto) { count1++; });

    // Verify actor-2 has 1 event
    int count2 = 0;
    state->GetEvents("actor-2", 0, -1, [&count2](auto) { count2++; });

    ASSERT_EQ(count1, 2);
    ASSERT_EQ(count2, 1);
    return true;
}

// ============================================================================
// Main
// ============================================================================

int main() {
    std::fprintf(stdout, "Persistence unit tests (module:persistence)\n");
    int failed = 0;
#define RUN(name) if (!run_test(#name, name)) ++failed

    // InMemoryProvider tests
    RUN(test_in_memory_provider_creation);
    RUN(test_in_memory_provider_state_not_null);

    // ProviderState tests
    RUN(test_provider_state_snapshot_interval);
    RUN(test_provider_state_restart);

    // EventStore tests
    RUN(test_event_store_get_events_empty);
    RUN(test_event_store_persist_and_get_event);
    RUN(test_event_store_multiple_events);
    RUN(test_event_store_delete_events);

    // SnapshotStore tests
    RUN(test_snapshot_store_get_empty);
    RUN(test_snapshot_store_persist_and_get);
    RUN(test_snapshot_store_overwrite);
    RUN(test_snapshot_store_delete_snapshots);

    // Provider interface tests
    RUN(test_provider_interface);

    // Multi-actor tests
    RUN(test_multiple_actors_independent);

#undef RUN
    std::fprintf(stdout, "\nTotal: %d failed\n", failed);
    return failed == 0 ? 0 : 1;
}
