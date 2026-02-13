#include "protoactor/dispatcher.h"
#include "protoactor/thread_pool.h"
#include <functional>
#include <iostream>

namespace protoactor {

// Default dispatcher implementation: schedules work onto a thread pool
class DefaultDispatcherImpl : public Dispatcher {
public:
    DefaultDispatcherImpl(int throughput, std::shared_ptr<ThreadPool> pool)
        : throughput_(throughput),
          pool_(pool ? pool : DefaultThreadPool()) {}

    void Schedule(std::function<void()> fn) override {
        if (!pool_ || pool_->IsShutdown()) return;
        pool_->Submit([fn]() {
            try {
                fn();
            } catch (const std::exception& e) {
                std::cerr << "Dispatcher task exception: " << e.what() << std::endl;
            } catch (...) {
                std::cerr << "Dispatcher task unknown exception" << std::endl;
            }
        });
    }

    int Throughput() const override {
        return throughput_;
    }

private:
    int throughput_;
    std::shared_ptr<ThreadPool> pool_;
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
