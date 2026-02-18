/**
 * @file pubsub_example.cpp
 * @brief Pub/Sub example demonstrating cluster-wide publish-subscribe messaging
 *
 * This example shows how to:
 * 1. Set up Pub/Sub in a cluster environment
 * 2. Subscribe to topics
 * 3. Publish messages to topics
 * 4. Handle topic messages
 *
 * Go equivalent:
 *   pubsub.Subscribe("topic", subscriber)
 *   pubsub.Publish("topic", message)
 *
 * NOTE: This example requires gRPC and Protobuf to be enabled.
 * Build with: cmake .. -DENABLE_GRPC=ON -DENABLE_PROTOBUF=ON
 */

#include "external/actor.h"
#include "external/context.h"
#include "external/actor_system.h"
#include "external/props.h"
#include "external/messages.h"
#include <iostream>
#include <memory>
#include <thread>
#include <chrono>
#include <string>

#ifdef ENABLE_GRPC
#include "external/cluster/cluster.h"
#include "external/cluster/pubsub.h"

// ============================================================================
// Message Types
// ============================================================================

struct NewsItem {
    std::string title;
    std::string content;
    std::string author;

    NewsItem(const std::string& t, const std::string& c, const std::string& a)
        : title(t), content(c), author(a) {}
};

struct StockUpdate {
    std::string symbol;
    double price;
    double change;

    StockUpdate(const std::string& s, double p, double c)
        : symbol(s), price(p), change(c) {}
};

struct Notification {
    std::string message;
    int64_t timestamp;

    Notification(const std::string& m, int64_t t)
        : message(m), timestamp(t) {}
};

// ============================================================================
// Subscriber Actors
// ============================================================================

/**
 * @brief News subscriber that receives news updates.
 */
class NewsSubscriber : public protoactor::Actor {
public:
    explicit NewsSubscriber(const std::string& name) : name_(name) {}

    void Receive(std::shared_ptr<protoactor::Context> context) override {
        auto msg = context->Message();
        if (!msg) return;

        void* ptr = msg.get();

        if (NewsItem* news = static_cast<NewsItem*>(ptr)) {
            std::cout << "[" << name_ << "] News received: " << news->title << std::endl;
            std::cout << "         By " << news->author << ": " << news->content << std::endl;
        }
    }

private:
    std::string name_;
};

/**
 * @brief Stock ticker subscriber that receives stock updates.
 */
class StockSubscriber : public protoactor::Actor {
public:
    explicit StockSubscriber(const std::string& name) : name_(name) {}

    void Receive(std::shared_ptr<protoactor::Context> context) override {
        auto msg = context->Message();
        if (!msg) return;

        void* ptr = msg.get();

        if (StockUpdate* stock = static_cast<StockUpdate*>(ptr)) {
            std::string direction = stock->change >= 0 ? "+" : "";
            std::cout << "[" << name_ << "] Stock: " << stock->symbol
                      << " $" << stock->price
                      << " (" << direction << stock->change << "%)" << std::endl;
        }
    }

private:
    std::string name_;
};

/**
 * @brief Multi-topic subscriber that subscribes to multiple topics.
 */
class MultiTopicSubscriber : public protoactor::Actor {
public:
    explicit MultiTopicSubscriber(const std::string& name) : name_(name) {}

    void Receive(std::shared_ptr<protoactor::Context> context) override {
        auto msg = context->Message();
        if (!msg) return;

        void* ptr = msg.get();

        if (NewsItem* news = static_cast<NewsItem*>(ptr)) {
            std::cout << "[" << name_ << "][News] " << news->title << std::endl;
            return;
        }

        if (StockUpdate* stock = static_cast<StockUpdate*>(ptr)) {
            std::cout << "[" << name_ << "][Stock] " << stock->symbol << std::endl;
            return;
        }

        if (Notification* notif = static_cast<Notification*>(ptr)) {
            std::cout << "[" << name_ << "][Notification] " << notif->message << std::endl;
            return;
        }
    }

private:
    std::string name_;
};

