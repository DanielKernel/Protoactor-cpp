/**
 * Integration tests for Remote and Cluster modules.
 *
 * These tests verify the integration between remote communication
 * and cluster functionality. They test scenarios that require
 * both modules working together.
 *
 * NOTE: These tests simulate integration scenarios. Full end-to-end
 * tests with actual gRPC connections should be run separately.
 */

#include "external/actor.h"
#include "external/context.h"
#include "external/actor_system.h"
#include "external/props.h"
#include "external/pid.h"
#include "external/remote/remote.h"
#include "external/remote/blocklist.h"
#include "external/cluster/cluster.h"
#include "external/cluster/member.h"
#include "external/cluster/member_list.h"
#include "tests/test_common.h"
#include <atomic>
#include <chrono>
#include <memory>
#include <string>
#include <thread>
#include <vector>
#include <cstdio>

using namespace protoactor;
using namespace protoactor::test;

// ============================================================================
// Test Message Types
// ============================================================================

struct RemoteRequest {
    std::string data;
    int sequence;
};

struct RemoteResponse {
    std::string result;
    int code;
};

struct ClusterMessage {
    std::string topic;
    std::string content;
};

struct MemberJoin {
    std::string member_id;
    std::string address;
};

struct MemberLeave {
    std::string member_id;
};

// ============================================================================
// Test Actors
// ============================================================================

class RemoteServiceActor : public Actor {
public:
    explicit RemoteServiceActor(std::atomic<int>* request_count)
        : request_count_(request_count) {}

    void Receive(std::shared_ptr<Context> ctx) override {
        auto req = ctx->MessageAs<RemoteRequest>();
        if (req) {
            request_count_->fetch_add(1, std::memory_order_relaxed);
            // In real scenario, would send response back
        }
    }

private:
    std::atomic<int>* request_count_;
};

class ClusterAwareActor : public Actor {
public:
    explicit ClusterAwareActor(std::atomic<int>* join_count, std::atomic<int>* leave_count)
        : join_count_(join_count), leave_count_(leave_count) {}

    void Receive(std::shared_ptr<Context> ctx) override {
        auto join = ctx->MessageAs<MemberJoin>();
        if (join) {
            join_count_->fetch_add(1, std::memory_order_relaxed);
            return;
        }

        auto leave = ctx->MessageAs<MemberLeave>();
        if (leave) {
            leave_count_->fetch_add(1, std::memory_order_relaxed);
            return;
        }
    }

private:
    std::atomic<int>* join_count_;
    std::atomic<int>* leave_count_;
};

class PubSubSubscriberActor : public Actor {
public:
    explicit PubSubSubscriberActor(std::atomic<int>* message_count, std::vector<std::string>* received)
        : message_count_(message_count), received_(received) {}

    void Receive(std::shared_ptr<Context> ctx) override {
        auto msg = ctx->MessageAs<ClusterMessage>();
        if (msg) {
            message_count_->fetch_add(1, std::memory_order_relaxed);
            received_->push_back(msg->content);
        }
    }

private:
    std::atomic<int>* message_count_;
    std::vector<std::string>* received_;
};

// ============================================================================
// Remote + Cluster Integration Tests
// ============================================================================

static bool test_remote_actor_with_cluster_member() {
    // Scenario: Create a remote actor and register it with cluster member

    std::atomic<int> request_count(0);
    auto system = ActorSystem::New();
    auto root = system->GetRoot();

    // Create actor that handles remote requests
    auto props = Props::FromProducer([&request_count]() -> std::shared_ptr<Actor> {
        return std::make_shared<RemoteServiceActor>(&request_count);
    });

    auto pid = root->Spawn(props);
    ASSERT_TRUE(pid != nullptr);

    // Create a cluster member representing this node
    auto member_pid = PID::New("localhost:8090", "service-actor");
    cluster::Member member("node-1", "localhost:8090", {"remote-service"}, member_pid);

    ASSERT_TRUE(member.HasKind("remote-service"));
    ASSERT_TRUE(member.Id() == "node-1");
    ASSERT_TRUE(member.Address() == "localhost:8090");

    // Simulate receiving a remote request
    root->Send(pid, std::make_shared<RemoteRequest>(RemoteRequest{"test-data", 1}));

    // Wait for message processing
    for (int i = 0; i < 50; ++i) {
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
        if (request_count.load(std::memory_order_relaxed) >= 1) break;
    }

    ASSERT_GE(request_count.load(), 1);

    system->Shutdown();
    return true;
}

