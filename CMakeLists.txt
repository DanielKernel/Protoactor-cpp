cmake_minimum_required(VERSION 3.10)
project(protoactor-cpp VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Platform detection (64-bit only)
if(CMAKE_SYSTEM_PROCESSOR MATCHES "aarch64" OR CMAKE_SYSTEM_PROCESSOR MATCHES "arm64")
    set(PLATFORM_ARM64 ON)
    message(STATUS "Building for ARM64 architecture")
elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64" OR CMAKE_SYSTEM_PROCESSOR MATCHES "amd64")
    set(PLATFORM_X86_64 ON)
    message(STATUS "Building for x86_64 architecture")
else()
    message(FATAL_ERROR "Unsupported architecture: ${CMAKE_SYSTEM_PROCESSOR}. Only x86_64 and ARM64 are supported.")
endif()

# Options
option(BUILD_EXAMPLES "Build examples" ON)
option(BUILD_TESTS "Build tests" ON)
option(BUILD_SHARED_LIBS "Build shared library" OFF)
option(ENABLE_COVERAGE "Enable code coverage (gcov/lcov)" OFF)

# Compiler-specific options (Linux only)
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(-Wall -Wextra -Wpedantic)
    if(CMAKE_BUILD_TYPE STREQUAL "Release")
        add_compile_options(-O3 -DNDEBUG)
    else()
        add_compile_options(-g -O0)
    endif()
endif()

# Include directories
include_directories(${CMAKE_SOURCE_DIR}/include)
include_directories(${CMAKE_BINARY_DIR}/proto)

# Find required packages
find_package(Threads REQUIRED)

# Optional dependencies
option(ENABLE_PROTOBUF "Enable Protobuf support (required for remote)" OFF)
option(ENABLE_GRPC "Enable gRPC support (required for remote)" OFF)
option(ENABLE_SPDLOG "Use spdlog for logging" ON)
option(ENABLE_GTEST "Enable Google Test for unit tests" OFF)
option(ENABLE_JSON "Enable JSON support (nlohmann/json)" OFF)

# Protobuf (required for remote communication)
if(ENABLE_PROTOBUF OR ENABLE_GRPC)
    find_package(Protobuf REQUIRED)
    
    # Protobuf files
    file(GLOB PROTO_FILES "proto/*.proto")
    if(PROTO_FILES)
        protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS ${PROTO_FILES})
        set(PROTO_GENERATED ON)
    endif()
endif()

# gRPC (required for remote communication)
if(ENABLE_GRPC)
    find_package(gRPC REQUIRED)
    if(NOT gRPC_FOUND)
        message(WARNING "gRPC not found. Remote communication will be limited.")
    endif()
endif()

# spdlog (recommended for logging)
if(ENABLE_SPDLOG)
    find_package(spdlog QUIET)
    if(spdlog_FOUND)
        message(STATUS "Using spdlog for logging")
        add_definitions(-DPROTOACTOR_USE_SPDLOG)
    else()
        message(STATUS "spdlog not found, using basic logging")
        add_definitions(-DPROTOACTOR_USE_BASIC_LOG)
    endif()
endif()

# Google Test (for unit tests)
if(ENABLE_GTEST)
    find_package(GTest QUIET)
    if(GTest_FOUND)
        message(STATUS "Google Test found, tests can be built")
        enable_testing()
    else()
        message(STATUS "Google Test not found, tests disabled")
    endif()
endif()

# nlohmann/json (for JSON serialization)
if(ENABLE_JSON)
    find_package(nlohmann_json QUIET)
    if(nlohmann_json_FOUND)
        message(STATUS "nlohmann/json found, JSON support enabled")
        add_definitions(-DPROTOACTOR_USE_JSON)
    else()
        message(STATUS "nlohmann/json not found, JSON support disabled")
    endif()
endif()

