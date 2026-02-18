#ifndef PROTOACTOR_THREAD_POOL_H
#define PROTOACTOR_THREAD_POOL_H

#include <functional>
#include <memory>
#include <cstddef>

namespace protoactor {

/**
 * @brief Production-grade thread pool for multi-threaded task scheduling.
 *
 * Features:
 * - Fixed number of worker threads (configurable, default: CPU count).
 * - Thread-safe unbounded task queue.
 * - Exception isolation: task exceptions do not terminate workers.
 * - Graceful shutdown: drains pending tasks then joins workers.
 * - Process exit: default pool (DefaultThreadPool()) is shut down via atexit.
 * - Unbounded queue: Submit() never blocks; consider backpressure at application level if needed.
 * - Non-copyable, non-movable.
 */
class ThreadPool {
public:
    /**
     * @brief Construct a thread pool with the given number of worker threads.
     * @param num_threads Number of worker threads (0 = use default from platform).
     */
    explicit ThreadPool(std::size_t num_threads = 0);

    ~ThreadPool();

    ThreadPool(const ThreadPool&) = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;
    ThreadPool(ThreadPool&&) = delete;
    ThreadPool& operator=(ThreadPool&&) = delete;

    /**
     * @brief Submit a task to be executed by a worker thread.
     * @param task Callable with signature void().
     * After Shutdown() or ShutdownNow(), Submit() is a no-op.
     */
    void Submit(std::function<void()> task);

    /**
     * @brief Graceful shutdown: stop accepting new tasks, drain queue, then join workers.
     * Idempotent; safe to call multiple times.
     */
    void Shutdown();

    /**
     * @brief Immediate shutdown: stop accepting and stop workers without draining queue.
     * Idempotent; safe to call multiple times.
     */
    void ShutdownNow();

    /**
     * @brief Number of worker threads.
     */
    std::size_t NumWorkers() const;

    /**
     * @brief Approximate number of tasks currently queued (for monitoring).
     */
    std::size_t PendingCount() const;

    /**
     * @brief Whether the pool has been shut down.
     */
    bool IsShutdown() const;

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

/**
 * @brief Return the process-wide default thread pool (lazy-initialized).
 * Used by the default dispatcher for multi-threaded scheduling.
 * Thread count defaults to platform::GetCPUCount().
 */
std::shared_ptr<ThreadPool> DefaultThreadPool();

/**
 * @brief Create a thread pool with the given number of threads.
 */
std::shared_ptr<ThreadPool> NewThreadPool(std::size_t num_threads);

} // namespace protoactor

#endif // PROTOACTOR_THREAD_POOL_H
