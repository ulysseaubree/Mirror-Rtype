cmake_minimum_required(VERSION 3.16)
project(rtype)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Trouver SFML
find_package(SFML 2.5 COMPONENTS graphics window system REQUIRED)

# Fichiers sources
set(SOURCES
    main.cpp
)

# Créer l'exécutable
add_executable(${PROJECT_NAME} ${SOURCES})

# Lier SFML
target_link_libraries(${PROJECT_NAME} 
    sfml-graphics 
    sfml-window 
    sfml-system
)

# Inclure les headers ECS
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

# Pour Windows, copier les DLLs SFML
if(WIN32)
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        set(SFML_DLL_SUFFIX "-d")
    endif()
    
    foreach(lib graphics window system)
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${SFML_ROOT}/bin/sfml-${lib}${SFML_DLL_SUFFIX}-2.dll"
            $<TARGET_FILE_DIR:${PROJECT_NAME}>)
    endforeach()
endif()