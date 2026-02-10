#include "protoactor/actor_system.h"
#include "protoactor/process_registry.h"
#include "protoactor/context.h"
#include "protoactor/root_context.h"
#include "protoactor/eventstream/eventstream.h"
#include "protoactor/deadletter.h"
#include "protoactor/guardian.h"
#include "protoactor/extensions.h"
#include "protoactor/config.h"
#include <sstream>
#include <random>
#include <iomanip>

namespace protoactor {

ActorSystem::ActorSystem() : stopped_(false) {
}

std::shared_ptr<ActorSystem> ActorSystem::New(std::shared_ptr<Config> config) {
    if (!config) {
        config = Config::Default();
    }
    
    auto system = std::make_shared<ActorSystem>();
    system->Initialize(config);
    return system;
}

void ActorSystem::Initialize(std::shared_ptr<Config> config) {
    config_ = config;
    
    // Generate unique ID
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 15);
    std::stringstream ss;
    ss << std::hex;
    for (int i = 0; i < 8; ++i) {
        ss << dis(gen);
    }
    id_ = ss.str();
    
    // Initialize components
    process_registry_ = ProcessRegistry::New(shared_from_this());
    root_ = std::make_shared<RootContext>(shared_from_this());
    event_stream_ = eventstream::EventStream::New();
    guardians_ = Guardians::New(shared_from_this());
    dead_letter_ = DeadLetterProcess::New(shared_from_this());
    extensions_ = Extensions::New();
    
    // Register event stream process
    // This would be done in a real implementation
}

std::shared_ptr<ProcessRegistry> ActorSystem::GetProcessRegistry() const {
    return process_registry_;
}

std::shared_ptr<RootContext> ActorSystem::GetRoot() const {
    return root_;
}

std::shared_ptr<eventstream::EventStream> ActorSystem::GetEventStream() const {
    return event_stream_;
}

std::shared_ptr<DeadLetterProcess> ActorSystem::GetDeadLetter() const {
    return dead_letter_;
}

std::shared_ptr<Extensions> ActorSystem::GetExtensions() const {
    return extensions_;
}

std::string ActorSystem::GetID() const {
    return id_;
}

std::string ActorSystem::Address() const {
    return process_registry_->Address();
}

std::shared_ptr<PID> ActorSystem::NewLocalPID(const std::string& id) {
    return NewPID(process_registry_->Address(), id);
}

void ActorSystem::Shutdown() {
    stopped_.store(true);
}

bool ActorSystem::IsStopped() const {
    return stopped_.load();
}

} // namespace protoactor
