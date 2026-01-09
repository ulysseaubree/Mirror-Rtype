/*
** EPITECH PROJECT, 2025
** r-type
** File description:
** Lua Bindings - Exposes ECS functions to Lua scripts
*/

#pragma once

#ifdef ENABLE_LUA_SCRIPTING

extern "C" {
    #include <lua.h>
    #include <lauxlib.h>
    #include <lualib.h>
}

namespace ecs {

/**
 * @brief Registers all C++ functions that Lua scripts can call.
 * 
 * The following functions are exposed to Lua:
 * 
 * Entity Functions:
 *   entity.get_position(id) -> x, y
 *   entity.set_position(id, x, y)
 *   entity.get_velocity(id) -> vx, vy
 *   entity.set_velocity(id, vx, vy)
 *   entity.get_health(id) -> current, max
 *   entity.set_health(id, current)
 *   entity.get_team(id) -> teamId
 *   entity.destroy(id)
 *   entity.is_valid(id) -> bool
 * 
 * Spawning Functions:
 *   spawn.projectile(x, y, vx, vy, team, damage) -> entityId
 *   spawn.enemy(x, y, scriptPath) -> entityId
 * 
 * Query Functions:
 *   query.find_nearest_enemy(id, range) -> entityId or nil
 *   query.find_nearest_player(id, range) -> entityId or nil
 *   query.get_distance(id1, id2) -> distance
 *   query.get_entities_in_range(x, y, range, team) -> {entityIds}
 * 
 * Utility Functions:
 *   utils.log(message)
 *   utils.random(min, max) -> number
 *   utils.get_time() -> seconds since start
 * 
 * Game State:
 *   game.get_screen_width() -> number
 *   game.get_screen_height() -> number
 *   game.get_player_count() -> number
 */
class LuaBindings {
public:
    /**
     * @brief Register all bindings to a Lua state.
     * @param L The Lua state to register bindings to
     */
    static void registerAll(lua_State* L);
    
private:
    // Entity manipulation functions
    static void registerEntityFunctions(lua_State* L);
    
    // Spawning functions
    static void registerSpawnFunctions(lua_State* L);
    
    // Query functions
    static void registerQueryFunctions(lua_State* L);
    
    // Utility functions
    static void registerUtilityFunctions(lua_State* L);
    
    // Game state functions
    static void registerGameFunctions(lua_State* L);
    
    // === Individual Lua C Functions ===
    
    // Entity functions
    static int lua_getPosition(lua_State* L);
    static int lua_setPosition(lua_State* L);
    static int lua_getVelocity(lua_State* L);
    static int lua_setVelocity(lua_State* L);
    static int lua_getHealth(lua_State* L);
    static int lua_setHealth(lua_State* L);
    static int lua_getTeam(lua_State* L);
    static int lua_destroyEntity(lua_State* L);
    static int lua_isEntityValid(lua_State* L);
    static int lua_getRotation(lua_State* L);
    static int lua_setRotation(lua_State* L);
    
    // Spawn functions
    static int lua_spawnProjectile(lua_State* L);
    static int lua_spawnEnemy(lua_State* L);
    
    // Query functions
    static int lua_findNearestEnemy(lua_State* L);
    static int lua_findNearestPlayer(lua_State* L);
    static int lua_getDistance(lua_State* L);
    static int lua_getEntitiesInRange(lua_State* L);
    
    // Utility functions
    static int lua_log(lua_State* L);
    static int lua_random(lua_State* L);
    static int lua_getTime(lua_State* L);
    
    // Game state functions
    static int lua_getScreenWidth(lua_State* L);
    static int lua_getScreenHeight(lua_State* L);
    static int lua_getPlayerCount(lua_State* L);
};

} // namespace ecs

#endif // ENABLE_LUA_SCRIPTING
