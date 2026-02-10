#include "protoactor/dispatcher.h"
#include <thread>
#include <functional>
#include <iostream>

namespace protoactor {

// Default dispatcher implementation
class DefaultDispatcherImpl : public Dispatcher {
public:
    explicit DefaultDispatcherImpl(int throughput) : throughput_(throughput) {}
    
    void Schedule(std::function<void()> fn) override {
        // Create a thread and detach it
        // Note: In production, we would use a thread pool
        std::thread([fn]() {
            try {
                fn();
            } catch (const std::exception& e) {
                // Log exception but don't crash
                std::cerr << "Dispatcher thread exception: " << e.what() << std::endl;
            } catch (...) {
                // Swallow other exceptions to prevent thread crash
                std::cerr << "Dispatcher thread unknown exception" << std::endl;
            }
        }).detach();
    }
    
    int Throughput() const override {
        return throughput_;
    }

private:
    int throughput_;
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

std::shared_ptr<Dispatcher> NewDefaultDispatcher(int throughput) {
    return std::make_shared<DefaultDispatcherImpl>(throughput);
}

std::shared_ptr<Dispatcher> NewSynchronizedDispatcher(int throughput) {
    return std::make_shared<SynchronizedDispatcherImpl>(throughput);
}

} // namespace protoactor
