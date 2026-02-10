#include "protoactor/future.h"
#include "protoactor/actor_system.h"
#include "protoactor/pid.h"
#include "protoactor/process_registry.h"
#include "protoactor/messages.h"
#include "protoactor/process.h"
#include "protoactor/new_pid.h"
#include "protoactor/deadletter.h"
#include <mutex>
#include <condition_variable>
#include <chrono>
#include <thread>

namespace protoactor {

// Future implementation
class FutureImpl : public Future, public Process, public std::enable_shared_from_this<FutureImpl> {
public:
    FutureImpl(std::shared_ptr<ActorSystem> actor_system, std::chrono::milliseconds timeout)
        : actor_system_(actor_system),
          done_(false),
          timeout_(timeout) {
        // Register as process
        auto id = actor_system_->GetProcessRegistry()->NextID();
        pid_ = NewPID(actor_system_->GetProcessRegistry()->Address(), "future" + id);
        auto self = std::static_pointer_cast<FutureImpl>(shared_from_this());
        actor_system_->GetProcessRegistry()->Add(self, "future" + id);
        
        // Start timeout timer if needed
        if (timeout.count() > 0) {
            timeout_thread_ = std::thread([this, timeout]() {
                std::this_thread::sleep_for(timeout);
                std::lock_guard<std::mutex> lock(mutex_);
                if (!done_) {
                    err_ = std::make_error_code(std::errc::timed_out);
                    done_ = true;
                    Stop(pid_);
                    cond_.notify_all();
                }
            });
        }
    }
    
    ~FutureImpl() {
        if (timeout_thread_.joinable()) {
            timeout_thread_.join();
        }
    }
    
    std::shared_ptr<PID> GetPID() override {
        return pid_;
    }
    
    void PipeTo(const std::vector<std::shared_ptr<PID>>& pids) override {
        std::lock_guard<std::mutex> lock(mutex_);
        pipes_.insert(pipes_.end(), pids.begin(), pids.end());
        if (done_) {
            SendToPipes();
        }
    }
    
    std::pair<std::shared_ptr<void>, std::error_code> Result() override {
        std::unique_lock<std::mutex> lock(mutex_);
        cond_.wait(lock, [this] { return done_; });
        return {result_, err_};
    }
    
    std::error_code Wait() override {
        std::unique_lock<std::mutex> lock(mutex_);
        cond_.wait(lock, [this] { return done_; });
        return err_;
    }
    
    void ContinueWith(std::function<void(std::shared_ptr<void>, std::error_code)> continuation) override {
        std::lock_guard<std::mutex> lock(mutex_);
        if (done_) {
            continuation(result_, err_);
        } else {
            completions_.push_back(continuation);
        }
    }
    
    // Process interface
    void SendUserMessage(std::shared_ptr<PID> pid, std::shared_ptr<void> message) override {
        std::lock_guard<std::mutex> lock(mutex_);
        if (done_) {
            return;
        }
        
        auto [header, msg, sender] = UnwrapEnvelope(message);
        
        // Check for dead letter response (would need proper type checking)
        // For now, just use the message as result
        if (msg) {
            result_ = nullptr;
            err_ = std::make_error_code(std::errc::host_unreachable);
        } else {
            result_ = msg;
            err_ = std::error_code(); // Success
        }
        
        Stop(pid);
    }
    
    void SendSystemMessage(std::shared_ptr<PID> pid, std::shared_ptr<void> message) override {
        std::lock_guard<std::mutex> lock(mutex_);
        if (done_) {
            return;
        }
        
        result_ = message;
        err_ = std::error_code(); // Success
        Stop(pid);
    }
    
    void Stop(std::shared_ptr<PID> pid) override {
        std::lock_guard<std::mutex> lock(mutex_);
        if (done_) {
            return;
        }
        
        done_ = true;
        actor_system_->GetProcessRegistry()->Remove(pid);
        
        SendToPipes();
        RunCompletions();
        
        cond_.notify_all();
    }

private:
    std::shared_ptr<ActorSystem> actor_system_;
    std::shared_ptr<PID> pid_;
    std::mutex mutex_;
    std::condition_variable cond_;
    bool done_;
    std::shared_ptr<void> result_;
    std::error_code err_;
    std::chrono::milliseconds timeout_;
    std::thread timeout_thread_;
    std::vector<std::shared_ptr<PID>> pipes_;
    std::vector<std::function<void(std::shared_ptr<void>, std::error_code)>> completions_;
    
    void SendToPipes() {
        if (pipes_.empty()) {
            return;
        }
        
        std::shared_ptr<void> msg;
        if (err_) {
            msg = std::make_shared<std::error_code>(err_);
        } else {
            msg = result_;
        }
        
        for (auto& pid : pipes_) {
            pid->SendUserMessage(actor_system_, msg);
        }
        
        pipes_.clear();
    }
    
    void RunCompletions() {
        for (auto& completion : completions_) {
            completion(result_, err_);
        }
        completions_.clear();
    }
};

std::shared_ptr<Future> NewFuture(
    std::shared_ptr<ActorSystem> actor_system,
    std::chrono::milliseconds timeout) {
    return std::make_shared<FutureImpl>(actor_system, timeout);
}

} // namespace protoactor
