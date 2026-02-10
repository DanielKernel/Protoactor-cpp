#ifndef PROTOACTOR_LOG_LOG_H
#define PROTOACTOR_LOG_LOG_H

#include <string>
#include <memory>

#ifdef PROTOACTOR_USE_SPDLOG
    #include <spdlog/spdlog.h>
    #include <spdlog/logger.h>
    namespace protoactor {
    namespace log {
        using Logger = std::shared_ptr<spdlog::logger>;
        
        inline Logger GetLogger(const std::string& name) {
            return spdlog::get(name);
        }
        
        inline Logger GetDefaultLogger() {
            return spdlog::default_logger();
        }
        
        inline void SetLevel(spdlog::level::level_enum level) {
            spdlog::set_level(level);
        }
    }
    }
#else
    // Basic logging implementation (fallback)
    namespace protoactor {
    namespace log {
        class LoggerImpl {
        public:
            void info(const std::string& msg) { /* Basic implementation */ }
            void debug(const std::string& msg) { /* Basic implementation */ }
            void warn(const std::string& msg) { /* Basic implementation */ }
            void error(const std::string& msg) { /* Basic implementation */ }
            template<typename... Args>
            void info(const std::string& fmt, Args... args) { /* Basic implementation */ }
            template<typename... Args>
            void debug(const std::string& fmt, Args... args) { /* Basic implementation */ }
            template<typename... Args>
            void warn(const std::string& fmt, Args... args) { /* Basic implementation */ }
            template<typename... Args>
            void error(const std::string& fmt, Args... args) { /* Basic implementation */ }
        };
        
        using Logger = std::shared_ptr<LoggerImpl>;
        
        inline Logger GetLogger(const std::string& name) {
            return std::make_shared<LoggerImpl>();
        }
        
        inline Logger GetDefaultLogger() {
            return std::make_shared<LoggerImpl>();
        }
    }
    }
#endif

#endif // PROTOACTOR_LOG_LOG_H