# Source files
set(ACTOR_SOURCES
    src/actor/pid.cpp
    src/actor/props.cpp
    src/actor/actor_system.cpp
    src/actor/process_registry.cpp
    src/actor/mailbox.cpp
    src/actor/thread_pool.cpp
    src/actor/dispatcher.cpp
    src/actor/future.cpp
    src/actor/messages.cpp
    src/actor/supervision.cpp
    src/actor/root_context.cpp
    src/actor/deadletter.cpp
    src/actor/guardian.cpp
    src/actor/queue.cpp
    src/actor/config.cpp
    src/actor/extensions.cpp
    src/actor/new_pid.cpp
    src/actor/actor_process.cpp
    src/actor/actor_context.cpp
    src/actor/platform.cpp
    src/actor/behavior.cpp
    src/actor/captured_context.cpp
    src/actor/middleware_chain.cpp
    src/actor/pidset.cpp
    src/actor/deduplication_context.cpp
)

set(QUEUE_SOURCES
    src/queue/priority_queue.cpp
)

set(REMOTE_SOURCES
    src/remote/remote.cpp
    src/remote/remote_process.cpp
    src/remote/serializer.cpp
    src/remote/endpoint_manager.cpp
    src/remote/endpoint_writer.cpp
    src/remote/endpoint_reader.cpp
    src/remote/proto_serializer.cpp
    src/remote/activator_actor.cpp
    src/remote/endpoint_watcher.cpp
    src/remote/blocklist.cpp
)

# Add gRPC service if gRPC is enabled
if(ENABLE_GRPC AND gRPC_FOUND)
    list(APPEND REMOTE_SOURCES src/remote/grpc_service.cpp)
endif()

# Add JSON serializer if JSON is enabled
if(ENABLE_JSON AND nlohmann_json_FOUND)
    list(APPEND REMOTE_SOURCES src/remote/json_serializer.cpp)
endif()

set(CLUSTER_SOURCES
    src/cluster/cluster.cpp
    src/cluster/member.cpp
    src/cluster/member_list.cpp
    src/cluster/pid_cache.cpp
    src/cluster/gossiper.cpp
    src/cluster/pubsub.cpp
    src/cluster/pubsub_delivery.cpp
    src/cluster/gossip.cpp
    src/cluster/identity_lookup.cpp
    src/cluster/cluster_provider.cpp
)

set(ROUTER_SOURCES
    src/router/router.cpp
    src/router/router_group.cpp
)

set(PERSISTENCE_SOURCES
    src/persistence/persistence.cpp
)

set(SCHEDULER_SOURCES
    src/scheduler/timer.cpp
)

set(STREAM_SOURCES
    src/stream/stream.cpp
)

set(METRICS_SOURCES
    src/metrics/metrics.cpp
)

set(TESTKIT_SOURCES
    src/testkit/testprobe.cpp
)

set(EVENTSTREAM_SOURCES
    src/actor/eventstream.cpp
)

# Main library
if(BUILD_SHARED_LIBS)
    add_library(protoactor-cpp SHARED
        ${ACTOR_SOURCES}
        ${REMOTE_SOURCES}
        ${CLUSTER_SOURCES}
        ${ROUTER_SOURCES}
        ${PERSISTENCE_SOURCES}
        ${SCHEDULER_SOURCES}
        ${STREAM_SOURCES}
        ${EVENTSTREAM_SOURCES}
        ${METRICS_SOURCES}
        ${TESTKIT_SOURCES}
        ${QUEUE_SOURCES}
    )
else()
    add_library(protoactor-cpp STATIC
        ${ACTOR_SOURCES}
        ${REMOTE_SOURCES}
        ${CLUSTER_SOURCES}
        ${ROUTER_SOURCES}
        ${PERSISTENCE_SOURCES}
        ${SCHEDULER_SOURCES}
        ${STREAM_SOURCES}
        ${EVENTSTREAM_SOURCES}
        ${METRICS_SOURCES}
        ${TESTKIT_SOURCES}
        ${QUEUE_SOURCES}
    )
endif()

