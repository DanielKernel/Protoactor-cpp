#include "protoactor/actor.h"
#include "protoactor/context.h"
#include "protoactor/actor_system.h"
#include "protoactor/props.h"
#include <iostream>
#include <memory>
#include <thread>
#include <chrono>
#include <vector>
#include <atomic>
#include <condition_variable>
#include <mutex>
#include <string>

struct EmptyMsg {};

std::atomic<long> g_received{0};
long g_total = 0;
std::mutex g_m;
std::condition_variable g_cv;

class BenchActor : public protoactor::Actor {
public:
    void Receive(std::shared_ptr<protoactor::Context> context) override {
        auto msg = context->Message();
        if (msg) {
            // We don't inspect the message, just count
            long v = ++g_received;
            if (v >= g_total) {
                std::lock_guard<std::mutex> lk(g_m);
                g_cv.notify_one();
            }
        }
    }
};

void print_usage(const char* prog) {
    std::cout << "Usage: " << prog << " --actors N --messages M --sleep-ms S\n";
    std::cout << "  --actors N    number of actors to spawn (default 100)\n";
    std::cout << "  --messages M  messages per actor to send (default 1000)\n";
    std::cout << "  --sleep-ms S  optional sleep between batches (ms, default 0)\n";
}

int main(int argc, char** argv) {
    int actors = 100;
    int messages = 1000;
    int sleep_ms = 0;
    for (int i = 1; i < argc; ++i) {
        std::string s = argv[i];
        if (s == "--actors" && i + 1 < argc) { actors = std::stoi(argv[++i]); }
        else if (s == "--messages" && i + 1 < argc) { messages = std::stoi(argv[++i]); }
        else if (s == "--sleep-ms" && i + 1 < argc) { sleep_ms = std::stoi(argv[++i]); }
        else if (s == "--help" || s == "-h") { print_usage(argv[0]); return 0; }
    }

    try {
        std::cout << "Perf benchmark: actors=" << actors << " messages=" << messages << "\n";
        auto system = protoactor::ActorSystem::New();
        auto root = system->GetRoot();

        auto props = protoactor::Props::FromProducer([]() -> std::shared_ptr<protoactor::Actor> {
            return std::make_shared<BenchActor>();
        });

        std::vector<std::shared_ptr<protoactor::PID>> pids;
        pids.reserve(actors);
        for (int i = 0; i < actors; ++i) {
            auto pid = root->Spawn(props);
            if (!pid) {
                std::cerr << "Failed to spawn actor" << std::endl;
                return 1;
            }
            pids.push_back(pid);
        }

        g_received.store(0);
        g_total = (long)actors * (long)messages;

        auto start = std::chrono::steady_clock::now();

        // Send messages
        for (int m = 0; m < messages; ++m) {
            for (int a = 0; a < actors; ++a) {
                auto msg = std::make_shared<EmptyMsg>();
                root->Send(pids[a], msg);
            }
            if (sleep_ms > 0) std::this_thread::sleep_for(std::chrono::milliseconds(sleep_ms));
        }

        // Wait until all messages processed or timeout
        {
            std::unique_lock<std::mutex> lk(g_m);
            g_cv.wait_for(lk, std::chrono::seconds(120), [](){ return g_received.load() >= g_total; });
        }

        auto end = std::chrono::steady_clock::now();
        std::chrono::duration<double> elapsed = end - start;

        std::cout << "Total messages expected=" << g_total << " received=" << g_received.load() << "\n";
        std::cout << "Elapsed seconds=" << elapsed.count() << "\n";

        system->Shutdown();
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Exception: " << e.what() << std::endl;
        return 1;
    }
}