static bool test_cluster_member_list_with_remote_endpoints() {
    // Scenario: Manage cluster members that represent remote endpoints

    cluster::MemberList member_list;

    // Add multiple members representing different nodes
    auto pid1 = PID::New("node1.example.com:8090", "member-1");
    auto pid2 = PID::New("node2.example.com:8090", "member-2");
    auto pid3 = PID::New("node3.example.com:8090", "member-3");

    member_list.AddMember(std::make_shared<cluster::Member>(
        "member-1", "node1.example.com:8090", {"service-a", "service-b"}, pid1
    ));
    member_list.AddMember(std::make_shared<cluster::Member>(
        "member-2", "node2.example.com:8090", {"service-a", "service-c"}, pid2
    ));
    member_list.AddMember(std::make_shared<cluster::Member>(
        "member-3", "node3.example.com:8090", {"service-b"}, pid3
    ));

    // Verify member count
    ASSERT_EQ(member_list.MemberCount(), 3);

    // Query by kind (simulating service discovery)
    auto service_a_members = member_list.GetMembersByKind("service-a");
    ASSERT_EQ(service_a_members.size(), 2);

    auto service_b_members = member_list.GetMembersByKind("service-b");
    ASSERT_EQ(service_b_members.size(), 2);

    auto service_c_members = member_list.GetMembersByKind("service-c");
    ASSERT_EQ(service_c_members.size(), 1);

    // Remove a member (simulating node failure)
    member_list.RemoveMember("member-2");
    ASSERT_EQ(member_list.MemberCount(), 2);

    // Verify service-a now has only 1 member
    service_a_members = member_list.GetMembersByKind("service-a");
    ASSERT_EQ(service_a_members.size(), 1);

    return true;
}

static bool test_blocklist_integration_with_cluster() {
    // Scenario: Block problematic remote endpoints in cluster

    remote::Blocklist blocklist;
    cluster::MemberList member_list;

    // Add members
    auto pid1 = PID::New("good-node:8090", "member-1");
    auto pid2 = PID::New("bad-node:8090", "member-2");

    member_list.AddMember(std::make_shared<cluster::Member>(
        "member-1", "good-node:8090", {"service"}, pid1
    ));
    member_list.AddMember(std::make_shared<cluster::Member>(
        "member-2", "bad-node:8090", {"service"}, pid2
    ));

    // Block the problematic node
    blocklist.Block("bad-node:8090");

    // Verify blocklist status
    ASSERT_TRUE(!blocklist.IsBlocked("good-node:8090"));
    ASSERT_TRUE(blocklist.IsBlocked("bad-node:8090"));

    // In real scenario, blocked members would be filtered out
    auto members = member_list.GetMembers();
    int active_count = 0;
    for (const auto& m : members) {
        if (!blocklist.IsBlocked(m->Address())) {
            active_count++;
        }
    }
    ASSERT_EQ(active_count, 1);

    // Unblock and verify
    blocklist.Unblock("bad-node:8090");
    ASSERT_TRUE(!blocklist.IsBlocked("bad-node:8090"));

    return true;
}