# Link libraries
target_link_libraries(protoactor-cpp
    Threads::Threads
)

if(ENABLE_COVERAGE AND CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(protoactor-cpp PRIVATE --coverage)
    target_link_libraries(protoactor-cpp --coverage)
endif()

# Add protobuf if enabled
if(ENABLE_PROTOBUF AND PROTO_GENERATED)
    target_sources(protoactor-cpp PRIVATE ${PROTO_SRCS})
    target_link_libraries(protoactor-cpp
        ${Protobuf_LIBRARIES}
    )
endif()

# Add gRPC if enabled
if(ENABLE_GRPC AND gRPC_FOUND)
    target_link_libraries(protoactor-cpp
        gRPC::grpc++
        gRPC::grpc++_reflection
    )
endif()

# Add spdlog if enabled
if(ENABLE_SPDLOG AND spdlog_FOUND)
    target_link_libraries(protoactor-cpp
        spdlog::spdlog
    )
endif()

# Add nlohmann/json if enabled
if(ENABLE_JSON AND nlohmann_json_FOUND)
    target_link_libraries(protoactor-cpp
        nlohmann_json::nlohmann_json
    )
endif()

target_include_directories(protoactor-cpp PUBLIC
    ${CMAKE_SOURCE_DIR}/include
)

if(ENABLE_PROTOBUF AND PROTO_GENERATED)
    target_include_directories(protoactor-cpp PUBLIC
        ${CMAKE_BINARY_DIR}/proto
    )
endif()

# Platform-specific settings (64-bit only)
if(PLATFORM_ARM64)
    message(STATUS "Using ARM64 optimizations")
    # ARM64 specific flags
    add_compile_options(-march=armv8-a)
elseif(PLATFORM_X86_64)
    message(STATUS "Using x86_64 optimizations")
    # Enable SSE/AVX if available
    include(CheckCXXCompilerFlag)
    check_cxx_compiler_flag("-march=native" COMPILER_SUPPORTS_MARCH_NATIVE)
    if(COMPILER_SUPPORTS_MARCH_NATIVE AND CMAKE_BUILD_TYPE STREQUAL "Release")
        add_compile_options(-march=native)
    else()
        add_compile_options(-march=x86-64 -mtune=generic)
    endif()
endif()

# Examples
if(BUILD_EXAMPLES)
    # Hello World example
    add_executable(hello_world examples/hello_world.cpp)
    target_link_libraries(hello_world protoactor-cpp)
    
    # Supervision example
    add_executable(supervision_example examples/supervision_example.cpp)
    target_link_libraries(supervision_example protoactor-cpp)
    
    # Set output directory
    set_target_properties(hello_world supervision_example
        PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
    )
endif()

# Tests: unit tests per module + integration + performance (no GTest required)
if(BUILD_TESTS)
    enable_testing()
    set(TEST_OUTPUT_DIR ${CMAKE_BINARY_DIR}/tests)

    # Unit tests by module: (source_file, test_name, ctest_label_module)
    set(UNIT_TESTS
        tests/unit/pid_test.cpp::unit_pid::pid
        tests/unit/config_test.cpp::unit_config::config
        tests/unit/platform_test.cpp::unit_platform::platform
        tests/unit/queue_test.cpp::unit_queue::queue
        tests/unit/pidset_test.cpp::unit_pidset::pidset
        tests/unit/priority_queue_test.cpp::unit_priority_queue::priority_queue
        tests/unit/messages_test.cpp::unit_messages::messages
        tests/unit/thread_pool_test.cpp::thread_pool_test::thread_pool
        tests/unit/dispatcher_test.cpp::dispatcher_test::dispatcher
        tests/unit/extensions_test.cpp::unit_extensions::extensions
        tests/unit/props_test.cpp::unit_props::props
        tests/unit/eventstream_test.cpp::unit_eventstream::eventstream
    )
    foreach(_ent ${UNIT_TESTS})
        string(REPLACE "::" ";" _parts "${_ent}")
        list(GET _parts 0 _src)
        list(GET _parts 1 _name)
        list(GET _parts 2 _mod)
        add_executable(${_name} ${_src})
        target_include_directories(${_name} PRIVATE ${CMAKE_SOURCE_DIR})
        target_link_libraries(${_name} protoactor-cpp)
        set_target_properties(${_name} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${TEST_OUTPUT_DIR})
        add_test(NAME ${_name} COMMAND ${_name} WORKING_DIRECTORY ${TEST_OUTPUT_DIR})
        set_tests_properties(${_name} PROPERTIES
            ENVIRONMENT "PROTOACTOR_TEST=1"
            LABELS "unit;module:${_mod}"
        )
        if(ENABLE_COVERAGE AND CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
            target_compile_options(${_name} PRIVATE --coverage)
            target_link_libraries(${_name} --coverage)
        endif()
    endforeach()

    # Functional tests (integration + performance)
    add_executable(actor_integration_test tests/functional/actor_integration_test.cpp)
    target_include_directories(actor_integration_test PRIVATE ${CMAKE_SOURCE_DIR})
    target_link_libraries(actor_integration_test protoactor-cpp)
    set_target_properties(actor_integration_test PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${TEST_OUTPUT_DIR})
    add_test(NAME actor_integration_test COMMAND actor_integration_test WORKING_DIRECTORY ${TEST_OUTPUT_DIR})
    set_tests_properties(actor_integration_test PROPERTIES
        ENVIRONMENT "PROTOACTOR_TEST=1"
        LABELS "integration;module:actor"
    )
    if(ENABLE_COVERAGE AND CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(actor_integration_test PRIVATE --coverage)
        target_link_libraries(actor_integration_test --coverage)
    endif()

    add_executable(performance_test tests/functional/performance_test.cpp)
    target_include_directories(performance_test PRIVATE ${CMAKE_SOURCE_DIR})
    target_link_libraries(performance_test protoactor-cpp)
    set_target_properties(performance_test PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${TEST_OUTPUT_DIR})
    add_test(NAME performance_test COMMAND performance_test WORKING_DIRECTORY ${TEST_OUTPUT_DIR})
    set_tests_properties(performance_test PROPERTIES
        ENVIRONMENT "PROTOACTOR_TEST=1"
        LABELS "performance"
    )
    if(ENABLE_COVERAGE AND CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(performance_test PRIVATE --coverage)
        target_link_libraries(performance_test --coverage)
    endif()

    message(STATUS "Unit tests (by module): pid, config, platform, queue, pidset, priority_queue, messages, thread_pool, dispatcher, extensions, props, eventstream")
    message(STATUS "Run by module: ctest -L 'module:<name>' (e.g. ctest -L 'module:pid'); all unit: ctest -L unit")
endif()

# Installation
install(TARGETS protoactor-cpp
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
)

install(DIRECTORY include/protoactor
    DESTINATION include
    FILES_MATCHING PATTERN "*.h"
)

# Print configuration summary
message(STATUS "")
message(STATUS "=== ProtoActor C++ Configuration ===")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "C++ standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "Platform: ${CMAKE_SYSTEM_PROCESSOR}")
message(STATUS "Build examples: ${BUILD_EXAMPLES}")
message(STATUS "Build tests: ${BUILD_TESTS}")
message(STATUS "Protobuf enabled: ${ENABLE_PROTOBUF}")
message(STATUS "gRPC enabled: ${ENABLE_GRPC}")
message(STATUS "spdlog enabled: ${ENABLE_SPDLOG} (found: ${spdlog_FOUND})")
message(STATUS "Google Test enabled: ${ENABLE_GTEST} (found: ${GTest_FOUND})")
message(STATUS "JSON enabled: ${ENABLE_JSON} (found: ${nlohmann_json_FOUND})")
message(STATUS "Shared library: ${BUILD_SHARED_LIBS}")
message(STATUS "=====================================")
message(STATUS "")
