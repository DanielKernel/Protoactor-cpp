#include "internal/scheduler/timer.h"
#include "external/context.h"
#include "external/pid.h"
#include <thread>
#include <chrono>
#include <atomic>
#include <functional>

namespace protoactor {
namespace scheduler {

TimerScheduler::TimerScheduler(std::shared_ptr<Context> sender)
    : ctx_(sender) {
}

std::shared_ptr<TimerScheduler> TimerScheduler::New(std::shared_ptr<Context> sender) {
    return std::make_shared<TimerScheduler>(sender);
}

CancelFunc TimerScheduler::SendOnce(
    std::chrono::milliseconds delay,
    std::shared_ptr<PID> pid,
    std::shared_ptr<void> message) {
    
    auto cancelled = std::make_shared<std::atomic<bool>>(false);
    auto timer_thread = std::make_shared<std::thread>([this, delay, pid, message, cancelled]() {
        std::this_thread::sleep_for(delay);
        if (!cancelled->load()) {
            ctx_->Send(pid, message);
        }
    });
    
    return [timer_thread, cancelled]() {
        cancelled->store(true);
        if (timer_thread->joinable()) {
            timer_thread->join();
        }
    };
}

CancelFunc TimerScheduler::SendRepeatedly(
    std::chrono::milliseconds initial,
    std::chrono::milliseconds interval,
    std::shared_ptr<PID> pid,
    std::shared_ptr<void> message) {
    
    auto cancelled = std::make_shared<std::atomic<bool>>(false);
    auto state = std::make_shared<std::atomic<int>>(0); // 0=init, 1=ready, 2=done
    
    auto timer_thread = std::make_shared<std::thread>([this, initial, interval, pid, message, cancelled, state]() {
        // Wait for initialization
        while (state->load() == 0) {
            std::this_thread::yield();
        }
        
        if (state->load() == 2) {
            return; // Cancelled before ready
        }
        
        std::this_thread::sleep_for(initial);
        
        while (!cancelled->load()) {
            if (state->load() == 2) {
                return;
            }
            ctx_->Send(pid, message);
            std::this_thread::sleep_for(interval);
        }
    });
    
    state->store(1); // Ready
    
    return [timer_thread, cancelled, state]() {
        if (state->exchange(2) != 2) {
            cancelled->store(true);
            if (timer_thread->joinable()) {
                timer_thread->join();
            }
        }
    };
}

CancelFunc TimerScheduler::RequestOnce(
    std::chrono::milliseconds delay,
    std::shared_ptr<PID> pid,
    std::shared_ptr<void> message) {
    
    auto cancelled = std::make_shared<std::atomic<bool>>(false);
    auto timer_thread = std::make_shared<std::thread>([this, delay, pid, message, cancelled]() {
        std::this_thread::sleep_for(delay);
        if (!cancelled->load()) {
            ctx_->Request(pid, message);
        }
    });
    
    return [timer_thread, cancelled]() {
        cancelled->store(true);
        if (timer_thread->joinable()) {
            timer_thread->join();
        }
    };
}

CancelFunc TimerScheduler::RequestRepeatedly(
    std::chrono::milliseconds delay,
    std::chrono::milliseconds interval,
    std::shared_ptr<PID> pid,
    std::shared_ptr<void> message) {
    
    auto cancelled = std::make_shared<std::atomic<bool>>(false);
    auto state = std::make_shared<std::atomic<int>>(0);
    
    auto timer_thread = std::make_shared<std::thread>([this, delay, interval, pid, message, cancelled, state]() {
        while (state->load() == 0) {
            std::this_thread::yield();
        }
        
        if (state->load() == 2) {
            return;
        }
        
        std::this_thread::sleep_for(delay);
        
        while (!cancelled->load()) {
            if (state->load() == 2) {
                return;
            }
            ctx_->Request(pid, message);
            std::this_thread::sleep_for(interval);
        }
    });
    
    state->store(1);
    
    return [timer_thread, cancelled, state]() {
        if (state->exchange(2) != 2) {
            cancelled->store(true);
            if (timer_thread->joinable()) {
                timer_thread->join();
            }
        }
    };
}

} // namespace scheduler
} // namespace protoactor
