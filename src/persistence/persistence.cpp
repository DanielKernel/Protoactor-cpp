#include "external/persistence.h"
#ifdef ENABLE_PROTOBUF
#include <google/protobuf/message.h>
#endif
#include <unordered_map>
#include <vector>
#include <mutex>

namespace protoactor {
namespace persistence {

// InMemoryProviderState implementation
class InMemoryProviderStateImpl : public ProviderState {
public:
    void Restart() override {
        // Nothing to do for in-memory
    }
    
    int GetSnapshotInterval() const override {
        return snapshot_interval_;
    }
    
#ifdef ENABLE_PROTOBUF
    std::tuple<std::shared_ptr<google::protobuf::Message>, int, bool>
        GetSnapshot(const std::string& actor_name) override {
#else
    std::tuple<std::shared_ptr<void>, int, bool>
        GetSnapshot(const std::string& actor_name) override {
#endif
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = snapshots_.find(actor_name);
        if (it != snapshots_.end()) {
            return {it->second.snapshot, it->second.index, true};
        }
        return {nullptr, 0, false};
    }
    
#ifdef ENABLE_PROTOBUF
    void PersistSnapshot(
        const std::string& actor_name,
        int snapshot_index,
        std::shared_ptr<google::protobuf::Message> snapshot) override {
#else
    void PersistSnapshot(
        const std::string& actor_name,
        int snapshot_index,
        std::shared_ptr<void> snapshot) override {
#endif
        std::lock_guard<std::mutex> lock(mutex_);
        snapshots_[actor_name] = {snapshot, snapshot_index};
    }
    
    void DeleteSnapshots(const std::string& actor_name, int inclusive_to_index) override {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = snapshots_.find(actor_name);
        if (it != snapshots_.end() && it->second.index <= inclusive_to_index) {
            snapshots_.erase(it);
        }
    }
    
#ifdef ENABLE_PROTOBUF
    void GetEvents(
        const std::string& actor_name,
        int event_index_start,
        int event_index_end,
        std::function<void(std::shared_ptr<google::protobuf::Message>)> callback) override {
#else
    void GetEvents(
        const std::string& actor_name,
        int event_index_start,
        int event_index_end,
        std::function<void(std::shared_ptr<void>)> callback) override {
#endif
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = events_.find(actor_name);
        if (it != events_.end()) {
            for (int i = event_index_start; i < event_index_end && i < static_cast<int>(it->second.size()); ++i) {
                if (i >= 0) {
                    callback(it->second[i]);
                }
            }
        }
    }
    
#ifdef ENABLE_PROTOBUF
    void PersistEvent(
        const std::string& actor_name,
        int event_index,
        std::shared_ptr<google::protobuf::Message> event) override {
#else
    void PersistEvent(
        const std::string& actor_name,
        int event_index,
        std::shared_ptr<void> event) override {
#endif
        std::lock_guard<std::mutex> lock(mutex_);
        auto& event_list = events_[actor_name];
        while (static_cast<int>(event_list.size()) <= event_index) {
            event_list.push_back(nullptr);
        }
        event_list[event_index] = event;
    }
    
    void DeleteEvents(const std::string& actor_name, int inclusive_to_index) override {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = events_.find(actor_name);
        if (it != events_.end()) {
            if (inclusive_to_index < static_cast<int>(it->second.size())) {
                it->second.erase(it->second.begin(), it->second.begin() + inclusive_to_index + 1);
            }
        }
    }

private:
    struct SnapshotData {
#ifdef ENABLE_PROTOBUF
        std::shared_ptr<google::protobuf::Message> snapshot;
#else
        std::shared_ptr<void> snapshot;
#endif
        int index;
    };
    
    mutable std::mutex mutex_;
    std::unordered_map<std::string, SnapshotData> snapshots_;
#ifdef ENABLE_PROTOBUF
    std::unordered_map<std::string, std::vector<std::shared_ptr<google::protobuf::Message>>> events_;
#else
    std::unordered_map<std::string, std::vector<std::shared_ptr<void>>> events_;
#endif
    int snapshot_interval_ = 100;
};

// InMemoryProvider implementation
std::shared_ptr<ProviderState> InMemoryProvider::GetState() {
    if (!state_) {
        state_ = std::make_shared<InMemoryProviderStateImpl>();
    }
    return state_;
}

} // namespace persistence
} // namespace protoactor
