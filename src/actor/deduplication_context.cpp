#include "internal/actor/deduplication_context.h"
#include "internal/log.h"
#include <chrono>
#include <mutex>

namespace protoactor {

/**
 * @brief Internal deduplication state.
 */
class DedupState {
public:
    DedupState(Deduplicator dedup, std::chrono::milliseconds ttl)
        : dedup_(dedup), ttl_(ttl) {
    }
    
    bool ShouldProcess(std::shared_ptr<MessageEnvelope> envelope) {
        if (!envelope || !envelope->message) {
            return true;
        }
        
        auto now = std::chrono::steady_clock::now();
        Cleanup(now);
        
        std::string key = dedup_(envelope->message);
        if (key.empty()) {
            return true;
        }
        
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = seen_.find(key);
        if (it != seen_.end()) {
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - it->second);
            if (elapsed < ttl_) {
                // Refresh timestamp and drop duplicate
                seen_[key] = now;
                protoactor::log::GetLogger("deduplication")->debug("Dropped duplicate message");
                return false;
            }
        }
        seen_[key] = now;
        return true;
    }
    
private:
    Deduplicator dedup_;
    std::chrono::milliseconds ttl_;
    std::mutex mutex_;
    std::unordered_map<std::string, std::chrono::steady_clock::time_point> seen_;
    
    void Cleanup(std::chrono::steady_clock::time_point now) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = seen_.begin();
        while (it != seen_.end()) {
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - it->second);
            if (elapsed >= ttl_) {
                it = seen_.erase(it);
            } else {
                ++it;
            }
        }
    }
};

ReceiverMiddleware DeduplicationContext(Deduplicator fn, std::chrono::milliseconds ttl) {
    // Create shared state for this middleware instance
    auto state = std::make_shared<DedupState>(fn, ttl);
    
    return [state](std::function<void(std::shared_ptr<Context>, std::shared_ptr<MessageEnvelope>)> next) {
        return [state, next](std::shared_ptr<Context> ctx, std::shared_ptr<MessageEnvelope> envelope) {
            if (state->ShouldProcess(envelope)) {
                next(ctx, envelope);
            }
            // Otherwise, drop the duplicate message
        };
    };
}

} // namespace protoactor
