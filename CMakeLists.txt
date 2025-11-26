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

# ========================================
# Dépendances
# ========================================

# SFML (pour le client)
if(BUILD_CLIENT)
    find_package(SFML 2.5 COMPONENTS graphics window system audio REQUIRED)
endif()

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
# SERVER
# ========================================
if(BUILD_SERVER)
    # Chercher les sources du serveur
    file(GLOB_RECURSE SERVER_SOURCES 
        "${CMAKE_SOURCE_DIR}/server/*.cpp"
        "${CMAKE_SOURCE_DIR}/server/*.cc"
    )
    
    # Si pas de sources, créer un main.cpp minimal
    if(NOT SERVER_SOURCES)
        set(SERVER_SOURCES "${CMAKE_SOURCE_DIR}/server/main.cpp")
        message(WARNING "Pas de sources serveur trouvées, main.cpp sera nécessaire")
    endif()
    
    # Ajouter Utils.cpp
    list(APPEND SERVER_SOURCES ${UTILS_SOURCES})
    
    add_executable(r-type_server ${SERVER_SOURCES})
    
    target_include_directories(r-type_server PRIVATE
        ${ECS_INCLUDE_DIRS}
        ${ASIO_INCLUDE_DIR}
        ${CMAKE_SOURCE_DIR}/server
    )
    
    target_link_libraries(r-type_server PRIVATE
        Threads::Threads
        sfml-graphics
        sfml-system
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
    # Chercher les sources du client
    file(GLOB_RECURSE CLIENT_SOURCES 
        "${CMAKE_SOURCE_DIR}/client/*.cpp"
        "${CMAKE_SOURCE_DIR}/client/*.cc"
    )
    
    # Si pas de sources, créer un main.cpp minimal
    if(NOT CLIENT_SOURCES)
        set(CLIENT_SOURCES "${CMAKE_SOURCE_DIR}/client/main.cpp")
        message(WARNING "Pas de sources client trouvées, main.cpp sera nécessaire")
    endif()
    
    # Ajouter Utils.cpp
    list(APPEND CLIENT_SOURCES ${UTILS_SOURCES})
    
    add_executable(r-type_client ${CLIENT_SOURCES})
    
    target_include_directories(r-type_client PRIVATE
        ${ECS_INCLUDE_DIRS}
        ${ASIO_INCLUDE_DIR}
        ${CMAKE_SOURCE_DIR}/client
    )
    
    target_link_libraries(r-type_client PRIVATE
        sfml-graphics
        sfml-window
        sfml-system
        sfml-audio
        Threads::Threads
    )
    
    set_target_properties(r-type_client PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    )
    
    message(STATUS "✓ Client target configured")
endif()

# ========================================
# Warnings
# ========================================
if(MSVC)
    target_compile_options(r-type_server PRIVATE /W4)
    if(BUILD_CLIENT)
        target_compile_options(r-type_client PRIVATE /W4)
    endif()
else()
    target_compile_options(r-type_server PRIVATE -Wall -Wextra -Wpedantic)
    if(BUILD_CLIENT)
        target_compile_options(r-type_client PRIVATE -Wall -Wextra -Wpedantic)
    endif()
endif()

# ========================================
# Installation
# ========================================
if (BUILD_SERVER)
    install(TARGETS r-type_server
        RUNTIME DESTINATION bin
    )
endif()

if (BUILD_CLIENT)
    install(TARGETS r-type_client
        RUNTIME DESTINATION bin
    )
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
message(STATUS "SFML found:       ${SFML_FOUND}")
message(STATUS "ASIO include:     ${ASIO_INCLUDE_DIR}")
message(STATUS "==========================================")
message(STATUS "")