// ============================================================================
// Publisher
// ============================================================================

/**
 * @brief Publisher actor that publishes messages to topics.
 */
class Publisher : public protoactor::Actor {
public:
    void SetPubSub(std::shared_ptr<protoactor::cluster::PubSub> pubsub) {
        pubsub_ = pubsub;
    }

    void Receive(std::shared_ptr<protoactor::Context> context) override {
        // Publisher can also receive messages if needed
    }

    void PublishNews(const std::string& title, const std::string& content, const std::string& author) {
        if (pubsub_) {
            pubsub_->Publish("news", std::make_shared<NewsItem>(title, content, author));
        }
    }

    void PublishStock(const std::string& symbol, double price, double change) {
        if (pubsub_) {
            pubsub_->Publish("stocks", std::make_shared<StockUpdate>(symbol, price, change));
        }
    }

    void PublishNotification(const std::string& message) {
        if (pubsub_) {
            auto timestamp = std::chrono::system_clock::now().time_since_epoch().count();
            pubsub_->Publish("notifications", std::make_shared<Notification>(message, timestamp));
        }
    }

private:
    std::shared_ptr<protoactor::cluster::PubSub> pubsub_;
};

// ============================================================================
// Demo Functions
// ============================================================================

void DemoBasicPubSub(std::shared_ptr<protoactor::Context> root,
                     std::shared_ptr<protoactor::cluster::PubSub> pubsub) {
    std::cout << "\n=== Basic Pub/Sub Demo ===" << std::endl;
    std::cout << "Subscribers receive messages from topics they subscribe to.\n" << std::endl;

    // Create subscribers
    auto news_props = protoactor::Props::FromProducer([]() {
        return std::make_shared<NewsSubscriber>("NewsReader-1");
    });
    auto news_subscriber = root->Spawn(news_props);

    // Subscribe to topics
    pubsub->Subscribe("news", news_subscriber);

    std::cout << "Subscribed to 'news' topic.\n" << std::endl;

    // Publish some news
    pubsub->Publish("news", std::make_shared<NewsItem>(
        "ProtoActor C++ Released",
        "Version 1.0 brings full feature parity with Go version",
        "Dev Team"
    ));

    pubsub->Publish("news", std::make_shared<NewsItem>(
        "Performance Update",
        "New benchmarks show 20% improvement in message throughput",
        "Performance Team"
    ));

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
}

void DemoMultipleSubscribers(std::shared_ptr<protoactor::Context> root,
                             std::shared_ptr<protoactor::cluster::PubSub> pubsub) {
    std::cout << "\n=== Multiple Subscribers Demo ===" << std::endl;
    std::cout << "Multiple subscribers can receive the same message.\n" << std::endl;

    // Create multiple subscribers for stocks
    auto stock_props1 = protoactor::Props::FromProducer([]() {
        return std::make_shared<StockSubscriber>("Trader-A");
    });
    auto stock_props2 = protoactor::Props::FromProducer([]() {
        return std::make_shared<StockSubscriber>("Trader-B");
    });
    auto stock_props3 = protoactor::Props::FromProducer([]() {
        return std::make_shared<StockSubscriber>("Analyst");
    });

    auto trader_a = root->Spawn(stock_props1);
    auto trader_b = root->Spawn(stock_props2);
    auto analyst = root->Spawn(stock_props3);

    // All subscribe to stocks topic
    pubsub->Subscribe("stocks", trader_a);
    pubsub->Subscribe("stocks", trader_b);
    pubsub->Subscribe("stocks", analyst);

    std::cout << "3 subscribers registered for 'stocks' topic.\n" << std::endl;

    // Publish stock updates
    pubsub->Publish("stocks", std::make_shared<StockUpdate>("AAPL", 175.50, 2.3));
    pubsub->Publish("stocks", std::make_shared<StockUpdate>("GOOGL", 2850.00, -1.5));
    pubsub->Publish("stocks", std::make_shared<StockUpdate>("MSFT", 320.75, 0.8));

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
}

