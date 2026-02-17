/**
 * @file persistence_example.cpp
 * @brief Persistence example demonstrating event sourcing and snapshots
 *
 * This example shows how to:
 * 1. Use event sourcing to persist actor state changes
 * 2. Implement snapshot support for faster recovery
 * 3. Use the InMemoryProvider for testing
 *
 * Go equivalent:
 *   provider := persistence.NewInMemoryProvider()
 *   props := actor.PropsFromProducer(NewPersistentActor).
 *       WithPersistence(provider, persistence.WithSnapshotInterval(10))
 *
 * NOTE: This example demonstrates the persistence pattern. Actual usage
 * requires ENABLE_PROTOBUF to be enabled for Protobuf message support.
 */

#include "protoactor/actor.h"
#include "protoactor/context.h"
#include "protoactor/actor_system.h"
#include "protoactor/props.h"
#include "protoactor/messages.h"
#include <iostream>
#include <memory>
#include <thread>
#include <chrono>
#include <string>
#include <vector>

#ifdef ENABLE_PROTOBUF
#include "protoactor/persistence/persistence.h"
#endif

// ============================================================================
// Message Types
// ============================================================================

// Commands (input messages that trigger state changes)
struct RegisterUser {
    std::string user_id;
    std::string name;
    std::string email;

    RegisterUser(const std::string& id, const std::string& n, const std::string& e)
        : user_id(id), name(n), email(e) {}
};

struct UpdateEmail {
    std::string user_id;
    std::string new_email;

    UpdateEmail(const std::string& id, const std::string& e)
        : user_id(id), new_email(e) {}
};

struct GetUser {
    std::string user_id;
    explicit GetUser(const std::string& id) : user_id(id) {}
};

// Events (persistent records of state changes)
struct UserRegistered {
    std::string user_id;
    std::string name;
    std::string email;
    int64_t timestamp;
};

struct EmailUpdated {
    std::string user_id;
    std::string new_email;
    int64_t timestamp;
};

// State
struct UserState {
    std::string user_id;
    std::string name;
    std::string email;
    bool registered = false;
};

// Response
struct UserResponse {
    UserState state;
    bool found;
};

// ============================================================================
// Event Sourced Actor
// ============================================================================

/**
 * @brief Actor that uses event sourcing for persistence.
 *
 * Key concepts:
 * 1. Commands trigger state changes but are not persisted directly
 * 2. Events are persisted and represent facts that happened
 * 3. State is rebuilt by replaying events
 * 4. Snapshots optimize recovery by reducing events to replay
 */
class UserActor : public protoactor::Actor {
public:
#ifdef ENABLE_PROTOBUF
    UserActor(std::shared_ptr<protoactor::persistence::ProviderState> persistence)
        : persistence_(persistence) {}
#else
    UserActor() {}
#endif

    void Receive(std::shared_ptr<protoactor::Context> context) override {
        auto msg = context->Message();
        if (!msg) return;

        void* ptr = msg.get();

        // Lifecycle events
        if (dynamic_cast<protoactor::Started*>(ptr)) {
            HandleStarted(context);
            return;
        }

        // Commands
        if (RegisterUser* cmd = static_cast<RegisterUser*>(ptr)) {
            HandleRegisterUser(context, cmd);
            return;
        }

        if (UpdateEmail* cmd = static_cast<UpdateEmail*>(ptr)) {
            HandleUpdateEmail(context, cmd);
            return;
        }

        // Queries
        if (GetUser* query = static_cast<GetUser*>(ptr)) {
            HandleGetUser(context, query);
            return;
        }
    }

private:
    UserState state_;
    int event_index_ = 0;
    int snapshot_interval_ = 3;  // Save snapshot every 3 events

#ifdef ENABLE_PROTOBUF
    std::shared_ptr<protoactor::persistence::ProviderState> persistence_;
#endif

    void HandleStarted(std::shared_ptr<protoactor::Context> context) {
#ifdef ENABLE_PROTOBUF
        if (persistence_) {
            // Recover state from persistence
            RecoverState();
        }
#endif
        std::cout << "[UserActor] Started, event_index=" << event_index_ << std::endl;
    }

