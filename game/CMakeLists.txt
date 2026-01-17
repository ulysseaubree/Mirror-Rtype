# --- GAME EXECUTABLES ---

# 1. CLIENT
file(GLOB_RECURSE CLIENT_SOURCES "client/*.cpp")

add_executable(r-type_client ${CLIENT_SOURCES})

# Lien avec tes libs (engine) et SFML
# Note: Assurez-vous que les cibles rtype_ecs et rtype_network existent bien dans lib/CMakeLists.txt
# Si vous n'avez qu'une seule lib "rtypesdk", remplacez ces deux lignes par rtypesdk
target_link_libraries(r-type_client PRIVATE 
    rtype_ecs 
    rtype_network
    SFML::Graphics SFML::Window SFML::System SFML::Audio
)

# 2. SERVER
file(GLOB_RECURSE SERVER_SOURCES "server/*.cpp")

add_executable(r-type_server ${SERVER_SOURCES})

# Lien avec tes libs (engine) et les Threads
target_link_libraries(r-type_server PRIVATE 
    rtype_ecs 
    rtype_network 
    Threads::Threads
)

# Installation (Copie les binaires dans un dossier accessible)
set_target_properties(r-type_client r-type_server PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)