void DemoMultiTopicSubscriber(std::shared_ptr<protoactor::Context> root,
                              std::shared_ptr<protoactor::cluster::PubSub> pubsub) {
    std::cout << "\n=== Multi-Topic Subscriber Demo ===" << std::endl;
    std::cout << "One subscriber can listen to multiple topics.\n" << std::endl;

    // Create a multi-topic subscriber
    auto multi_props = protoactor::Props::FromProducer([]() {
        return std::make_shared<MultiTopicSubscriber>("Dashboard");
    });
    auto dashboard = root->Spawn(multi_props);

    // Subscribe to multiple topics
    pubsub->Subscribe("news", dashboard);
    pubsub->Subscribe("stocks", dashboard);
    pubsub->Subscribe("notifications", dashboard);

    std::cout << "Dashboard subscribed to: news, stocks, notifications\n" << std::endl;

    // Publish to different topics
    pubsub->Publish("news", std::make_shared<NewsItem>(
        "Market Open", "Trading session begins", "System"
    ));
    pubsub->Publish("stocks", std::make_shared<StockUpdate>("TSLA", 850.00, 3.5));
    pubsub->Publish("notifications", std::make_shared<Notification>(
        "System maintenance scheduled", std::chrono::system_clock::now().time_since_epoch().count()
    ));

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
}

void DemoUnsubscribe(std::shared_ptr<protoactor::Context> root,
                     std::shared_ptr<protoactor::cluster::PubSub> pubsub) {
    std::cout << "\n=== Unsubscribe Demo ===" << std::endl;
    std::cout << "Subscribers can unsubscribe from topics.\n" << std::endl;

    // Create subscriber
    auto props = protoactor::Props::FromProducer([]() {
        return std::make_shared<NewsSubscriber>("TempReader");
    });
    auto temp_reader = root->Spawn(props);

    // Subscribe
    pubsub->Subscribe("notifications", temp_reader);
    std::cout << "Subscribed to 'notifications'" << std::endl;

    // Publish - should receive
    pubsub->Publish("notifications", std::make_shared<Notification>(
        "Message 1", std::chrono::system_clock::now().time_since_epoch().count()
    ));

    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    // Unsubscribe
    pubsub->Unsubscribe("notifications", temp_reader);
    std::cout << "Unsubscribed from 'notifications'" << std::endl;

    // Publish - should NOT receive
    pubsub->Publish("notifications", std::make_shared<Notification>(
        "Message 2 (should not be received)", std::chrono::system_clock::now().time_since_epoch().count()
    ));

    std::this_thread::sleep_for(std::chrono::milliseconds(50));
}

void ShowPubSubSummary() {
    std::cout << "\n=== Pub/Sub Summary ===" << std::endl;

    std::cout << "\n"
              << "| Operation     | Method                    |\n"
              << "|---------------|---------------------------|\n"
              << "| Subscribe     | pubsub->Subscribe(topic, pid) |\n"
              << "| Unsubscribe   | pubsub->Unsubscribe(topic, pid) |\n"
              << "| Publish       | pubsub->Publish(topic, msg) |\n"
              << std::endl;

    std::cout << "Use Cases:\n"
              << "  - Real-time notifications\n"
              << "  - Event broadcasting\n"
              << "  - Chat/messaging systems\n"
              << "  - Market data distribution\n"
              << "  - IoT sensor data\n"
              << std::endl;

    std::cout << "Go equivalent:\n"
              << "  // Subscribe\n"
              << "  pubsub.Subscribe(\"topic\", subscriberPid)\n"
              << "\n"
              << "  // Publish\n"
              << "  pubsub.Publish(\"topic\", &Message{...})\n"
              << "\n"
              << "  // Unsubscribe\n"
              << "  pubsub.Unsubscribe(\"topic\", subscriberPid)\n"
              << std::endl;
}

