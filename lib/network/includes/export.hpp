#pragma once

#if defined(_WIN32) || defined(_WIN64)
    #ifdef RTYPESDK_EXPORTS // Défini par CMake lors de la compilation de la lib
        #define RTYPE_API __declspec(dllexport)
    #else
        #define RTYPE_API __declspec(dllimport)
    #endif
#else
    #define RTYPE_API __attribute__((visibility("default")))
#endif
