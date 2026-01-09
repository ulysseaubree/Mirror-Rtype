/*
** EPITECH PROJECT, 2025
** r-type
** File description:
** Lua Bindings Implementation - C++ functions exposed to Lua
*/

#ifdef ENABLE_LUA_SCRIPTING

#include "lua_bindings.hpp"
#include "../ecs/core/ecs.hpp"
#include "../ecs/game/components.hpp"
#include <iostream>
#include <random>
#include <chrono>
#include <cmath>

namespace ecs {

// Game constants (should match your game window)
static constexpr float SCREEN_WIDTH = 800.f;
static constexpr float SCREEN_HEIGHT = 600.f;

// Start time for utils.get_time()
static auto g_startTime = std::chrono::steady_clock::now();

// Random number generator
static std::mt19937 g_rng{std::random_device{}()};

void LuaBindings::registerAll(lua_State* L) {
    registerEntityFunctions(L);
    registerSpawnFunctions(L);
    registerQueryFunctions(L);
    registerUtilityFunctions(L);
    registerGameFunctions(L);
    
    std::cout << "[LuaBindings] All bindings registered" << std::endl;
}

// ============================================================================
// Entity Functions
// ============================================================================

void LuaBindings::registerEntityFunctions(lua_State* L) {
    lua_newtable(L);
    
    lua_pushcfunction(L, lua_getPosition);
    lua_setfield(L, -2, "get_position");
    
    lua_pushcfunction(L, lua_setPosition);
    lua_setfield(L, -2, "set_position");
    
    lua_pushcfunction(L, lua_getVelocity);
    lua_setfield(L, -2, "get_velocity");
    
    lua_pushcfunction(L, lua_setVelocity);
    lua_setfield(L, -2, "set_velocity");
    
    lua_pushcfunction(L, lua_getHealth);
    lua_setfield(L, -2, "get_health");
    
    lua_pushcfunction(L, lua_setHealth);
    lua_setfield(L, -2, "set_health");
    
    lua_pushcfunction(L, lua_getTeam);
    lua_setfield(L, -2, "get_team");
    
    lua_pushcfunction(L, lua_destroyEntity);
    lua_setfield(L, -2, "destroy");
    
    lua_pushcfunction(L, lua_isEntityValid);
    lua_setfield(L, -2, "is_valid");
    
    lua_pushcfunction(L, lua_getRotation);
    lua_setfield(L, -2, "get_rotation");
    
    lua_pushcfunction(L, lua_setRotation);
    lua_setfield(L, -2, "set_rotation");
    
    lua_setglobal(L, "entity");
}

int LuaBindings::lua_getPosition(lua_State* L) {
    Entity id = static_cast<Entity>(luaL_checkinteger(L, 1));
    
    try {
        const auto& transform = gCoordinator.GetComponent<Transform>(id);
        lua_pushnumber(L, transform.x);
        lua_pushnumber(L, transform.y);
        return 2;
    } catch (...) {
        lua_pushnil(L);
        lua_pushnil(L);
        return 2;
    }
}

int LuaBindings::lua_setPosition(lua_State* L) {
    Entity id = static_cast<Entity>(luaL_checkinteger(L, 1));
    float x = static_cast<float>(luaL_checknumber(L, 2));
    float y = static_cast<float>(luaL_checknumber(L, 3));
    
    try {
        auto& transform = gCoordinator.GetComponent<Transform>(id);
        transform.x = x;
        transform.y = y;
        lua_pushboolean(L, 1);
    } catch (...) {
        lua_pushboolean(L, 0);
    }
    return 1;
}

int LuaBindings::lua_getVelocity(lua_State* L) {
    Entity id = static_cast<Entity>(luaL_checkinteger(L, 1));
    
    try {
        const auto& velocity = gCoordinator.GetComponent<Velocity>(id);
        lua_pushnumber(L, velocity.vx);
        lua_pushnumber(L, velocity.vy);
        return 2;
    } catch (...) {
        lua_pushnil(L);
        lua_pushnil(L);
        return 2;
    }
}

int LuaBindings::lua_setVelocity(lua_State* L) {
    Entity id = static_cast<Entity>(luaL_checkinteger(L, 1));
    float vx = static_cast<float>(luaL_checknumber(L, 2));
    float vy = static_cast<float>(luaL_checknumber(L, 3));
    
    try {
        auto& velocity = gCoordinator.GetComponent<Velocity>(id);
        velocity.vx = vx;
        velocity.vy = vy;
        lua_pushboolean(L, 1);
    } catch (...) {
        lua_pushboolean(L, 0);
    }
    return 1;
}

int LuaBindings::lua_getHealth(lua_State* L) {
    Entity id = static_cast<Entity>(luaL_checkinteger(L, 1));
    
    try {
        const auto& health = gCoordinator.GetComponent<Health>(id);
        lua_pushinteger(L, health.current);
        lua_pushinteger(L, health.max);
        return 2;
    } catch (...) {
        lua_pushnil(L);
        lua_pushnil(L);
        return 2;
    }
}

int LuaBindings::lua_setHealth(lua_State* L) {
    Entity id = static_cast<Entity>(luaL_checkinteger(L, 1));
    int current = static_cast<int>(luaL_checkinteger(L, 2));
    
    try {
        auto& health = gCoordinator.GetComponent<Health>(id);
        health.current = current;
        lua_pushboolean(L, 1);
    } catch (...) {
        lua_pushboolean(L, 0);
    }
    return 1;
}

int LuaBindings::lua_getTeam(lua_State* L) {
    Entity id = static_cast<Entity>(luaL_checkinteger(L, 1));
    
    try {
        const auto& team = gCoordinator.GetComponent<Team>(id);
        lua_pushinteger(L, team.teamID);
        return 1;
    } catch (...) {
        lua_pushnil(L);
        return 1;
    }
}

int LuaBindings::lua_destroyEntity(lua_State* L) {
    Entity id = static_cast<Entity>(luaL_checkinteger(L, 1));
    
    try {
        gCoordinator.DestroyEntity(id);
        lua_pushboolean(L, 1);
    } catch (...) {
        lua_pushboolean(L, 0);
    }
    return 1;
}

int LuaBindings::lua_isEntityValid(lua_State* L) {
    Entity id = static_cast<Entity>(luaL_checkinteger(L, 1));
    
    // Check if entity ID is within valid range
    // This is a simplified check - ideally EntityManager would expose a method
    bool valid = (id < MAX_ENTITIES);
    lua_pushboolean(L, valid ? 1 : 0);
    return 1;
}

int LuaBindings::lua_getRotation(lua_State* L) {
    Entity id = static_cast<Entity>(luaL_checkinteger(L, 1));
    
    try {
        const auto& transform = gCoordinator.GetComponent<Transform>(id);
        lua_pushnumber(L, transform.rotation);
        return 1;
    } catch (...) {
        lua_pushnil(L);
        return 1;
    }
}

int LuaBindings::lua_setRotation(lua_State* L) {
    Entity id = static_cast<Entity>(luaL_checkinteger(L, 1));
    float rotation = static_cast<float>(luaL_checknumber(L, 2));
    
    try {
        auto& transform = gCoordinator.GetComponent<Transform>(id);
        transform.rotation = rotation;
        lua_pushboolean(L, 1);
    } catch (...) {
        lua_pushboolean(L, 0);
    }
    return 1;
}

// ============================================================================
// Spawn Functions
// ============================================================================

void LuaBindings::registerSpawnFunctions(lua_State* L) {
    lua_newtable(L);
    
    lua_pushcfunction(L, lua_spawnProjectile);
    lua_setfield(L, -2, "projectile");
    
    lua_pushcfunction(L, lua_spawnEnemy);
    lua_setfield(L, -2, "enemy");
    
    lua_setglobal(L, "spawn");
}

int LuaBindings::lua_spawnProjectile(lua_State* L) {
    float x = static_cast<float>(luaL_checknumber(L, 1));
    float y = static_cast<float>(luaL_checknumber(L, 2));
    float vx = static_cast<float>(luaL_checknumber(L, 3));
    float vy = static_cast<float>(luaL_checknumber(L, 4));
    int team = static_cast<int>(luaL_optinteger(L, 5, 1));  // Default: enemy team
    int damage = static_cast<int>(luaL_optinteger(L, 6, 10));
    
    try {
        Entity projectile = gCoordinator.CreateEntity();
        
        Transform t{};
        t.x = x;
        t.y = y;
        gCoordinator.AddComponent(projectile, t);
        
        Velocity v{};
        v.vx = vx;
        v.vy = vy;
        gCoordinator.AddComponent(projectile, v);
        
        Team tm{};
        tm.teamID = team;
        gCoordinator.AddComponent(projectile, tm);
        
        Damager dmg{};
        dmg.damage = damage;
        gCoordinator.AddComponent(projectile, dmg);
        
        Lifetime lt{};
        lt.timeLeft = 5.f;
        gCoordinator.AddComponent(projectile, lt);
        
        Collider col{};
        col.radius = 5.f;
        gCoordinator.AddComponent(projectile, col);
        
        Boundary b{};
        b.destroy = true;
        gCoordinator.AddComponent(projectile, b);
        
        lua_pushinteger(L, static_cast<lua_Integer>(projectile));
        return 1;
    } catch (...) {
        lua_pushnil(L);
        return 1;
    }
}

int LuaBindings::lua_spawnEnemy(lua_State* L) {
    float x = static_cast<float>(luaL_checknumber(L, 1));
    float y = static_cast<float>(luaL_checknumber(L, 2));
    const char* scriptPath = luaL_optstring(L, 3, nullptr);
    
    try {
        Entity enemy = gCoordinator.CreateEntity();
        
        Transform t{};
        t.x = x;
        t.y = y;
        gCoordinator.AddComponent(enemy, t);
        
        Velocity v{};
        v.vx = -80.f;  // Default: move left
        v.vy = 0.f;
        gCoordinator.AddComponent(enemy, v);
        
        Team tm{};
        tm.teamID = 1;  // Enemy team
        gCoordinator.AddComponent(enemy, tm);
        
        Health h{};
        h.current = 1;
        h.max = 1;
        gCoordinator.AddComponent(enemy, h);
        
        Collider col{};
        col.radius = 15.f;
        gCoordinator.AddComponent(enemy, col);
        
        Boundary b{};
        b.destroy = true;
        b.minX = -100.f;
        gCoordinator.AddComponent(enemy, b);
        
        // If a script path is provided, attach ScriptComponent
        if (scriptPath) {
            ScriptComponent sc{};
            sc.scriptPath = scriptPath;
            gCoordinator.AddComponent(enemy, sc);
        }
        
        lua_pushinteger(L, static_cast<lua_Integer>(enemy));
        return 1;
    } catch (...) {
        lua_pushnil(L);
        return 1;
    }
}

// ============================================================================
// Query Functions
// ============================================================================

void LuaBindings::registerQueryFunctions(lua_State* L) {
    lua_newtable(L);
    
    lua_pushcfunction(L, lua_findNearestEnemy);
    lua_setfield(L, -2, "find_nearest_enemy");
    
    lua_pushcfunction(L, lua_findNearestPlayer);
    lua_setfield(L, -2, "find_nearest_player");
    
    lua_pushcfunction(L, lua_getDistance);
    lua_setfield(L, -2, "get_distance");
    
    lua_pushcfunction(L, lua_getEntitiesInRange);
    lua_setfield(L, -2, "get_entities_in_range");
    
    lua_setglobal(L, "query");
}

int LuaBindings::lua_findNearestEnemy(lua_State* L) {
    Entity selfId = static_cast<Entity>(luaL_checkinteger(L, 1));
    float range = static_cast<float>(luaL_optnumber(L, 2, 300.0));
    
    try {
        const auto& selfTransform = gCoordinator.GetComponent<Transform>(selfId);
        const auto& selfTeam = gCoordinator.GetComponent<Team>(selfId);
        
        Entity nearest = MAX_ENTITIES;
        float nearestDist = range;
        
        // This is a simplified search - in production you'd want spatial partitioning
        for (Entity e = 0; e < MAX_ENTITIES; ++e) {
            if (e == selfId) continue;
            
            try {
                const auto& otherTeam = gCoordinator.GetComponent<Team>(e);
                if (otherTeam.teamID == selfTeam.teamID) continue;
                
                const auto& otherTransform = gCoordinator.GetComponent<Transform>(e);
                float dx = otherTransform.x - selfTransform.x;
                float dy = otherTransform.y - selfTransform.y;
                float dist = std::sqrt(dx * dx + dy * dy);
                
                if (dist < nearestDist) {
                    nearestDist = dist;
                    nearest = e;
                }
            } catch (...) {
                continue;
            }
        }
        
        if (nearest != MAX_ENTITIES) {
            lua_pushinteger(L, static_cast<lua_Integer>(nearest));
        } else {
            lua_pushnil(L);
        }
        return 1;
    } catch (...) {
        lua_pushnil(L);
        return 1;
    }
}

int LuaBindings::lua_findNearestPlayer(lua_State* L) {
    Entity selfId = static_cast<Entity>(luaL_checkinteger(L, 1));
    float range = static_cast<float>(luaL_optnumber(L, 2, 300.0));
    
    try {
        const auto& selfTransform = gCoordinator.GetComponent<Transform>(selfId);
        
        Entity nearest = MAX_ENTITIES;
        float nearestDist = range;
        
        for (Entity e = 0; e < MAX_ENTITIES; ++e) {
            if (e == selfId) continue;
            
            try {
                // Check if it's a player (team 0)
                const auto& otherTeam = gCoordinator.GetComponent<Team>(e);
                if (otherTeam.teamID != 0) continue;
                
                const auto& otherTransform = gCoordinator.GetComponent<Transform>(e);
                float dx = otherTransform.x - selfTransform.x;
                float dy = otherTransform.y - selfTransform.y;
                float dist = std::sqrt(dx * dx + dy * dy);
                
                if (dist < nearestDist) {
                    nearestDist = dist;
                    nearest = e;
                }
            } catch (...) {
                continue;
            }
        }
        
        if (nearest != MAX_ENTITIES) {
            lua_pushinteger(L, static_cast<lua_Integer>(nearest));
        } else {
            lua_pushnil(L);
        }
        return 1;
    } catch (...) {
        lua_pushnil(L);
        return 1;
    }
}

int LuaBindings::lua_getDistance(lua_State* L) {
    Entity id1 = static_cast<Entity>(luaL_checkinteger(L, 1));
    Entity id2 = static_cast<Entity>(luaL_checkinteger(L, 2));
    
    try {
        const auto& t1 = gCoordinator.GetComponent<Transform>(id1);
        const auto& t2 = gCoordinator.GetComponent<Transform>(id2);
        
        float dx = t2.x - t1.x;
        float dy = t2.y - t1.y;
        float dist = std::sqrt(dx * dx + dy * dy);
        
        lua_pushnumber(L, dist);
        return 1;
    } catch (...) {
        lua_pushnil(L);
        return 1;
    }
}

int LuaBindings::lua_getEntitiesInRange(lua_State* L) {
    float x = static_cast<float>(luaL_checknumber(L, 1));
    float y = static_cast<float>(luaL_checknumber(L, 2));
    float range = static_cast<float>(luaL_checknumber(L, 3));
    int filterTeam = static_cast<int>(luaL_optinteger(L, 4, -1));  // -1 = all teams
    
    lua_newtable(L);
    int tableIndex = 1;
    
    for (Entity e = 0; e < MAX_ENTITIES; ++e) {
        try {
            if (filterTeam >= 0) {
                const auto& team = gCoordinator.GetComponent<Team>(e);
                if (team.teamID != filterTeam) continue;
            }
            
            const auto& transform = gCoordinator.GetComponent<Transform>(e);
            float dx = transform.x - x;
            float dy = transform.y - y;
            float dist = std::sqrt(dx * dx + dy * dy);
            
            if (dist <= range) {
                lua_pushinteger(L, static_cast<lua_Integer>(e));
                lua_rawseti(L, -2, tableIndex++);
            }
        } catch (...) {
            continue;
        }
    }
    
    return 1;
}

// ============================================================================
// Utility Functions
// ============================================================================

void LuaBindings::registerUtilityFunctions(lua_State* L) {
    lua_newtable(L);
    
    lua_pushcfunction(L, lua_log);
    lua_setfield(L, -2, "log");
    
    lua_pushcfunction(L, lua_random);
    lua_setfield(L, -2, "random");
    
    lua_pushcfunction(L, lua_getTime);
    lua_setfield(L, -2, "get_time");
    
    lua_setglobal(L, "utils");
}

int LuaBindings::lua_log(lua_State* L) {
    const char* message = luaL_checkstring(L, 1);
    std::cout << "[Lua] " << message << std::endl;
    return 0;
}

int LuaBindings::lua_random(lua_State* L) {
    float min = static_cast<float>(luaL_checknumber(L, 1));
    float max = static_cast<float>(luaL_checknumber(L, 2));
    
    std::uniform_real_distribution<float> dist(min, max);
    lua_pushnumber(L, dist(g_rng));
    return 1;
}

int LuaBindings::lua_getTime(lua_State* L) {
    auto now = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration<double>(now - g_startTime);
    lua_pushnumber(L, duration.count());
    return 1;
}

// ============================================================================
// Game State Functions
// ============================================================================

void LuaBindings::registerGameFunctions(lua_State* L) {
    lua_newtable(L);
    
    lua_pushcfunction(L, lua_getScreenWidth);
    lua_setfield(L, -2, "get_screen_width");
    
    lua_pushcfunction(L, lua_getScreenHeight);
    lua_setfield(L, -2, "get_screen_height");
    
    lua_pushcfunction(L, lua_getPlayerCount);
    lua_setfield(L, -2, "get_player_count");
    
    lua_setglobal(L, "game");
}

int LuaBindings::lua_getScreenWidth(lua_State* L) {
    lua_pushnumber(L, SCREEN_WIDTH);
    return 1;
}

int LuaBindings::lua_getScreenHeight(lua_State* L) {
    lua_pushnumber(L, SCREEN_HEIGHT);
    return 1;
}

int LuaBindings::lua_getPlayerCount(lua_State* L) {
    int count = 0;
    
    for (Entity e = 0; e < MAX_ENTITIES; ++e) {
        try {
            const auto& team = gCoordinator.GetComponent<Team>(e);
            if (team.teamID == 0) {
                count++;
            }
        } catch (...) {
            continue;
        }
    }
    
    lua_pushinteger(L, count);
    return 1;
}

} // namespace ecs

#endif // ENABLE_LUA_SCRIPTING
