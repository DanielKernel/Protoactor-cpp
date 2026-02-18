#include "internal/mailbox.h"
#include "internal/queue.h"
#include "external/dispatcher.h"
#include "external/messages.h"
#include "internal/actor/actor_context.h"
#include <atomic>
#include <thread>

namespace protoactor {

// Forward declarations
class MessageInvoker;

// Default mailbox implementation
class DefaultMailbox : public Mailbox {
public:
    DefaultMailbox()
        : scheduler_status_(IDLE),
          user_messages_(0),
          sys_messages_(0),
          suspended_(0) {
        user_mailbox_ = NewUnboundedQueue();
        system_mailbox_ = NewMPSCQueue();
    }
    
    void PostUserMessage(std::shared_ptr<void> message) override {
        // Normal message - batch handling would be implemented if MessageBatch is defined
        user_mailbox_->Push(message);
        user_messages_.fetch_add(1, std::memory_order_relaxed);
        // Always try to schedule, even if already scheduled
        // This ensures messages get processed even if the previous schedule hasn't started yet
        Schedule();
    }
    
    void PostSystemMessage(std::shared_ptr<void> message) override {
        system_mailbox_->Push(message);
        sys_messages_.fetch_add(1, std::memory_order_relaxed);
        Schedule();
    }
    
    void RegisterHandlers(
        std::shared_ptr<void> invoker_ptr,
        std::shared_ptr<Dispatcher> dispatcher) override {
        invoker_ptr_ = invoker_ptr;
        dispatcher_ = dispatcher;
    }
    
    void Start() override {
        // Mailbox started
    }
    
    int UserMessageCount() const override {
        return user_messages_.load(std::memory_order_relaxed);
    }

private:
    enum {
        IDLE = 0,
        RUNNING = 1
    };
    
    std::shared_ptr<Queue> user_mailbox_;
    std::shared_ptr<MPSCQueue> system_mailbox_;
    std::atomic<int> scheduler_status_;
    std::atomic<int> user_messages_;
    std::atomic<int> sys_messages_;
    std::atomic<int> suspended_;
    std::shared_ptr<void> invoker_ptr_;  // ActorContext as void*
    std::shared_ptr<Dispatcher> dispatcher_;
    
    void Schedule() {
        // Try to set status to RUNNING
        int expected = IDLE;
        if (scheduler_status_.compare_exchange_strong(expected, RUNNING)) {
            // Successfully acquired lock, schedule processing
            dispatcher_->Schedule([this]() {
                ProcessMessages();
            });
        }
        // If already RUNNING, don't schedule again
        // The current processing will handle any new messages
    }
    
    void ProcessMessages() {
        // Process messages in a loop to avoid deep recursion
        // This is especially important for synchronized dispatcher
        int max_rounds = 1000; // Limit processing rounds to prevent infinite loop
        int round = 0;
        
        while (round < max_rounds) {
            // Process one iteration of messages
            bool processed = ProcessLoop();
            
            // Set mailbox to idle
            scheduler_status_.store(IDLE, std::memory_order_release);
            
            // Re-check message counts after processing
            int sys = sys_messages_.load(std::memory_order_relaxed);
            int user = user_messages_.load(std::memory_order_relaxed);
            int suspended = suspended_.load(std::memory_order_relaxed);
            
            // Check if there are still messages to process
            if (sys > 0 || (suspended == 0 && user > 0)) {
                // Try setting the mailbox back to running
                int expected = IDLE;
                if (scheduler_status_.compare_exchange_strong(expected, RUNNING)) {
                    round++;
                    continue; // Process more messages in same call
                } else {
                    // Another thread took over, exit - they'll handle remaining messages
                    break;
                }
            } else {
                // No more messages, we're done
                break;
            }
        }
    }
    
    bool ProcessLoop() {
        int throughput = dispatcher_->Throughput();
        int i = 0;
        int max_iterations = throughput * 10; // Limit iterations to prevent infinite loop
        bool processed_any = false;
        
        while (i < max_iterations) {
            if (i > throughput) {
                i = 0;
                std::this_thread::yield();
            }
            i++;
            
            // Process system messages first
            auto sys_msg = system_mailbox_->Pop();
            if (sys_msg) {
                sys_messages_.fetch_sub(1, std::memory_order_relaxed);
                processed_any = true;
                
                if (invoker_ptr_) {
                    // Cast to ActorContext and call InvokeSystemMessage
                    // This is safe because we know it's an ActorContext
                    auto ctx = std::static_pointer_cast<ActorContext>(invoker_ptr_);
                    if (ctx) {
                        ctx->InvokeSystemMessage(sys_msg);
                    }
                }
                continue;
            }
            
            // Check if suspended
            if (suspended_.load(std::memory_order_relaxed) == 1) {
                return processed_any;
            }
            
            // Process user messages
            auto user_msg = user_mailbox_->Pop();
            if (user_msg) {
                user_messages_.fetch_sub(1, std::memory_order_relaxed);
                processed_any = true;
                if (invoker_ptr_) {
                    auto ctx = std::static_pointer_cast<ActorContext>(invoker_ptr_);
                    if (ctx) {
                        ctx->InvokeUserMessage(user_msg);
                    }
                }
            } else {
                // No more messages in this iteration
                return processed_any;
            }
        }
        return processed_any;
    }
};

// MessageInvoker is now ActorContext - no separate interface needed

MailboxProducer Unbounded() {
    return []() -> std::shared_ptr<Mailbox> {
        return std::make_shared<DefaultMailbox>();
    };
}

MailboxProducer Bounded(int size) {
    // Bounded implementation would be similar but with size limit
    return Unbounded(); // Simplified for now
}

} // namespace protoactor
