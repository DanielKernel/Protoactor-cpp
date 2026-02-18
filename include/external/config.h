#ifndef PROTOACTOR_CONFIG_H
#define PROTOACTOR_CONFIG_H

#include <memory>
#include <string>
#include <functional>
#include <chrono>

namespace protoactor {

// Forward declarations
class ActorSystem;
class Logger;

/**
 * @brief Configuration for ActorSystem.
 */
class Config {
public:
    bool metrics_enabled;
    bool dead_letter_request_logging;
    int dead_letter_throttle_count;
    std::chrono::milliseconds dead_letter_throttle_interval;
    std::function<std::shared_ptr<Logger>(std::shared_ptr<ActorSystem>)> logger_factory;
    
    Config();
    
    /**
     * @brief Create default configuration.
     * @return Default config
     */
    static std::shared_ptr<Config> Default();
};

} // namespace protoactor

#endif // PROTOACTOR_CONFIG_H
