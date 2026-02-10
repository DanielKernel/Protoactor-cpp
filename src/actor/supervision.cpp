#include "protoactor/supervision.h"
#include "protoactor/actor_system.h"
#include "protoactor/pid.h"
#include <chrono>

namespace protoactor {

// RestartStatistics implementation
RestartStatistics::RestartStatistics()
    : failure_count(0),
      last_failure_time(std::chrono::system_clock::time_point::min()) {
}

void RestartStatistics::Reset() {
    failure_count = 0;
    last_failure_time = std::chrono::system_clock::time_point::min();
}

void RestartStatistics::Fail() {
    failure_count++;
    last_failure_time = std::chrono::system_clock::now();
}

// OneForOneStrategy implementation
class OneForOneStrategyImpl : public SupervisorStrategy {
public:
    OneForOneStrategyImpl(int max_retries, std::chrono::milliseconds within_duration, DeciderFunc decider)
        : max_retries_(max_retries),
          within_duration_(within_duration),
          decider_(decider) {
    }
    
    void HandleFailure(
        std::shared_ptr<ActorSystem> actor_system,
        std::shared_ptr<Supervisor> supervisor,
        std::shared_ptr<PID> child,
        std::shared_ptr<RestartStatistics> rs,
        std::shared_ptr<void> reason,
        std::shared_ptr<void> message) override {
        
        auto directive = decider_(reason);
        
        switch (directive) {
            case Directive::Resume:
                supervisor->ResumeChildren({child});
                break;
                
            case Directive::Restart:
                if (ShouldStop(rs)) {
                    supervisor->StopChildren({child});
                } else {
                    supervisor->RestartChildren({child});
                }
                break;
                
            case Directive::Stop:
                supervisor->StopChildren({child});
                break;
                
            case Directive::Escalate:
                supervisor->EscalateFailure(reason, message);
                break;
        }
    }

private:
    int max_retries_;
    std::chrono::milliseconds within_duration_;
    DeciderFunc decider_;
    
    bool ShouldStop(std::shared_ptr<RestartStatistics> rs) {
        if (!rs || rs->failure_count == 0) {
            return false;
        }
        
        auto now = std::chrono::system_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            now - rs->last_failure_time);
        
        if (elapsed > within_duration_) {
            rs->Reset();
            return false;
        }
        
        return rs->failure_count >= max_retries_;
    }
};

// AllForOneStrategy implementation
class AllForOneStrategyImpl : public SupervisorStrategy {
public:
    AllForOneStrategyImpl(int max_retries, std::chrono::milliseconds within_duration, DeciderFunc decider)
        : max_retries_(max_retries),
          within_duration_(within_duration),
          decider_(decider) {
    }
    
    void HandleFailure(
        std::shared_ptr<ActorSystem> actor_system,
        std::shared_ptr<Supervisor> supervisor,
        std::shared_ptr<PID> child,
        std::shared_ptr<RestartStatistics> rs,
        std::shared_ptr<void> reason,
        std::shared_ptr<void> message) override {
        
        auto directive = decider_(reason);
        auto children = supervisor->Children();
        
        switch (directive) {
            case Directive::Resume:
                supervisor->ResumeChildren({child});
                break;
                
            case Directive::Restart:
                if (ShouldStop(rs)) {
                    supervisor->StopChildren(children);
                } else {
                    supervisor->RestartChildren(children);
                }
                break;
                
            case Directive::Stop:
                supervisor->StopChildren(children);
                break;
                
            case Directive::Escalate:
                supervisor->EscalateFailure(reason, message);
                break;
        }
    }

private:
    int max_retries_;
    std::chrono::milliseconds within_duration_;
    DeciderFunc decider_;
    
    bool ShouldStop(std::shared_ptr<RestartStatistics> rs) {
        if (!rs || rs->failure_count == 0) {
            return false;
        }
        
        auto now = std::chrono::system_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            now - rs->last_failure_time);
        
        if (elapsed > within_duration_) {
            rs->Reset();
            return false;
        }
        
        return rs->failure_count >= max_retries_;
    }
};

// RestartingStrategy implementation
class RestartingStrategyImpl : public SupervisorStrategy {
public:
    void HandleFailure(
        std::shared_ptr<ActorSystem> actor_system,
        std::shared_ptr<Supervisor> supervisor,
        std::shared_ptr<PID> child,
        std::shared_ptr<RestartStatistics> rs,
        std::shared_ptr<void> reason,
        std::shared_ptr<void> message) override {
        
        supervisor->RestartChildren({child});
    }
};

std::shared_ptr<SupervisorStrategy> NewOneForOneStrategy(
    int max_retries,
    std::chrono::milliseconds within_duration,
    DeciderFunc decider) {
    return std::make_shared<OneForOneStrategyImpl>(max_retries, within_duration, decider);
}

std::shared_ptr<SupervisorStrategy> NewAllForOneStrategy(
    int max_retries,
    std::chrono::milliseconds within_duration,
    DeciderFunc decider) {
    return std::make_shared<AllForOneStrategyImpl>(max_retries, within_duration, decider);
}

std::shared_ptr<SupervisorStrategy> NewRestartingStrategy() {
    return std::make_shared<RestartingStrategyImpl>();
}

std::shared_ptr<SupervisorStrategy> DefaultSupervisorStrategy() {
    static auto strategy = NewOneForOneStrategy(
        10,
        std::chrono::milliseconds(10000),
        DefaultDecider);
    return strategy;
}

Directive DefaultDecider(std::shared_ptr<void> reason) {
    return Directive::Restart;
}

} // namespace protoactor
