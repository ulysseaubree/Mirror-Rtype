/*
** EPITECH PROJECT, 2025
** r-type
** File description:
** Script Engine - Manages Lua state and script execution
*/

#pragma once

#ifdef ENABLE_LUA_SCRIPTING

extern "C" {
    #include <lua.h>
    #include <lauxlib.h>
    #include <lualib.h>
}

#include <string>
#include <unordered_map>
#include <memory>
#include <functional>
#include <filesystem>
#include "types.hpp"

namespace ecs {

/**
 * @brief Manages Lua scripting for the R-Type game engine.
 * 
 * The ScriptEngine is responsible for:
 * - Loading and caching Lua scripts
 * - Providing a safe execution environment
 * - Exposing C++ functions to Lua (via LuaBindings)
 * - Managing per-entity script states
 * 
 * Usage:
 *   ScriptEngine engine;
 *   engine.init("scripts/");
 *   engine.loadScript("enemies/bydos_wave.lua");
 *   engine.callFunction("on_update", entityId, deltaTime);
 */
class ScriptEngine {
public:
    ScriptEngine();
    ~ScriptEngine();
    
    // Disable copy (Lua state is not copyable)
    ScriptEngine(const ScriptEngine&) = delete;
    ScriptEngine& operator=(const ScriptEngine&) = delete;
    
    // Move semantics
    ScriptEngine(ScriptEngine&& other) noexcept;
    ScriptEngine& operator=(ScriptEngine&& other) noexcept;
    
    /**
     * @brief Initialize the script engine with a base path for scripts.
     * @param scriptsPath Base directory for Lua scripts
     * @return true if initialization succeeded
     */
    bool init(const std::string& scriptsPath = "scripts/");
    
    /**
     * @brief Shutdown the script engine and release all resources.
     */
    void shutdown();
    
    /**
     * @brief Check if the engine is initialized.
     */
    bool isInitialized() const { return m_luaState != nullptr; }
    
    /**
     * @brief Load a Lua script file.
     * @param scriptPath Path relative to the scripts base directory
     * @return true if the script loaded successfully
     */
    bool loadScript(const std::string& scriptPath);
    
    /**
     * @brief Check if a script is already loaded.
     */
    bool isScriptLoaded(const std::string& scriptPath) const;
    
    /**
     * @brief Reload a script (useful for hot-reloading during development).
     */
    bool reloadScript(const std::string& scriptPath);
    
    /**
     * @brief Reload all loaded scripts.
     */
    void reloadAllScripts();
    
    // === Script Execution ===
    
    /**
     * @brief Call a Lua function with an entity ID.
     * @param scriptPath The script containing the function
     * @param funcName Function name (e.g., "on_init", "on_update")
     * @param entity The entity ID to pass to the function
     * @return true if the function executed successfully
     */
    bool callEntityFunction(const std::string& scriptPath, 
                           const std::string& funcName, 
                           Entity entity);
    
    /**
     * @brief Call on_update(entity, dt) for a script.
     */
    bool callUpdate(const std::string& scriptPath, Entity entity, float dt);
    
    /**
     * @brief Call on_collision(entity, other) for a script.
     */
    bool callCollision(const std::string& scriptPath, Entity entity, Entity other);
    
    /**
     * @brief Call on_damage(entity, amount) for a script.
     */
    bool callDamage(const std::string& scriptPath, Entity entity, int amount);
    
    // === Variable Access ===
    
    /**
     * @brief Set a global variable accessible from all scripts.
     */
    void setGlobalNumber(const std::string& name, double value);
    void setGlobalString(const std::string& name, const std::string& value);
    void setGlobalBool(const std::string& name, bool value);
    
    /**
     * @brief Get the raw Lua state (for advanced usage).
     */
    lua_State* getLuaState() { return m_luaState; }
    
    /**
     * @brief Get the last error message.
     */
    const std::string& getLastError() const { return m_lastError; }
    
    /**
     * @brief Get the scripts base path.
     */
    const std::string& getScriptsPath() const { return m_scriptsPath; }

private:
    lua_State* m_luaState{nullptr};
    std::string m_scriptsPath{"scripts/"};
    std::string m_lastError{};
    
    // Cache of loaded script paths
    std::unordered_map<std::string, bool> m_loadedScripts;
    
    // Setup the Lua environment (sandboxing, libraries)
    void setupEnvironment();
    
    // Register all C++ bindings
    void registerBindings();
    
    // Error handling helper
    void handleError(const std::string& context);
    
    // Get full path for a script
    std::string getFullPath(const std::string& scriptPath) const;
};

/**
 * @brief Global script engine instance.
 * 
 * Similar to gCoordinator, this provides a convenient global access point
 * for the scripting system. Initialize it once at startup.
 */
inline ScriptEngine gScriptEngine{};

} // namespace ecs

#endif // ENABLE_LUA_SCRIPTING