void ShowArchitecture() {
    std::cout << "\n=== Pub/Sub Architecture ===" << std::endl;
    std::cout << "\n"
              << "  ┌──────────────────────────────────────────────────────────┐\n"
              << "  │                      Cluster                             │\n"
              << "  │                                                          │\n"
              << "  │   ┌─────────────┐   ┌─────────────┐   ┌─────────────┐   │\n"
              << "  │   │   Node 1    │   │   Node 2    │   │   Node 3    │   │\n"
              << "  │   │ ┌─────────┐ │   │ ┌─────────┐ │   │ ┌─────────┐ │   │\n"
              << "  │   │ │Publisher│ │   │ │Sub A    │ │   │ │Sub B    │ │   │\n"
              << "  │   │ └────┬────┘ │   │ └────▲────┘ │   │ └────▲────┘ │   │\n"
              << "  │   │      │      │   │      │      │   │      │      │   │\n"
              << "  │   │      └──────┼───┴──────┼──────┴───┬──┘      │      │\n"
              << "  │   │             │  PubSub  │          │  PubSub │      │\n"
              << "  │   │             └──────────┴──────────┴─────────┘      │\n"
              << "  │                         Topic                          │\n"
              << "  └──────────────────────────────────────────────────────────┘\n"
              << std::endl;
}

#endif // ENABLE_GRPC

void PrintUsage(const char* program) {
    std::cout << "ProtoActor C++ Pub/Sub Example\n\n"
              << "Usage:\n"
              << "  " << program << " [options]\n\n"
              << "This example demonstrates cluster-wide publish-subscribe messaging.\n"
              << std::endl;

#ifdef ENABLE_GRPC
    std::cout << "gRPC/Cluster support: ENABLED" << std::endl;
#else
    std::cout << "gRPC/Cluster support: DISABLED\n"
              << "Rebuild with -DENABLE_GRPC=ON -DENABLE_PROTOBUF=ON"
              << std::endl;
#endif
}

int main(int argc, char* argv[]) {
    std::cout << "=== ProtoActor C++ Pub/Sub Example ===" << std::endl;

#ifdef ENABLE_GRPC
    try {
        auto system = protoactor::ActorSystem::New();
        auto root = system->GetRoot();

        // Create cluster config
        auto config = protoactor::cluster::Config::New("pubsub-demo", "localhost", 8090);
        auto cluster = protoactor::cluster::Cluster::New(system, config);
        cluster->Start();

        // Get PubSub from cluster
        auto pubsub = cluster->GetPubSub();
        if (!pubsub) {
            std::cout << "\nNote: PubSub requires cluster to be fully initialized." << std::endl;
            std::cout << "Showing conceptual demos instead.\n" << std::endl;
        }

        // Demonstrate different Pub/Sub patterns
        if (pubsub) {
            DemoBasicPubSub(root, pubsub);
            DemoMultipleSubscribers(root, pubsub);
            DemoMultiTopicSubscriber(root, pubsub);
            DemoUnsubscribe(root, pubsub);
        }

        // Show architecture and summary
        ShowArchitecture();
        ShowPubSubSummary();

        // Wait for all messages to be processed
        std::this_thread::sleep_for(std::chrono::milliseconds(500));

        std::cout << "\n=== Example Complete ===" << std::endl;

        if (cluster) {
            cluster->Shutdown();
        }
        system->Shutdown();
    } catch (const std::exception& e) {
        std::cerr << "Exception: " << e.what() << std::endl;
        return 1;
    }
#else
    PrintUsage(argv[0]);

    // Show conceptual information even without gRPC
    std::cout << "\n=== Pub/Sub Concepts (Demo Mode) ===" << std::endl;

    std::cout << "\nPub/Sub Pattern:\n"
              << "  1. Publishers send messages to topics (not directly to receivers)\n"
              << "  2. Subscribers register interest in topics\n"
              << "  3. PubSub system delivers messages to all interested subscribers\n"
              << std::endl;

    std::cout << "C++ API:\n"
              << "  // Subscribe to a topic\n"
              << "  pubsub->Subscribe(\"my-topic\", subscriberPid);\n"
              << "\n"
              << "  // Publish a message\n"
              << "  pubsub->Publish(\"my-topic\", std::make_shared<MyMessage>(...));\n"
              << "\n"
              << "  // Unsubscribe\n"
              << "  pubsub->Unsubscribe(\"my-topic\", subscriberPid);\n"
              << std::endl;

    ShowArchitecture();
    ShowPubSubSummary();
#endif

    return 0;
}