static bool test_cluster_topology_change_notification() {
    // Scenario: Actor receives notifications about cluster topology changes

    std::atomic<int> join_count(0);
    std::atomic<int> leave_count(0);

    auto system = ActorSystem::New();
    auto root = system->GetRoot();

    auto props = Props::FromProducer([&join_count, &leave_count]() -> std::shared_ptr<Actor> {
        return std::make_shared<ClusterAwareActor>(&join_count, &leave_count);
    });

    auto pid = root->Spawn(props);
    ASSERT_TRUE(pid != nullptr);

    // Simulate topology events
    root->Send(pid, std::make_shared<MemberJoin>(MemberJoin{"node-1", "host1:8090"}));
    root->Send(pid, std::make_shared<MemberJoin>(MemberJoin{"node-2", "host2:8090"}));
    root->Send(pid, std::make_shared<MemberJoin>(MemberJoin{"node-3", "host3:8090"}));

    // Wait for processing
    for (int i = 0; i < 50; ++i) {
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
        if (join_count.load(std::memory_order_relaxed) >= 3) break;
    }
    ASSERT_GE(join_count.load(), 3);

    // Simulate member leaving
    root->Send(pid, std::make_shared<MemberLeave>(MemberLeave{"node-2"}));

    for (int i = 0; i < 50; ++i) {
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
        if (leave_count.load(std::memory_order_relaxed) >= 1) break;
    }
    ASSERT_GE(leave_count.load(), 1);

    system->Shutdown();
    return true;
}

static bool test_cross_node_message_routing() {
    // Scenario: Route messages to appropriate nodes based on member capabilities

    cluster::MemberList member_list;

    // Setup members with different capabilities
    auto pid1 = PID::New("order-node:8090", "order-service");
    auto pid2 = PID::New("payment-node:8090", "payment-service");
    auto pid3 = PID::New("inventory-node:8090", "inventory-service");

    member_list.AddMember(std::make_shared<cluster::Member>(
        "order-node", "order-node:8090", {"order-service"}, pid1
    ));
    member_list.AddMember(std::make_shared<cluster::Member>(
        "payment-node", "payment-node:8090", {"payment-service"}, pid2
    ));
    member_list.AddMember(std::make_shared<cluster::Member>(
        "inventory-node", "inventory-node:8090", {"inventory-service"}, pid3
    ));

    // Route to order service
    auto order_members = member_list.GetMembersByKind("order-service");
    ASSERT_EQ(order_members.size(), 1);
    ASSERT_TRUE(order_members[0]->Address() == "order-node:8090");

    // Route to payment service
    auto payment_members = member_list.GetMembersByKind("payment-service");
    ASSERT_EQ(payment_members.size(), 1);
    ASSERT_TRUE(payment_members[0]->Address() == "payment-node:8090");

    // Route to inventory service
    auto inventory_members = member_list.GetMembersByKind("inventory-service");
    ASSERT_EQ(inventory_members.size(), 1);
    ASSERT_TRUE(inventory_members[0]->Address() == "inventory-node:8090");

    // Non-existent service
    auto shipping_members = member_list.GetMembersByKind("shipping-service");
    ASSERT_EQ(shipping_members.size(), 0);

    return true;
}

static bool test_cluster_config_with_remote_settings() {
    // Scenario: Create cluster config with remote-compatible settings

    auto config = cluster::Config::New("test-cluster", "0.0.0.0", 8090);

    ASSERT_TRUE(config != nullptr);
    ASSERT_TRUE(config->ClusterName() == "test-cluster");
    ASSERT_TRUE(config->Host() == "0.0.0.0");
    ASSERT_EQ(config->Port(), 8090);

    // Verify address format is compatible with remote module
    std::string expected_addr = "0.0.0.0:8090";
    std::string actual_addr = config->Host() + ":" + std::to_string(config->Port());
    ASSERT_TRUE(actual_addr.find(":8090") != std::string::npos);

    return true;
}

