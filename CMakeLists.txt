cmake_minimum_required(VERSION 3.16)

project(RType VERSION 1.0.0 LANGUAGES CXX)

# ========================================
# Configuration
# ========================================
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# ========================================
# Dépendances VCPKG
# ========================================
find_package(SFML CONFIG REQUIRED COMPONENTS Graphics Window System Audio)
find_package(asio CONFIG REQUIRED)
find_package(Threads REQUIRED)

add_compile_definitions(ASIO_STANDALONE _WIN32_WINNT=0x0601)

# ========================================
# Inclusion de la librairie (Moteur)
# ========================================
add_subdirectory(lib)

# ========================================
# Exécutables (Jeu)
# ========================================

# --- SERVER ---
# Vérifie si le dossier server existe et contient des sources
if(EXISTS "${CMAKE_SOURCE_DIR}/server")
    file(GLOB_RECURSE SERVER_SOURCES "server/*.cpp")
    if(SERVER_SOURCES)
        add_executable(r-type_server ${SERVER_SOURCES})
        # On lie simplement notre moteur aliasé
        target_link_libraries(r-type_server PRIVATE rtype::engine)
        message(STATUS "Build target addded: r-type_server")
    endif()
endif()

# --- CLIENT ---
# Vérifie si le dossier client existe et contient des sources
if(EXISTS "${CMAKE_SOURCE_DIR}/client")
    file(GLOB_RECURSE CLIENT_SOURCES "client/*.cpp")
    if(CLIENT_SOURCES)
        add_executable(r-type_client ${CLIENT_SOURCES})
        # Le client a besoin du moteur et des modules graphiques/audio SFML
        target_link_libraries(r-type_client PRIVATE 
            rtype::engine
            SFML::Graphics SFML::Window SFML::Audio
        )
        message(STATUS "Build target addded: r-type_client")
    endif()
endif()

add_subdirectory(game)
