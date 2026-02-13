#include "protoactor/thread_pool.h"
#include "protoactor/platform.h"
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <algorithm>
#include <iostream>
#include <cstdlib>
#include <cstring>

namespace protoactor {

class ThreadPool::Impl {
public:
    explicit Impl(std::size_t num_threads) : stop_(false), stop_now_(false) {
        if (num_threads == 0) {
            num_threads = static_cast<std::size_t>(std::max(1, platform::GetCPUCount()));
        }
        workers_.reserve(num_threads);
        for (std::size_t i = 0; i < num_threads; ++i) {
            workers_.emplace_back(&Impl::WorkerLoop, this);
        }
    }

    ~Impl() {
        Shutdown();
    }

    void Submit(std::function<void()> task) {
        if (!task) return;
        std::lock_guard<std::mutex> lock(mutex_);
        if (stop_ || stop_now_) return;
        queue_.push(std::move(task));
        cv_.notify_one();
    }

    void Shutdown() {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            if (stop_) return;
            stop_ = true;
            cv_.notify_all();
        }
        for (std::thread& t : workers_) {
            if (t.joinable()) t.join();
        }
    }

    void ShutdownNow() {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            stop_now_ = true;
            stop_ = true;
            while (!queue_.empty()) queue_.pop();
            cv_.notify_all();
        }
        for (std::thread& t : workers_) {
            if (t.joinable()) t.join();
        }
    }

    std::size_t NumWorkers() const {
        return workers_.size();
    }

    std::size_t PendingCount() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return queue_.size();
    }

    bool IsShutdown() const {
        return stop_.load(std::memory_order_acquire);
    }

private:
    void WorkerLoop() {
        for (;;) {
            std::function<void()> task;
            {
                std::unique_lock<std::mutex> lock(mutex_);
                cv_.wait(lock, [this] {
                    return stop_.load(std::memory_order_acquire) || !queue_.empty();
                });
                if (stop_.load(std::memory_order_acquire) && queue_.empty()) {
                    return;
                }
                if (!queue_.empty()) {
                    task = std::move(queue_.front());
                    queue_.pop();
                }
            }
            if (task) {
                try {
                    task();
                } catch (const std::exception& e) {
                    std::cerr << "ThreadPool task exception: " << e.what() << std::endl;
                } catch (...) {
                    std::cerr << "ThreadPool task unknown exception" << std::endl;
                }
            }
        }
    }

    mutable std::mutex mutex_;
    std::condition_variable cv_;
    std::queue<std::function<void()>> queue_;
    std::atomic<bool> stop_;
    std::atomic<bool> stop_now_;
    std::vector<std::thread> workers_;
};

ThreadPool::ThreadPool(std::size_t num_threads) : impl_(new Impl(num_threads)) {}

ThreadPool::~ThreadPool() = default;

void ThreadPool::Submit(std::function<void()> task) {
    impl_->Submit(std::move(task));
}

void ThreadPool::Shutdown() {
    impl_->Shutdown();
}

void ThreadPool::ShutdownNow() {
    impl_->ShutdownNow();
}

std::size_t ThreadPool::NumWorkers() const {
    return impl_->NumWorkers();
}

std::size_t ThreadPool::PendingCount() const {
    return impl_->PendingCount();
}

bool ThreadPool::IsShutdown() const {
    return impl_->IsShutdown();
}

std::shared_ptr<ThreadPool> DefaultThreadPool() {
    static std::shared_ptr<ThreadPool> pool = []() {
        std::shared_ptr<ThreadPool> p = std::make_shared<ThreadPool>(0);
        if (std::getenv("PROTOACTOR_TEST") == nullptr) {
            std::atexit([]() { DefaultThreadPool()->Shutdown(); });
        }
        return p;
    }();
    return pool;
}

std::shared_ptr<ThreadPool> NewThreadPool(std::size_t num_threads) {
    return std::make_shared<ThreadPool>(num_threads);
}

} // namespace protoactor