static bool test_multi_region_cluster_simulation() {
    // Scenario: Simulate a multi-region cluster with remote connections

    cluster::MemberList member_list;
    remote::Blocklist blocklist;

    // Region US
    auto us1 = PID::New("us-east.example.com:8090", "us-node-1");
    auto us2 = PID::New("us-west.example.com:8090", "us-node-2");

    // Region EU
    auto eu1 = PID::New("eu-west.example.com:8090", "eu-node-1");
    auto eu2 = PID::New("eu-central.example.com:8090", "eu-node-2");

    // Region APAC
    auto ap1 = PID::New("ap-southeast.example.com:8090", "ap-node-1");

    member_list.AddMember(std::make_shared<cluster::Member>(
        "us-node-1", "us-east.example.com:8090", {"service", "us-region"}, us1
    ));
    member_list.AddMember(std::make_shared<cluster::Member>(
        "us-node-2", "us-west.example.com:8090", {"service", "us-region"}, us2
    ));
    member_list.AddMember(std::make_shared<cluster::Member>(
        "eu-node-1", "eu-west.example.com:8090", {"service", "eu-region"}, eu1
    ));
    member_list.AddMember(std::make_shared<cluster::Member>(
        "eu-node-2", "eu-central.example.com:8090", {"service", "eu-region"}, eu2
    ));
    member_list.AddMember(std::make_shared<cluster::Member>(
        "ap-node-1", "ap-southeast.example.com:8090", {"service", "ap-region"}, ap1
    ));

    // Verify total members
    ASSERT_EQ(member_list.MemberCount(), 5);

    // Query by service (should return all nodes)
    auto service_members = member_list.GetMembersByKind("service");
    ASSERT_EQ(service_members.size(), 5);

    // Query by region
    auto us_members = member_list.GetMembersByKind("us-region");
    ASSERT_EQ(us_members.size(), 2);

    auto eu_members = member_list.GetMembersByKind("eu-region");
    ASSERT_EQ(eu_members.size(), 2);

    auto ap_members = member_list.GetMembersByKind("ap-region");
    ASSERT_EQ(ap_members.size(), 1);

    // Simulate network partition - block EU region
    blocklist.Block("eu-west.example.com:8090");
    blocklist.Block("eu-central.example.com:8090");

    int available_count = 0;
    for (const auto& m : service_members) {
        if (!blocklist.IsBlocked(m->Address())) {
            available_count++;
        }
    }
    ASSERT_EQ(available_count, 3); // Only US and AP nodes available

    // Restore EU region
    blocklist.Clear();
    ASSERT_TRUE(!blocklist.IsBlocked("eu-west.example.com:8090"));
    ASSERT_TRUE(!blocklist.IsBlocked("eu-central.example.com:8090"));

    return true;
}

static bool test_failover_scenario() {
    // Scenario: Handle failover when primary node fails

    cluster::MemberList member_list;
    remote::Blocklist blocklist;

    // Primary and backup nodes
    auto primary = PID::New("primary.example.com:8090", "primary-node");
    auto backup1 = PID::New("backup1.example.com:8090", "backup-node-1");
    auto backup2 = PID::New("backup2.example.com:8090", "backup-node-2");

    member_list.AddMember(std::make_shared<cluster::Member>(
        "primary-node", "primary.example.com:8090", {"critical-service"}, primary
    ));
    member_list.AddMember(std::make_shared<cluster::Member>(
        "backup-node-1", "backup1.example.com:8090", {"critical-service"}, backup1
    ));
    member_list.AddMember(std::make_shared<cluster::Member>(
        "backup-node-2", "backup2.example.com:8090", {"critical-service"}, backup2
    ));

    // All nodes available
    auto available = member_list.GetMembersByKind("critical-service");
    ASSERT_EQ(available.size(), 3);

    // Primary fails
    blocklist.Block("primary.example.com:8090");

    // Find next available node
    std::shared_ptr<cluster::Member> new_primary = nullptr;
    for (const auto& m : available) {
        if (!blocklist.IsBlocked(m->Address())) {
            new_primary = m;
            break;
        }
    }

    ASSERT_TRUE(new_primary != nullptr);
    ASSERT_TRUE(new_primary->Address() != "primary.example.com:8090");

    // Backup1 also fails
    blocklist.Block("backup1.example.com:8090");

    // Find next available
    new_primary = nullptr;
    for (const auto& m : available) {
        if (!blocklist.IsBlocked(m->Address())) {
            new_primary = m;
            break;
        }
    }

    ASSERT_TRUE(new_primary != nullptr);
    ASSERT_TRUE(new_primary->Address() == "backup2.example.com:8090");

    return true;
}