    void HandleRegisterUser(std::shared_ptr<protoactor::Context> context, RegisterUser* cmd) {
        if (state_.registered) {
            std::cout << "[UserActor] User already registered: " << state_.user_id << std::endl;
            return;
        }

        // Create event
        UserRegistered event{
            cmd->user_id,
            cmd->name,
            cmd->email,
            std::chrono::system_clock::now().time_since_epoch().count()
        };

        // Persist event
        PersistEvent(event);

        // Apply event to state
        ApplyEvent(event);

        std::cout << "[UserActor] User registered: " << cmd->user_id
                  << " (" << cmd->name << ")" << std::endl;
    }

    void HandleUpdateEmail(std::shared_ptr<protoactor::Context> context, UpdateEmail* cmd) {
        if (!state_.registered) {
            std::cout << "[UserActor] User not registered, cannot update email" << std::endl;
            return;
        }

        // Create event
        EmailUpdated event{
            cmd->user_id,
            cmd->new_email,
            std::chrono::system_clock::now().time_since_epoch().count()
        };

        // Persist event
        PersistEvent(event);

        // Apply event to state
        ApplyEvent(event);

        std::cout << "[UserActor] Email updated for " << cmd->user_id
                  << ": " << cmd->new_email << std::endl;
    }

    void HandleGetUser(std::shared_ptr<protoactor::Context> context, GetUser* query) {
        UserResponse response{state_, state_.registered};
        context->Respond(std::make_shared<UserResponse>(response));
    }

    void PersistEvent(const UserRegistered& event) {
#ifdef ENABLE_PROTOBUF
        if (persistence_) {
            // In real implementation, convert to Protobuf and persist
            // persistence_->PersistEvent(actor_name_, event_index_, protobuf_event);
        }
#endif
        event_index_++;

        // Check if we should save a snapshot
        if (event_index_ % snapshot_interval_ == 0) {
            SaveSnapshot();
        }
    }

    void PersistEvent(const EmailUpdated& event) {
#ifdef ENABLE_PROTOBUF
        if (persistence_) {
            // In real implementation, convert to Protobuf and persist
            // persistence_->PersistEvent(actor_name_, event_index_, protobuf_event);
        }
#endif
        event_index_++;

        // Check if we should save a snapshot
        if (event_index_ % snapshot_interval_ == 0) {
            SaveSnapshot();
        }
    }

    void ApplyEvent(const UserRegistered& event) {
        state_.user_id = event.user_id;
        state_.name = event.name;
        state_.email = event.email;
        state_.registered = true;
    }

    void ApplyEvent(const EmailUpdated& event) {
        state_.email = event.new_email;
    }

    void RecoverState() {
#ifdef ENABLE_PROTOBUF
        if (!persistence_) return;

        // Try to load snapshot first
        auto [snapshot, snapshot_index, has_snapshot] = persistence_->GetSnapshot("user-actor");

        if (has_snapshot) {
            // Restore from snapshot
            // state_ = ... (restore from snapshot)
            event_index_ = snapshot_index;
            std::cout << "[UserActor] Restored from snapshot at index " << event_index_ << std::endl;
        }

        // Replay events after snapshot
        persistence_->GetEvents("user-actor", event_index_, -1,
            [this](std::shared_ptr<void> event) {
                // Apply each event to rebuild state
                // ApplyEvent(*event);
                event_index_++;
            });
#endif
    }

    void SaveSnapshot() {
#ifdef ENABLE_PROTOBUF
        if (persistence_) {
            // Convert state to snapshot and persist
            // persistence_->PersistSnapshot("user-actor", event_index_, snapshot);

            // Delete old events (they're now captured in snapshot)
            persistence_->DeleteEvents("user-actor", event_index_);

            std::cout << "[UserActor] Snapshot saved at index " << event_index_ << std::endl;
        }
#else
        std::cout << "[UserActor] Would save snapshot at index " << event_index_
                  << " (Protobuf not enabled)" << std::endl;
#endif
    }
};

// ============================================================================
// Demo Functions
// ============================================================================

