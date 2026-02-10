#include "protoactor/testkit/testprobe.h"
#include "protoactor/actor.h"
#include "protoactor/props.h"
#include "protoactor/messages.h"
#include <thread>
#include <chrono>
#include <mutex>
#include <condition_variable>

namespace protoactor {
namespace testkit {

// Test probe actor
class TestProbeActor : public Actor {
public:
    TestProbeActor(std::shared_ptr<TestProbe> probe)
        : probe_(probe) {
    }
    
    void Receive(std::shared_ptr<Context> context) override {
        auto msg = context->Message();
        if (!msg) {
            return;
        }
        
        // Try Started (it's a SystemMessage)
        // Note: We cannot use dynamic_pointer_cast on std::shared_ptr<void>
        // We use static_pointer_cast assuming the message is a SystemMessage if it's a system message
        // In full implementation, we would use type information
        auto sys_msg = std::static_pointer_cast<SystemMessage>(msg);
        if (sys_msg) {
            auto started = std::dynamic_pointer_cast<Started>(sys_msg);
            if (started) {
                // Ignore Started message
                return;
            }
        }
        
        // Forward message to probe
        if (auto probe = probe_.lock()) {
            probe->OnMessage(msg);
        }
    }

private:
    std::weak_ptr<TestProbe> probe_;
};

TestProbe::TestProbe(std::shared_ptr<Context> context) : context_(context) {
    // Note: This will be called from New(), so shared_from_this() is safe
    // But we need to use a workaround since we're in constructor
    auto self = std::shared_ptr<TestProbe>(this, [](TestProbe*) {});
    auto props = Props::FromProducer([self]() {
        return std::make_shared<TestProbeActor>(self);
    });
    pid_ = context->Spawn(props);
}

std::shared_ptr<TestProbe> TestProbe::New(std::shared_ptr<Context> context) {
    return std::make_shared<TestProbe>(context);
}

std::shared_ptr<PID> TestProbe::GetPID() const {
    return pid_;
}

void TestProbe::Send(std::shared_ptr<void> message) {
    if (pid_ && context_) {
        context_->Send(pid_, message);
    }
}

std::shared_ptr<void> TestProbe::Request(std::shared_ptr<void> message, std::chrono::milliseconds timeout) {
    if (pid_ && context_) {
        auto future = context_->RequestFuture(pid_, message, timeout);
        auto [result, err] = future->Result();
        if (err) {
            throw std::runtime_error("Request failed: " + err.message());
        }
        return result;
    }
    throw std::runtime_error("TestProbe not initialized");
}

// Template implementation moved to header file

std::shared_ptr<void> TestProbe::ExpectMsg(std::function<bool(std::shared_ptr<void>)> predicate, std::chrono::milliseconds timeout) {
    auto start = std::chrono::steady_clock::now();
    
    while (true) {
        std::unique_lock<std::mutex> lock(messages_mutex_);
        
        // Check if we have messages matching predicate
        if (!messages_.empty()) {
            auto msg = messages_.front();
            if (predicate(msg)) {
                messages_.pop();
                return msg;
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

void TestProbe::ExpectNoMsg(std::chrono::milliseconds timeout) {
    std::this_thread::sleep_for(timeout);
    
    std::lock_guard<std::mutex> lock(messages_mutex_);
    if (!messages_.empty()) {
        throw std::runtime_error("Unexpected message received");
    }
}

std::vector<std::shared_ptr<void>> TestProbe::GetAllMessages() const {
    std::lock_guard<std::mutex> lock(messages_mutex_);
    std::vector<std::shared_ptr<void>> result;
    
    auto temp = messages_;
    while (!temp.empty()) {
        result.push_back(temp.front());
        temp.pop();
    }
    
    return result;
}

void TestProbe::Clear() {
    std::lock_guard<std::mutex> lock(messages_mutex_);
    while (!messages_.empty()) {
        messages_.pop();
    }
}

void TestProbe::OnMessage(std::shared_ptr<void> message) {
    std::lock_guard<std::mutex> lock(messages_mutex_);
    messages_.push(message);
}

// Await implementation
bool Await::Condition(std::function<bool()> condition, std::chrono::milliseconds timeout) {
    auto start = std::chrono::steady_clock::now();
    
    while (true) {
        if (condition()) {
            return true;
        }
        
        auto elapsed = std::chrono::steady_clock::now() - start;
        if (elapsed >= timeout) {
            return false;
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

std::pair<std::shared_ptr<void>, std::error_code> Await::Future(
    std::shared_ptr<protoactor::Future> future,
    std::chrono::milliseconds timeout) {
    return future->Result();
}

} // namespace testkit
} // namespace protoactor