static bool test_request_response_with_remote_target() {
    // Scenario: Simulate request-response pattern with remote target

    std::atomic<int> response_count(0);
    auto system = ActorSystem::New();
    auto root = system->GetRoot();

    // Create a service actor
    auto props = Props::FromProducer([&response_count]() -> std::shared_ptr<Actor> {
        return std::make_shared<RemoteServiceActor>(&response_count);
    });

    auto service_pid = root->Spawn(props);
    ASSERT_TRUE(service_pid != nullptr);

    // Create a "remote" PID (simulating remote actor reference)
    auto remote_pid = PID::New("remote-host:8090", "remote-service");

    // Verify we can distinguish local vs remote
    ASSERT_TRUE(service_pid->Address() != remote_pid->Address());

    // In real scenario, would route through remote module
    // For now, verify the PID structure is correct
    ASSERT_TRUE(remote_pid->Address().find("remote-host") != std::string::npos);

    system->Shutdown();
    return true;
}

static bool test_pubsub_integration_with_cluster() {
    // Scenario: Pub/Sub pattern with cluster member awareness

    std::atomic<int> message_count(0);
    std::vector<std::string> received_messages;

    auto system = ActorSystem::New();
    auto root = system->GetRoot();

    // Create subscriber actors
    auto props = Props::FromProducer([&message_count, &received_messages]() -> std::shared_ptr<Actor> {
        return std::make_shared<PubSubSubscriberActor>(&message_count, &received_messages);
    });

    auto sub1 = root->Spawn(props);
    auto sub2 = root->Spawn(props);
    auto sub3 = root->Spawn(props);

    ASSERT_TRUE(sub1 != nullptr);
    ASSERT_TRUE(sub2 != nullptr);
    ASSERT_TRUE(sub3 != nullptr);

    // Create cluster member list representing subscriber nodes
    cluster::MemberList member_list;
    member_list.AddMember(std::make_shared<cluster::Member>(
        "sub-node-1", "sub1-host:8090", {"pubsub-subscriber"}, sub1
    ));
    member_list.AddMember(std::make_shared<cluster::Member>(
        "sub-node-2", "sub2-host:8090", {"pubsub-subscriber"}, sub2
    ));
    member_list.AddMember(std::make_shared<cluster::Member>(
        "sub-node-3", "sub3-host:8090", {"pubsub-subscriber"}, sub3
    ));

    // Broadcast messages to all subscribers
    root->Send(sub1, std::make_shared<ClusterMessage>(ClusterMessage{"topic-1", "message-A"}));
    root->Send(sub2, std::make_shared<ClusterMessage>(ClusterMessage{"topic-1", "message-A"}));
    root->Send(sub3, std::make_shared<ClusterMessage>(ClusterMessage{"topic-1", "message-A"}));

    // Wait for delivery
    for (int i = 0; i < 50; ++i) {
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
        if (message_count.load(std::memory_order_relaxed) >= 3) break;
    }

    ASSERT_GE(message_count.load(), 3);

    // Verify all subscribers received the message
    int expected_count = 0;
    for (const auto& msg : received_messages) {
        if (msg == "message-A") {
            expected_count++;
        }
    }
    ASSERT_GE(expected_count, 3);

    system->Shutdown();
    return true;
}

// ============================================================================
// Main
// ============================================================================

int main() {
    std::fprintf(stdout, "Remote/Cluster Integration tests\n");
    std::fprintf(stdout, "================================\n\n");

    int failed = 0;
#define RUN(name) if (!run_test(#name, name)) ++failed

    // Remote + Cluster integration tests
    RUN(test_remote_actor_with_cluster_member);
    RUN(test_cluster_member_list_with_remote_endpoints);
    RUN(test_blocklist_integration_with_cluster);
    RUN(test_cluster_topology_change_notification);
    RUN(test_cross_node_message_routing);
    RUN(test_cluster_config_with_remote_settings);
    RUN(test_multi_region_cluster_simulation);
    RUN(test_failover_scenario);
    RUN(test_request_response_with_remote_target);
    RUN(test_pubsub_integration_with_cluster);

#undef RUN

    std::fprintf(stdout, "\n================================\n");
    std::fprintf(stdout, "Total: %d failed\n", failed);
    return failed == 0 ? 0 : 1;
}
