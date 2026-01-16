cmake_minimum_required(VERSION 3.16)
project(RType VERSION 1.0.0 LANGUAGES CXX)

# ========================================
# Configuration globale
# ========================================
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ========================================
# Options
# ========================================
option(BUILD_SERVER "Build the server" ON)
option(BUILD_CLIENT "Build the client" ON)
option(BUILD_TESTS "Build unit tests" OFF)
option(BUILD_DOCS "Build documentation" OFF)

# ========================================
# Dépendances (via vcpkg)
# ========================================
find_package(Threads REQUIRED)
find_package(SFML 3 COMPONENTS Graphics Window System Audio REQUIRED)
find_package(asio CONFIG REQUIRED)

# ========================================
# Définitions globales
# ========================================
add_compile_definitions(
    ASIO_STANDALONE
    $<$<CONFIG:Debug>:DEBUG_BUILD>
    $<$<CONFIG:Release>:RELEASE_BUILD>
)

# ========================================
# ECS (header-only)
# ========================================
set(ECS_INCLUDE_DIRS
    ${CMAKE_SOURCE_DIR}/ecs/core
    ${CMAKE_SOURCE_DIR}/ecs/game
    ${CMAKE_SOURCE_DIR}/ecs/network
    ${CMAKE_SOURCE_DIR}/ecs/utils
)

add_library(ecs_lib INTERFACE)
target_include_directories(ecs_lib INTERFACE ${ECS_INCLUDE_DIRS})
target_link_libraries(ecs_lib INTERFACE SFML::Graphics SFML::System)

# ========================================
# Network library
# ========================================
set(NETWORK_SOURCES
    ${CMAKE_SOURCE_DIR}/network/src/threadQueue.cpp
    ${CMAKE_SOURCE_DIR}/network/src/udpClient.cpp
    ${CMAKE_SOURCE_DIR}/network/src/udpServer.cpp
    ${CMAKE_SOURCE_DIR}/network/src/protocol.cpp
)

add_library(network_lib STATIC ${NETWORK_SOURCES})
target_include_directories(network_lib PUBLIC
    ${CMAKE_SOURCE_DIR}/network/includes
)
target_link_libraries(network_lib PUBLIC
    ecs_lib
    asio::asio
    Threads::Threads
)

# ========================================
# Warnings
# ========================================
if(MSVC)
    set(WARNING_FLAGS /W4)
else()
    set(WARNING_FLAGS -Wall -Wextra -Wpedantic)
endif()

# ========================================
# Server
# ========================================
if(BUILD_SERVER)
    file(GLOB_RECURSE SERVER_SOURCES ${CMAKE_SOURCE_DIR}/server/*.cpp)
    add_executable(r-type_server ${SERVER_SOURCES})

    target_include_directories(r-type_server PRIVATE ${CMAKE_SOURCE_DIR}/server)
    target_link_libraries(r-type_server PRIVATE
        ecs_lib
        network_lib
        SFML::Graphics
        SFML::System
    )

    target_compile_options(r-type_server PRIVATE ${WARNING_FLAGS})
    set_target_properties(r-type_server PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
    )
endif()

# ========================================
# Client
# ========================================
if(BUILD_CLIENT)
    file(GLOB_RECURSE CLIENT_SOURCES ${CMAKE_SOURCE_DIR}/client/*.cpp)
    add_executable(r-type_client ${CLIENT_SOURCES})

    target_include_directories(r-type_client PRIVATE ${CMAKE_SOURCE_DIR}/client)
    target_link_libraries(r-type_client PRIVATE
        ecs_lib
        network_lib
        SFML::Graphics
        SFML::Window
        SFML::System
        SFML::Audio
    )

    target_compile_options(r-type_client PRIVATE ${WARNING_FLAGS})
    set_target_properties(r-type_client PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
    )
endif()

# ========================================
# Tests
# ========================================
if(BUILD_TESTS)
    enable_testing()
endif()

# ========================================
# Documentation
# ========================================
if(BUILD_DOCS)
    find_package(Doxygen)
    if(DOXYGEN_FOUND)
        add_custom_target(docs
            COMMAND ${DOXYGEN_EXECUTABLE}
            WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        )
    endif()
endif()

# ========================================
# Résumé
# ========================================
message(STATUS "========== R-Type ==========")
message(STATUS "Build server: ${BUILD_SERVER}")
message(STATUS "Build client: ${BUILD_CLIENT}")
message(STATUS "Compiler: ${CMAKE_CXX_COMPILER_ID}")
message(STATUS "============================")
