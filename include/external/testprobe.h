#ifndef PROTOACTOR_TESTKIT_TESTPROBE_H
#define PROTOACTOR_TESTKIT_TESTPROBE_H

#include "pid.h"
#include "context.h"
#include "future.h"
#include <memory>
#include <vector>
#include <chrono>
#include <queue>
#include <thread>
#include <mutex>
#include <functional>

namespace protoactor {
namespace testkit {

/**
 * @brief Test probe for testing actors.
 * 
 * TestProbe allows testing actors by intercepting messages sent to it.
 */
class TestProbe {
public:
    /**
     * @brief Create a new test probe.
     * @param context Actor context
     * @return Test probe instance
     */
    static std::shared_ptr<TestProbe> New(std::shared_ptr<Context> context);
    
    /**
     * @brief Get the PID of the probe.
     * @return Probe PID
     */
    std::shared_ptr<PID> GetPID() const;
    
    /**
     * @brief Send a message to the probe.
     * @param message Message to send
     */
    void Send(std::shared_ptr<void> message);
    
    /**
     * @brief Request a message and wait for response.
     * @param message Request message
     * @param timeout Timeout duration
     * @return Response message
     */
    std::shared_ptr<void> Request(std::shared_ptr<void> message, std::chrono::milliseconds timeout = std::chrono::milliseconds(5000));
    
    /**
     * @brief Expect a message of a specific type.
     * @param timeout Timeout duration
     * @return Received message
     */
    template<typename T>
    T ExpectMsg(std::chrono::milliseconds timeout = std::chrono::milliseconds(5000)) {
        auto start = std::chrono::steady_clock::now();
        
        while (true) {
            std::unique_lock<std::mutex> lock(messages_mutex_);
            
            // Check if we have messages
            if (!messages_.empty()) {
                auto msg = messages_.front();
                messages_.pop();
                
                // Note: Type casting from std::shared_ptr<void> is unsafe
                // In full implementation, we would use type information or type registry
                // For now, we assume msg actually contains a std::shared_ptr<T>
                // This is a limitation: we cannot safely cast from void* to T
                // In a full implementation, we would store type information with the message
                if (msg) {
                    // Unsafe cast: assume msg is actually a std::shared_ptr<T>
                    // This will only work if the message was originally stored as shared_ptr<T>
                    // In full implementation, we would use type registry or type erasure with type info
                    // Note: We cannot use static_pointer_cast from shared_ptr<void> to shared_ptr<T>
                    // Workaround: cast the raw pointer (shared_ptr<void> to shared_ptr<T>)
                    // This is unsafe but necessary for this template to work
                    // In full implementation, we would use type registry
                    // Cast void* to T* (unsafe but necessary)
                    void* void_ptr = msg.get();
                    T* raw_ptr = static_cast<T*>(void_ptr);
                    if (raw_ptr) {
                        // Use aliasing constructor to create shared_ptr<T> without ownership transfer
                        std::shared_ptr<T> typed_ptr(msg, raw_ptr);
                        return *typed_ptr;
                    }
                }
            }
            
            // Check timeout
            auto elapsed = std::chrono::steady_clock::now() - start;
            if (elapsed >= timeout) {
                throw std::runtime_error("Timeout waiting for message");
            }
            
            // Wait a bit before checking again
            lock.unlock();
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }
    
    /**
     * @brief Expect a message matching a predicate.
     * @param predicate Predicate function
     * @param timeout Timeout duration
     * @return Received message
     */
    std::shared_ptr<void> ExpectMsg(std::function<bool(std::shared_ptr<void>)> predicate, std::chrono::milliseconds timeout = std::chrono::milliseconds(5000));
    
    /**
     * @brief Expect no message within timeout.
     * @param timeout Timeout duration
     */
    void ExpectNoMsg(std::chrono::milliseconds timeout = std::chrono::milliseconds(100));
    
    /**
     * @brief Get all received messages.
     * @return Vector of messages
     */
    std::vector<std::shared_ptr<void>> GetAllMessages() const;
    
    /**
     * @brief Clear all received messages.
     */
    void Clear();

private:
    std::shared_ptr<PID> pid_;
    std::shared_ptr<Context> context_;
    std::queue<std::shared_ptr<void>> messages_;
    mutable std::mutex messages_mutex_;
    
public:
    TestProbe(std::shared_ptr<Context> context);
    void OnMessage(std::shared_ptr<void> message);
};

/**
 * @brief Await utility for waiting on conditions.
 */
class Await {
public:
    /**
     * @brief Wait for a condition to become true.
     * @param condition Condition function
     * @param timeout Timeout duration
     * @return True if condition met, false if timeout
     */
    static bool Condition(std::function<bool()> condition, std::chrono::milliseconds timeout = std::chrono::milliseconds(5000));
    
    /**
     * @brief Wait for a future to complete.
     * @param future Future to wait for
     * @param timeout Timeout duration
     * @return Result and error code
     */
    static std::pair<std::shared_ptr<void>, std::error_code> Future(std::shared_ptr<protoactor::Future> future, std::chrono::milliseconds timeout = std::chrono::milliseconds(5000));
};

} // namespace testkit
} // namespace protoactor

#endif // PROTOACTOR_TESTKIT_TESTPROBE_H