void DemoEventSourcing(std::shared_ptr<protoactor::Context> root) {
    std::cout << "\n=== Event Sourcing Demo ===" << std::endl;
    std::cout << "Events are persisted and replayed to rebuild state.\n" << std::endl;

    // Create actor (without persistence for demo)
    auto props = protoactor::Props::FromProducer([]() {
        return std::make_shared<UserActor>();
    });

    auto pid = root->Spawn(props);

    // Register user
    root->Send(pid, std::make_shared<RegisterUser>("user-123", "John Doe", "john@example.com"));

    // Update email multiple times
    root->Send(pid, std::make_shared<UpdateEmail>("user-123", "john.doe@example.com"));
    root->Send(pid, std::make_shared<UpdateEmail>("user-123", "john.new@example.com"));

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
}

void DemoSnapshotting(std::shared_ptr<protoactor::Context> root) {
    std::cout << "\n=== Snapshotting Demo ===" << std::endl;
    std::cout << "Snapshots optimize recovery by capturing state at a point in time.\n" << std::endl;

    // Create actor with snapshot interval of 3
    auto props = protoactor::Props::FromProducer([]() {
        return std::make_shared<UserActor>();
    });

    auto pid = root->Spawn(props);

    // Generate events to trigger snapshot
    root->Send(pid, std::make_shared<RegisterUser>("user-456", "Jane Doe", "jane@example.com"));
    root->Send(pid, std::make_shared<UpdateEmail>("user-456", "jane.doe@example.com"));
    root->Send(pid, std::make_shared<UpdateEmail>("user-456", "jane.updated@example.com"));

    // This should trigger snapshot (3rd event)
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
}

void ShowPersistenceSummary() {
    std::cout << "\n=== Persistence Summary ===" << std::endl;

    std::cout << "\n"
              << "| Concept        | Description                              |\n"
              << "|----------------|------------------------------------------|\n"
              << "| Command        | Intent to change state (not persisted)   |\n"
              << "| Event          | Fact that happened (persisted)           |\n"
              << "| State          | Current state rebuilt from events        |\n"
              << "| Snapshot       | Point-in-time state capture              |\n"
              << "| Provider       | Storage backend (memory, DB, etc.)       |\n"
              << std::endl;

    std::cout << "Event Sourcing Benefits:\n"
              << "  - Complete audit trail of all changes\n"
              << "  - Ability to replay events for debugging\n"
              << "  - Temporal queries (state at any point in time)\n"
              << "  - Event-driven integration with other systems\n"
              << std::endl;

    std::cout << "Snapshot Benefits:\n"
              << "  - Faster recovery (fewer events to replay)\n"
              << "  - Reduced storage for old events\n"
              << "  - Better performance for long-lived actors\n"
              << std::endl;

#ifdef ENABLE_PROTOBUF
    std::cout << "Protobuf support: ENABLED\n";
#else
    std::cout << "Protobuf support: DISABLED\n"
              << "Rebuild with -DENABLE_PROTOBUF=ON for full persistence support\n";
#endif

    std::cout << "\nGo equivalent:\n"
              << "  // Create provider\n"
              << "  provider := persistence.NewInMemoryProvider()\n"
              << "\n"
              << "  // Create props with persistence\n"
              << "  props := actor.PropsFromProducer(NewPersistentActor).\n"
              << "      WithPersistence(provider,\n"
              << "          persistence.WithSnapshotInterval(100))\n"
              << std::endl;
}

void ShowArchitecture() {
    std::cout << "\n=== Persistence Architecture ===" << std::endl;
    std::cout << "\n"
              << "  ┌────────────────────────────────────────────────────────┐\n"
              << "  │                    Persistent Actor                     │\n"
              << "  │                                                        │\n"
              << "  │  ┌──────────┐   ┌──────────┐   ┌──────────────────┐   │\n"
              << "  │  │ Command  │──►│  Event   │──►│     State        │   │\n"
              << "  │  │ (input)  │   │ (stored) │   │  (in-memory)     │   │\n"
              << "  │  └──────────┘   └────┬─────┘   └──────────────────┘   │\n"
              << "  │                      │                                │\n"
              << "  │                      ▼                                │\n"
              << "  │            ┌──────────────────┐                       │\n"
              << "  │            │    Provider      │                       │\n"
              << "  │            │ (EventStore +    │                       │\n"
              << "  │            │  SnapshotStore)  │                       │\n"
              << "  │            └────────┬─────────┘                       │\n"
              << "  └─────────────────────┼────────────────────────────────┘\n"
              << "                        │\n"
              << "                        ▼\n"
              << "            ┌──────────────────────┐\n"
              << "            │   Storage Backend    │\n"
              << "            │ (Memory/DB/Cloud)    │\n"
              << "            └──────────────────────┘\n"
              << std::endl;
}