/*
 * ============================================================================
 * PUB/SUB GUIDE
 * ============================================================================
 *
 * 1. BASIC CONCEPTS
 * -----------------
 *
 * Pub/Sub (Publish-Subscribe) is a messaging pattern where:
 * - Publishers send messages to topics (not directly to receivers)
 * - Subscribers express interest in topics
 * - The system delivers messages to all interested subscribers
 *
 * Benefits:
 *   - Loose coupling between publishers and subscribers
 *   - Scalable broadcast messaging
 *   - Dynamic subscription management
 *
 *
 * 2. CLUSTER-WIDE PUB/SUB
 * -----------------------
 *
 * In a cluster, Pub/Sub works across all nodes:
 * - Subscribers on any node can subscribe to topics
 * - Publishers on any node can publish to topics
 * - Messages are delivered to all subscribers regardless of location
 *
 * C++:
 *   auto cluster = Cluster::New(system, config);
 *   cluster->Start();
 *
 *   auto pubsub = cluster->GetPubSub();
 *   pubsub->Subscribe("topic", subscriberPid);
 *   pubsub->Publish("topic", message);
 *
 * Go:
 *   c := cluster.New(system, config)
 *   c.Start()
 *
 *   pubsub := c.PubSub()
 *   pubsub.Subscribe("topic", subscriberPid)
 *   pubsub.Publish("topic", message)
 *
 *
 * 3. TOPIC NAMING CONVENTIONS
 * ---------------------------
 *
 * Good topic names:
 *   - "news.sports" (hierarchical)
 *   - "stocks.AAPL" (entity-specific)
 *   - "user.123.notifications" (user-specific)
 *   - "system.alerts.critical" (severity-based)
 *
 * Avoid:
 *   - Dynamic topics that change frequently
 *   - Very long topic names
 *   - Topics with special characters
 *
 *
 * 4. MESSAGE DESIGN
 * -----------------
 *
 * Messages should be:
 *   - Serializable (for cluster delivery)
 *   - Self-contained (include all needed context)
 *   - Small (for performance)
 *
 * Good:
 *   struct StockUpdate {
 *       string symbol;    // "AAPL"
 *       double price;     // 175.50
 *       double change;    // 2.3
 *   };
 *
 * Avoid:
 *   struct LargeUpdate {
 *       vector<char> data;  // Could be very large
 *   };
 *
 *
 * 5. SUBSCRIBER LIFECYCLE
 * -----------------------
 *
 * - Remember to unsubscribe when actor stops
 * - Handle duplicate messages (at-least-once delivery)
 * - Handle out-of-order messages if needed
 *
 * C++:
 *   void Receive(Context ctx) {
 *       if (dynamic_cast<Stopped*>(ctx->Message().get())) {
 *           pubsub->Unsubscribe("topic", ctx->Self());
 *       }
 *   }
 *
 *
 * 6. PERFORMANCE CONSIDERATIONS
 * -----------------------------
 *
 * - Batch messages when possible
 * - Use appropriate message sizes
 * - Monitor subscriber count per topic
 * - Consider topic partitioning for high-volume topics
 *
 */
