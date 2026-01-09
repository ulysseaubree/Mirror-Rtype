/*
** EPITECH PROJECT, 2025
** r-type
** File description:
** Script Engine Implementation
*/

#ifdef ENABLE_LUA_SCRIPTING

#include "script_engine.hpp"
#include "lua_bindings.hpp"
#include <iostream>
#include <fstream>

namespace ecs {

ScriptEngine::ScriptEngine() = default;

ScriptEngine::~ScriptEngine() {
    shutdown();
}

ScriptEngine::ScriptEngine(ScriptEngine&& other) noexcept
    : m_luaState(other.m_luaState)
    , m_scriptsPath(std::move(other.m_scriptsPath))
    , m_lastError(std::move(other.m_lastError))
    , m_loadedScripts(std::move(other.m_loadedScripts))
{
    other.m_luaState = nullptr;
}

ScriptEngine& ScriptEngine::operator=(ScriptEngine&& other) noexcept {
    if (this != &other) {
        shutdown();
        m_luaState = other.m_luaState;
        m_scriptsPath = std::move(other.m_scriptsPath);
        m_lastError = std::move(other.m_lastError);
        m_loadedScripts = std::move(other.m_loadedScripts);
        other.m_luaState = nullptr;
    }
    return *this;
}

bool ScriptEngine::init(const std::string& scriptsPath) {
    if (m_luaState) {
        shutdown();
    }
    
    m_scriptsPath = scriptsPath;
    
    // Create new Lua state
    m_luaState = luaL_newstate();
    if (!m_luaState) {
        m_lastError = "Failed to create Lua state";
        return false;
    }
    
    // Setup environment and register bindings
    setupEnvironment();
    registerBindings();
    
    std::cout << "[ScriptEngine] Initialized with scripts path: " << m_scriptsPath << std::endl;
    return true;
}

void ScriptEngine::shutdown() {
    if (m_luaState) {
        lua_close(m_luaState);
        m_luaState = nullptr;
    }
    m_loadedScripts.clear();
    std::cout << "[ScriptEngine] Shutdown complete" << std::endl;
}

void ScriptEngine::setupEnvironment() {
    // Open standard libraries (safe subset)
    luaL_openlibs(m_luaState);
    
    // Remove potentially dangerous functions for sandboxing
    // In a production environment, you'd want to be more restrictive
    lua_pushnil(m_luaState);
    lua_setglobal(m_luaState, "os");  // Remove os library (file access, etc.)
    
    lua_pushnil(m_luaState);
    lua_setglobal(m_luaState, "io");  // Remove io library (file I/O)
    
    lua_pushnil(m_luaState);
    lua_setglobal(m_luaState, "loadfile");  // Remove loadfile
    
    lua_pushnil(m_luaState);
    lua_setglobal(m_luaState, "dofile");  // Remove dofile
    
    // Set up package path for require()
    std::string packagePath = m_scriptsPath + "?.lua;" + m_scriptsPath + "lib/?.lua";
    lua_getglobal(m_luaState, "package");
    lua_pushstring(m_luaState, packagePath.c_str());
    lua_setfield(m_luaState, -2, "path");
    lua_pop(m_luaState, 1);
}

void ScriptEngine::registerBindings() {
    LuaBindings::registerAll(m_luaState);
}

std::string ScriptEngine::getFullPath(const std::string& scriptPath) const {
    // If it's already an absolute path, use it
    if (!scriptPath.empty() && scriptPath[0] == '/') {
        return scriptPath;
    }
    return m_scriptsPath + scriptPath;
}

bool ScriptEngine::loadScript(const std::string& scriptPath) {
    if (!m_luaState) {
        m_lastError = "Script engine not initialized";
        return false;
    }
    
    std::string fullPath = getFullPath(scriptPath);
    
    // Check if file exists
    std::ifstream file(fullPath);
    if (!file.good()) {
        m_lastError = "Script file not found: " + fullPath;
        std::cerr << "[ScriptEngine] " << m_lastError << std::endl;
        return false;
    }
    file.close();
    
    // Load and execute the script
    if (luaL_dofile(m_luaState, fullPath.c_str()) != LUA_OK) {
        handleError("Loading script " + scriptPath);
        return false;
    }
    
    m_loadedScripts[scriptPath] = true;
    std::cout << "[ScriptEngine] Loaded script: " << scriptPath << std::endl;
    return true;
}

bool ScriptEngine::isScriptLoaded(const std::string& scriptPath) const {
    auto it = m_loadedScripts.find(scriptPath);
    return it != m_loadedScripts.end() && it->second;
}

bool ScriptEngine::reloadScript(const std::string& scriptPath) {
    m_loadedScripts.erase(scriptPath);
    return loadScript(scriptPath);
}

void ScriptEngine::reloadAllScripts() {
    std::vector<std::string> scripts;
    for (const auto& [path, loaded] : m_loadedScripts) {
        if (loaded) {
            scripts.push_back(path);
        }
    }
    
    m_loadedScripts.clear();
    
    for (const auto& path : scripts) {
        loadScript(path);
    }
}

bool ScriptEngine::callEntityFunction(const std::string& scriptPath,
                                      const std::string& funcName,
                                      Entity entity) {
    if (!m_luaState) {
        m_lastError = "Script engine not initialized";
        return false;
    }
    
    // Ensure script is loaded
    if (!isScriptLoaded(scriptPath)) {
        if (!loadScript(scriptPath)) {
            return false;
        }
    }
    
    // Get the function
    lua_getglobal(m_luaState, funcName.c_str());
    if (!lua_isfunction(m_luaState, -1)) {
        lua_pop(m_luaState, 1);
        // Function doesn't exist - this is OK, not all scripts define all callbacks
        return true;
    }
    
    // Push entity ID as argument
    lua_pushinteger(m_luaState, static_cast<lua_Integer>(entity));
    
    // Call the function (1 argument, 0 results)
    if (lua_pcall(m_luaState, 1, 0, 0) != LUA_OK) {
        handleError("Calling " + funcName + " in " + scriptPath);
        return false;
    }
    
    return true;
}

bool ScriptEngine::callUpdate(const std::string& scriptPath, Entity entity, float dt) {
    if (!m_luaState) {
        return false;
    }
    
    if (!isScriptLoaded(scriptPath)) {
        if (!loadScript(scriptPath)) {
            return false;
        }
    }
    
    lua_getglobal(m_luaState, "on_update");
    if (!lua_isfunction(m_luaState, -1)) {
        lua_pop(m_luaState, 1);
        return true;  // No on_update defined is OK
    }
    
    lua_pushinteger(m_luaState, static_cast<lua_Integer>(entity));
    lua_pushnumber(m_luaState, dt);
    
    if (lua_pcall(m_luaState, 2, 0, 0) != LUA_OK) {
        handleError("Calling on_update in " + scriptPath);
        return false;
    }
    
    return true;
}

bool ScriptEngine::callCollision(const std::string& scriptPath, Entity entity, Entity other) {
    if (!m_luaState) {
        return false;
    }
    
    if (!isScriptLoaded(scriptPath)) {
        if (!loadScript(scriptPath)) {
            return false;
        }
    }
    
    lua_getglobal(m_luaState, "on_collision");
    if (!lua_isfunction(m_luaState, -1)) {
        lua_pop(m_luaState, 1);
        return true;
    }
    
    lua_pushinteger(m_luaState, static_cast<lua_Integer>(entity));
    lua_pushinteger(m_luaState, static_cast<lua_Integer>(other));
    
    if (lua_pcall(m_luaState, 2, 0, 0) != LUA_OK) {
        handleError("Calling on_collision in " + scriptPath);
        return false;
    }
    
    return true;
}

bool ScriptEngine::callDamage(const std::string& scriptPath, Entity entity, int amount) {
    if (!m_luaState) {
        return false;
    }
    
    if (!isScriptLoaded(scriptPath)) {
        if (!loadScript(scriptPath)) {
            return false;
        }
    }
    
    lua_getglobal(m_luaState, "on_damage");
    if (!lua_isfunction(m_luaState, -1)) {
        lua_pop(m_luaState, 1);
        return true;
    }
    
    lua_pushinteger(m_luaState, static_cast<lua_Integer>(entity));
    lua_pushinteger(m_luaState, amount);
    
    if (lua_pcall(m_luaState, 2, 0, 0) != LUA_OK) {
        handleError("Calling on_damage in " + scriptPath);
        return false;
    }
    
    return true;
}

void ScriptEngine::setGlobalNumber(const std::string& name, double value) {
    if (m_luaState) {
        lua_pushnumber(m_luaState, value);
        lua_setglobal(m_luaState, name.c_str());
    }
}

void ScriptEngine::setGlobalString(const std::string& name, const std::string& value) {
    if (m_luaState) {
        lua_pushstring(m_luaState, value.c_str());
        lua_setglobal(m_luaState, name.c_str());
    }
}

void ScriptEngine::setGlobalBool(const std::string& name, bool value) {
    if (m_luaState) {
        lua_pushboolean(m_luaState, value ? 1 : 0);
        lua_setglobal(m_luaState, name.c_str());
    }
}

void ScriptEngine::handleError(const std::string& context) {
    if (m_luaState) {
        m_lastError = lua_tostring(m_luaState, -1);
        lua_pop(m_luaState, 1);
    } else {
        m_lastError = "Unknown error (no Lua state)";
    }
    std::cerr << "[ScriptEngine] Error in " << context << ": " << m_lastError << std::endl;
}

} // namespace ecs

#endif // ENABLE_LUA_SCRIPTING
