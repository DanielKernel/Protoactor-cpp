#ifndef PROTOACTOR_PERSISTENCE_PERSISTENCE_H
#define PROTOACTOR_PERSISTENCE_PERSISTENCE_H

#include <memory>
#include <string>
#include <functional>
#ifdef ENABLE_PROTOBUF
#include <google/protobuf/message.h>
#endif

namespace protoactor {
namespace persistence {

// Forward declarations
class ProviderState;

/**
 * @brief Snapshot store interface.
 */
class SnapshotStore {
public:
    virtual ~SnapshotStore() = default;
    
    /**
     * @brief Get a snapshot.
     * @param actor_name Actor name
     * @return Snapshot, event index, and success flag
     */
#ifdef ENABLE_PROTOBUF
    virtual std::tuple<std::shared_ptr<google::protobuf::Message>, int, bool>
        GetSnapshot(const std::string& actor_name) = 0;
#else
    virtual std::tuple<std::shared_ptr<void>, int, bool>
        GetSnapshot(const std::string& actor_name) = 0;
#endif
    
    /**
     * @brief Persist a snapshot.
     * @param actor_name Actor name
     * @param snapshot_index Snapshot index
     * @param snapshot Snapshot message
     */
#ifdef ENABLE_PROTOBUF
    virtual void PersistSnapshot(
        const std::string& actor_name,
        int snapshot_index,
        std::shared_ptr<google::protobuf::Message> snapshot) = 0;
#else
    virtual void PersistSnapshot(
        const std::string& actor_name,
        int snapshot_index,
        std::shared_ptr<void> snapshot) = 0;
#endif
    
    /**
     * @brief Delete snapshots.
     * @param actor_name Actor name
     * @param inclusive_to_index Inclusive end index
     */
    virtual void DeleteSnapshots(const std::string& actor_name, int inclusive_to_index) = 0;
};

/**
 * @brief Event store interface.
 */
class EventStore {
public:
    virtual ~EventStore() = default;
    
    /**
     * @brief Get events.
     * @param actor_name Actor name
     * @param event_index_start Start index
     * @param event_index_end End index
     * @param callback Callback for each event
     */
#ifdef ENABLE_PROTOBUF
    virtual void GetEvents(
        const std::string& actor_name,
        int event_index_start,
        int event_index_end,
        std::function<void(std::shared_ptr<google::protobuf::Message>)> callback) = 0;
#else
    virtual void GetEvents(
        const std::string& actor_name,
        int event_index_start,
        int event_index_end,
        std::function<void(std::shared_ptr<void>)> callback) = 0;
#endif
    
    /**
     * @brief Persist an event.
     * @param actor_name Actor name
     * @param event_index Event index
     * @param event Event message
     */
#ifdef ENABLE_PROTOBUF
    virtual void PersistEvent(
        const std::string& actor_name,
        int event_index,
        std::shared_ptr<google::protobuf::Message> event) = 0;
#else
    virtual void PersistEvent(
        const std::string& actor_name,
        int event_index,
        std::shared_ptr<void> event) = 0;
#endif
    
    /**
     * @brief Delete events.
     * @param actor_name Actor name
     * @param inclusive_to_index Inclusive end index
     */
    virtual void DeleteEvents(const std::string& actor_name, int inclusive_to_index) = 0;
};

/**
 * @brief Provider state interface.
 */
class ProviderState : public SnapshotStore, public EventStore {
public:
    virtual ~ProviderState() = default;
    
    /**
     * @brief Restart the provider.
     */
    virtual void Restart() = 0;
    
    /**
     * @brief Get the snapshot interval.
     * @return Interval
     */
    virtual int GetSnapshotInterval() const = 0;
};

/**
 * @brief Provider interface.
 */
class Provider {
public:
    virtual ~Provider() = default;
    
    /**
     * @brief Get the provider state.
     * @return Provider state
     */
    virtual std::shared_ptr<ProviderState> GetState() = 0;
};

/**
 * @brief In-memory persistence provider (for testing).
 */
class InMemoryProvider : public Provider {
public:
    std::shared_ptr<ProviderState> GetState() override;

private:
    std::shared_ptr<ProviderState> state_;
};

} // namespace persistence
} // namespace protoactor

#endif // PROTOACTOR_PERSISTENCE_PERSISTENCE_H