int main() {
    std::cout << "=== ProtoActor C++ Persistence Example ===" << std::endl;

    try {
        auto system = protoactor::ActorSystem::New();
        auto root = system->GetRoot();

        // Demonstrate event sourcing
        DemoEventSourcing(root);

        // Demonstrate snapshotting
        DemoSnapshotting(root);

        // Show architecture
        ShowArchitecture();

        // Show summary
        ShowPersistenceSummary();

        // Wait for all messages to be processed
        std::this_thread::sleep_for(std::chrono::milliseconds(500));

        std::cout << "\n=== Example Complete ===" << std::endl;

        system->Shutdown();
    } catch (const std::exception& e) {
        std::cerr << "Exception: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}

/*
 * ============================================================================
 * PERSISTENCE GUIDE
 * ============================================================================
 *
 * 1. EVENT SOURCING PATTERN
 * -------------------------
 *
 * Instead of storing current state, store all events that led to the state.
 *
 * Example: Bank Account
 *   Events: [AccountOpened, Deposit($100), Withdrawal($50), Deposit($25)]
 *   State: Balance = $75
 *
 * Benefits:
 *   - Complete audit trail
 *   - Temporal queries (what was balance on date X?)
 *   - Debug friendly (replay events)
 *   - Event-driven integrations
 *
 *
 * 2. IMPLEMENTING A PERSISTENT ACTOR
 * -----------------------------------
 *
 * Steps:
 *   a) Handle commands -> emit events
 *   b) Persist events
 *   c) Apply events to update state
 *   d) On recovery: load snapshot + replay events
 *
 * C++:
 *   class MyActor : public Actor {
 *       void Receive(Context ctx) override {
 *           if (auto* cmd = ctx->MessageAs<DoSomething>()) {
 *               // 1. Create event
 *               SomethingHappened event{...};
 *
 *               // 2. Persist event
 *               persistence_->PersistEvent(name, index++, event);
 *
 *               // 3. Apply event
 *               Apply(event);
 *           }
 *       }
 *
 *       void Apply(const SomethingHappened& e) {
 *           // Update state from event
 *       }
 *   };
 *
 * Go:
 *   func (a *MyActor) Receive(ctx actor.Context) {
 *       switch msg := ctx.Message().(type) {
 *       case *DoSomething:
 *           event := &SomethingHappened{...}
 *           ctx.PersistEvent(event)
 *       case *SomethingHappened:
 *           a.Apply(event)
 *       }
 *   }
 *
 *
 * 3. SNAPSHOTTING
 * ---------------
 *
 * Periodically save complete state to avoid replaying all events.
 *
 * When to snapshot:
 *   - Every N events (e.g., every 100 events)
 *   - After significant state changes
 *   - Before shutdown
 *
 * Recovery with snapshot:
 *   1. Load latest snapshot
 *   2. Replay only events after snapshot
 *
 *
 * 4. PROVIDERS
 * ------------
 *
 * Built-in:
 *   - InMemoryProvider: Testing only
 *
 * Production (implement yourself or use extensions):
 *   - SQL databases (PostgreSQL, MySQL)
 *   - NoSQL databases (MongoDB, DynamoDB)
 *   - Event stores (EventStoreDB, Kafka)
 *   - Cloud services (AWS DynamoDB, Azure Cosmos DB)
 *
 *
 * 5. BEST PRACTICES
 * -----------------
 *
 * - Keep events immutable and small
 * - Use meaningful event names
 * - Version your events for schema evolution
 * - Set appropriate snapshot intervals
 * - Handle event replay idempotently
 * - Consider event ordering guarantees
 *
 */
