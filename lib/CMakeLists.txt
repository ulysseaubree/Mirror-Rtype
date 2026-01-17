add_library(rtype_ecs INTERFACE)

target_include_directories(rtype_ecs INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/ecs
    ${CMAKE_CURRENT_SOURCE_DIR}/ecs/core
    ${CMAKE_CURRENT_SOURCE_DIR}/ecs/game
)

# Dépendances ECS (SFML pour les types comme Vector2f, etc.)
target_link_libraries(rtype_ecs INTERFACE 
    SFML::Graphics 
    SFML::System
)

# 2. Module Network (Contient du .cpp)
file(GLOB_RECURSE NETWORK_SOURCES "network/src/*.cpp")

add_library(rtype_network STATIC ${NETWORK_SOURCES})

# Includes pour le réseau (PUBLIC pour que le client/serveur y ait accès)
target_include_directories(rtype_network PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/network/includes
)

# Le réseau dépend de l'ECS (pour synchroniser les entités) et d'ASIO
target_link_libraries(rtype_network PUBLIC 
    rtype_ecs 
    asio::asio 
    Threads::Threads
)

if(MSVC)
    target_compile_options(rtype_network PRIVATE /W4)
else()
    target_compile_options(rtype_network PRIVATE -Wall -Wextra)
endif()

# 3. Cible globale (Optionnel, facilite l'inclusion)
# Permet de lier simplement "rtype::engine" dans le client/serveur
add_library(rtype_engine INTERFACE)
target_link_libraries(rtype_engine INTERFACE rtype_network rtype_ecs)
add_library(rtype::engine ALIAS rtype_engine)
