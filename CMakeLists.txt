cmake_minimum_required(VERSION 3.16)
project(RType VERSION 1.0.0 LANGUAGES CXX)

# ========================================
# Configuration
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
option(USE_SFML "Enable SFML" ON)

# ========================================
# Dépendances
# ========================================

# SFML (pour le client et certains composants serveur)
find_package(SFML 3 COMPONENTS Graphics Window System Audio REQUIRED)
get_property(_allTargets GLOBAL PROPERTY TARGETS)
foreach(t ${_allTargets})
  if(t MATCHES "SFML|sfml")
    message(STATUS "Target visible: ${t}")
  endif()
endforeach()

# ASIO (header-only pour le réseau)
find_path(ASIO_INCLUDE_DIR asio.hpp
    PATHS
        /usr/include
        /usr/local/include
)

if(NOT ASIO_INCLUDE_DIR)
    message(STATUS "ASIO not found locally, downloading...")
    include(FetchContent)
    FetchContent_Declare(
        asio
        GIT_REPOSITORY https://github.com/chriskohlhoff/asio.git
        GIT_TAG asio-1-28-0
        GIT_SHALLOW TRUE
    )
    FetchContent_MakeAvailable(asio)
    set(ASIO_INCLUDE_DIR ${asio_SOURCE_DIR}/asio/include)
endif()

message(STATUS "ASIO include: ${ASIO_INCLUDE_DIR}")

# Threads (requis pour ASIO et le serveur)
find_package(Threads REQUIRED)

# ========================================
# Définitions globales
# ========================================
add_compile_definitions(
    ASIO_STANDALONE
    $<$<CONFIG:Debug>:DEBUG_BUILD>
    $<$<CONFIG:Release>:RELEASE_BUILD>
)

# ========================================
# Include directories ECS
# ========================================
set(ECS_INCLUDE_DIRS
    ${CMAKE_SOURCE_DIR}/ecs/core
    ${CMAKE_SOURCE_DIR}/ecs/game
    ${CMAKE_SOURCE_DIR}/ecs/network
    ${CMAKE_SOURCE_DIR}/ecs/utils
)

# ========================================
# Utils source
# ========================================
set(UTILS_SOURCES
    ${CMAKE_SOURCE_DIR}/ecs/utils/utils.cpp
)

# ========================================
# ECS Library (Static)
# ========================================
add_library(ecs_lib INTERFACE)

target_include_directories(ecs_lib INTERFACE
    ${ECS_INCLUDE_DIRS}
)

target_link_libraries(ecs_lib INTERFACE
    $<$<BOOL:${USE_SFML}>:SFML::Graphics>
    $<$<BOOL:${USE_SFML}>:SFML::System>
)

# ========================================
# NETWORK LIB (NEW)
# ========================================
set(NETWORK_SOURCES
    ${CMAKE_SOURCE_DIR}/network/src/threadQueue.cpp
    ${CMAKE_SOURCE_DIR}/network/src/udpClient.cpp
    ${CMAKE_SOURCE_DIR}/network/src/udpServer.cpp
    ${CMAKE_SOURCE_DIR}/network/src/protocol.cpp
)

add_library(network_lib STATIC ${NETWORK_SOURCES})
target_link_libraries(network_lib PUBLIC ecs_lib)

target_include_directories(network_lib PUBLIC
    ${CMAKE_SOURCE_DIR}/network/includes
)

# ========================================
# SERVER
# ========================================
if(BUILD_SERVER)
    file(GLOB_RECURSE SERVER_SOURCES 
        "${CMAKE_SOURCE_DIR}/server/*.cpp"
        "${CMAKE_SOURCE_DIR}/server/*.cc"
    )
    
    if(NOT SERVER_SOURCES)
        set(SERVER_SOURCES "${CMAKE_SOURCE_DIR}/server/main.cpp")
        message(WARNING "Pas de sources serveur trouvées, main.cpp sera nécessaire")
    endif()
    
    if(UTILS_SOURCES)
        list(APPEND SERVER_SOURCES ${UTILS_SOURCES})
    endif()
    
    add_executable(r-type_server ${SERVER_SOURCES})
    
    target_include_directories(r-type_server PRIVATE
        ${ECS_INCLUDE_DIRS}
        ${ASIO_INCLUDE_DIR}
        ${CMAKE_SOURCE_DIR}/server
        # optionnel, si tu veux inclure directement "udpServer.hpp" etc.
        ${CMAKE_SOURCE_DIR}/network/includes
    )
    
    target_link_libraries(r-type_server PRIVATE
        ecs_lib
        network_lib      # <--- IMPORTANT
        Threads::Threads
        SFML::Graphics
        SFML::System
    )
    
    set_target_properties(r-type_server PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    )
    
    message(STATUS "✓ Server target configured")
