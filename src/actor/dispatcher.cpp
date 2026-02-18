#include "external/dispatcher.h"
#include "internal/thread_pool.h"
#include <functional>
#include <iostream>

namespace protoactor {

// Default dispatcher implementation: schedules work onto a thread pool
class DefaultDispatcherImpl : public Dispatcher {
public:
    DefaultDispatcherImpl(int throughput, std::shared_ptr<ThreadPool> pool)
        : throughput_(throughput) {
        // Use weak_ptr to avoid holding a strong reference to the default thread pool
        // This breaks the cycle of shared_ptr references and prevents destruction order issues
        if (pool) {
            weak_pool_ = pool;
        } else {
            weak_pool_ = DefaultThreadPool();
        }
    }

    void Schedule(std::function<void()> fn) override {
        // Lock the weak_ptr to get a shared_ptr, then submit the task
        if (auto pool = weak_pool_.lock()) {
            if (!pool->IsShutdown()) {
                pool->Submit([fn]() {
                    try {
                        fn();
                    } catch (const std::exception& e) {
                        std::cerr << "Dispatcher task exception: " << e.what() << std::endl;
                    } catch (...) {
                        std::cerr << "Dispatcher task unknown exception" << std::endl;
                    }
                });
            }
        }
    }

    int Throughput() const override {
        return throughput_;
    }

private:
    int throughput_;
    std::weak_ptr<ThreadPool> weak_pool_;
};

// Synchronized dispatcher implementation
class SynchronizedDispatcherImpl : public Dispatcher {
public:
    explicit SynchronizedDispatcherImpl(int throughput) : throughput_(throughput) {}

    void Schedule(std::function<void()> fn) override {
        fn(); // Execute synchronously
    }

    int Throughput() const override {
        return throughput_;
    }

private:
    int throughput_;
};

std::shared_ptr<Dispatcher> NewDefaultDispatcher(int throughput, std::shared_ptr<ThreadPool> pool) {
    return std::make_shared<DefaultDispatcherImpl>(throughput, pool);
}

std::shared_ptr<Dispatcher> NewSynchronizedDispatcher(int throughput) {
    return std::make_shared<SynchronizedDispatcherImpl>(throughput);
}

} // namespace protoactor