endif()

# ========================================
# CLIENT
# ========================================
if(BUILD_CLIENT)
    file(GLOB_RECURSE CLIENT_SOURCES 
        "${CMAKE_SOURCE_DIR}/client/*.cpp"
        "${CMAKE_SOURCE_DIR}/client/*.cc"
    )
    
    if(NOT CLIENT_SOURCES)
        set(CLIENT_SOURCES "${CMAKE_SOURCE_DIR}/client/main.cpp")
        message(WARNING "Pas de sources client trouvées, main.cpp sera nécessaire")
    endif()
    
    if(UTILS_SOURCES)
        list(APPEND CLIENT_SOURCES ${UTILS_SOURCES})
    endif()
    
    add_executable(r-type_client ${CLIENT_SOURCES})
    
    target_include_directories(r-type_client PRIVATE
        ${ECS_INCLUDE_DIRS}
        ${ASIO_INCLUDE_DIR}
        ${CMAKE_SOURCE_DIR}/client
        ${CMAKE_SOURCE_DIR}/network/includes
    )
    
    target_link_libraries(r-type_client PRIVATE
        ecs_lib
        network_lib
        Threads::Threads
        SFML::Graphics
        SFML::Window
        SFML::System
        SFML::Audio
    )
    
    set_target_properties(r-type_client PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    )
    
    message(STATUS "✓ Client target configured")
endif()

# ========================================
# Tests (optionnel)
# ========================================
if(BUILD_TESTS)
    enable_testing()
    message(STATUS "Tests enabled")
endif()

# ========================================
# Warnings
# ========================================
if(MSVC)
    set(WARNING_FLAGS /W4 /WX-)
else()
    set(WARNING_FLAGS -Wall -Wextra -Wpedantic -Wno-unused-parameter)
endif()

if(BUILD_SERVER)
    target_compile_options(r-type_server PRIVATE ${WARNING_FLAGS})
endif()

if(BUILD_CLIENT)
    target_compile_options(r-type_client PRIVATE ${WARNING_FLAGS})
endif()

# ========================================
# Installation
# ========================================
if(BUILD_SERVER)
    install(TARGETS r-type_server
        RUNTIME DESTINATION bin
    )
endif()

if(BUILD_CLIENT)
    install(TARGETS r-type_client
        RUNTIME DESTINATION bin
    )
    
    if(EXISTS "${CMAKE_SOURCE_DIR}/assets")
        install(DIRECTORY "${CMAKE_SOURCE_DIR}/assets"
            DESTINATION bin
        )
    endif()
endif()

# ========================================
# Documentation (optionnel)
# ========================================
option(BUILD_DOCS "Build documentation" OFF)
if(BUILD_DOCS)
    find_package(Doxygen)
    if(DOXYGEN_FOUND)
        set(DOXYGEN_IN ${CMAKE_SOURCE_DIR}/docs/Doxyfile.in)
        set(DOXYGEN_OUT ${CMAKE_BINARY_DIR}/Doxyfile)
        
        configure_file(${DOXYGEN_IN} ${DOXYGEN_OUT} @ONLY)
        
        add_custom_target(docs
            COMMAND ${DOXYGEN_EXECUTABLE} ${DOXYGEN_OUT}
            WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
            COMMENT "Generating API documentation with Doxygen"
            VERBATIM
        )
    endif()
endif()

# ========================================
# Résumé
# ========================================
message(STATUS "")
message(STATUS "========== R-Type Configuration ==========")
message(STATUS "Build type:       ${CMAKE_BUILD_TYPE}")
message(STATUS "C++ Standard:     ${CMAKE_CXX_STANDARD}")
message(STATUS "Compiler:         ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS "Build Server:     ${BUILD_SERVER}")
message(STATUS "Build Client:     ${BUILD_CLIENT}")
message(STATUS "Build Tests:      ${BUILD_TESTS}")
message(STATUS "Build Docs:       ${BUILD_DOCS}")
message(STATUS "SFML found:       ${SFML_FOUND}")
message(STATUS "ASIO include:     ${ASIO_INCLUDE_DIR}")
message(STATUS "==========================================")
message(STATUS "")
