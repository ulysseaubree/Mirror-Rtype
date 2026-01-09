# R-Type Scripting System

This document describes the Lua scripting system for the R-Type game engine. The scripting system allows game designers to create custom enemy behaviors, movement patterns, and game logic without modifying C++ code.

## Table of Contents

1. [Overview](#overview)
2. [Getting Started](#getting-started)
3. [Script Structure](#script-structure)
4. [API Reference](#api-reference)
5. [Examples](#examples)
6. [Best Practices](#best-practices)
7. [Troubleshooting](#troubleshooting)

---

## Overview

The R-Type scripting system uses **Lua 5.4** as its scripting language. Lua was chosen for its:

- **Performance**: Extremely fast execution, suitable for real-time games
- **Simplicity**: Easy to learn syntax
- **Industry standard**: Used by World of Warcraft, Roblox, and many other games
- **Lightweight**: Minimal memory footprint (~200KB)

### Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                      Game Server                             │
│  ┌─────────────┐    ┌─────────────┐    ┌─────────────────┐  │
│  │    ECS      │◄───│  Scripting  │◄───│   Lua Scripts   │  │
│  │ Coordinator │    │   System    │    │  (*.lua files)  │  │
│  └─────────────┘    └─────────────┘    └─────────────────┘  │
│         ▲                  │                                 │
│         │                  ▼                                 │
│         │           ┌─────────────┐                         │
│         └───────────│ Lua Bindings│                         │
│                     │ (C++ → Lua) │                         │
│                     └─────────────┘                         │
└─────────────────────────────────────────────────────────────┘
```

### How It Works

1. Entities with a `ScriptComponent` are processed by the `ScriptingSystem`
2. The system calls Lua callbacks (`on_init`, `on_update`, etc.) for each entity
3. Lua scripts use the exposed API to query and modify entity state
4. Changes are applied to the ECS components

---

## Getting Started

### Creating Your First Script

1. Create a new file in the `scripts/enemies/` directory:

```lua
-- scripts/enemies/my_enemy.lua

function on_init(entity)
    utils.log("My enemy spawned!")
    entity.set_velocity(entity, -100, 0)  -- Move left
end

function on_update(entity, dt)
    -- Called every frame
end
```

2. Attach the script to an enemy entity in C++:

```cpp
Entity enemy = gCoordinator.CreateEntity();
// ... add Transform, Velocity, etc.

ScriptComponent script;
script.scriptPath = "enemies/my_enemy.lua";
gCoordinator.AddComponent(enemy, script);
```

3. The `ScriptingSystem` will automatically call your script's functions.

---

## Script Structure

### Callbacks

Every script can define these callback functions:

| Callback | Arguments | Description |
|----------|-----------|-------------|
| `on_init(entity)` | Entity ID | Called once when the entity is created |
| `on_update(entity, dt)` | Entity ID, Delta time | Called every frame |
| `on_collision(entity, other)` | Entity ID, Other entity ID | Called when colliding |
| `on_damage(entity, amount)` | Entity ID, Damage amount | Called when taking damage |
| `on_death(entity)` | Entity ID | Called before entity destruction |

### Example Template

```lua
--[[
    Enemy Name
    Description of behavior
]]

-- Configuration
local SPEED = 100
local SHOOT_COOLDOWN = 2.0

-- State (persists between on_update calls)
local timer = 0

function on_init(entity)
    utils.log("Entity " .. entity .. " initialized")
end

function on_update(entity, dt)
    timer = timer + dt
    -- Your logic here
end

function on_collision(entity, other)
    -- Handle collision
end

function on_damage(entity, amount)
    -- React to damage
end

function on_death(entity)
    -- Cleanup or spawn effects
end
```

---

## API Reference

### Entity Functions (`entity.*`)

#### Position

```lua
-- Get position
local x, y = entity.get_position(entity_id)

-- Set position
entity.set_position(entity_id, x, y)
```

#### Velocity

```lua
-- Get velocity
local vx, vy = entity.get_velocity(entity_id)

-- Set velocity
entity.set_velocity(entity_id, vx, vy)
```

#### Health

```lua
-- Get health (returns current, max)
local current, max = entity.get_health(entity_id)

-- Set health
entity.set_health(entity_id, new_health)
```

#### Team

```lua
-- Get team ID (0 = player, 1 = enemy, 2 = neutral)
local team = entity.get_team(entity_id)
```

#### Rotation

```lua
-- Get rotation (degrees)
local rotation = entity.get_rotation(entity_id)

-- Set rotation
entity.set_rotation(entity_id, angle_degrees)
```

#### Lifecycle

```lua
-- Check if entity exists
local valid = entity.is_valid(entity_id)

-- Destroy entity
entity.destroy(entity_id)
```

---

### Spawn Functions (`spawn.*`)

#### Projectile

```lua
-- Spawn a projectile
-- Returns: new entity ID
local bullet = spawn.projectile(x, y, velocity_x, velocity_y, team, damage)

-- Example: Enemy fires a bullet moving left
spawn.projectile(100, 200, -200, 0, 1, 10)
```

#### Enemy

```lua
-- Spawn an enemy with an optional script
-- Returns: new entity ID
local enemy = spawn.enemy(x, y, "enemies/bydos_wave.lua")

-- Example: Spawn a basic enemy
spawn.enemy(800, 300, "enemies/bydos_basic.lua")
```

---

### Query Functions (`query.*`)

#### Find Nearest Enemy

```lua
-- Find nearest enemy within range
-- Returns: entity ID or nil
local target = query.find_nearest_enemy(entity_id, range)

-- Example
local enemy = query.find_nearest_enemy(player_id, 300)
if enemy then
    utils.log("Found enemy: " .. enemy)
end
```

#### Find Nearest Player

```lua
-- Find nearest player within range
-- Returns: entity ID or nil
local player = query.find_nearest_player(entity_id, range)
```

#### Get Distance

```lua
-- Get distance between two entities
local dist = query.get_distance(entity1, entity2)
```

#### Get Entities in Range

```lua
-- Get all entities in range (optional team filter)
-- Returns: table of entity IDs
local entities = query.get_entities_in_range(x, y, range, team)

-- Example: Find all enemies near position
local enemies = query.get_entities_in_range(400, 300, 200, 1)
for i, enemy in ipairs(enemies) do
    utils.log("Enemy found: " .. enemy)
end
```

---

### Utility Functions (`utils.*`)

```lua
-- Log a message to console
utils.log("Hello from Lua!")

-- Get random number in range
local value = utils.random(min, max)

-- Get time since game start (seconds)
local time = utils.get_time()
```

---

### Game State Functions (`game.*`)

```lua
-- Get screen dimensions
local width = game.get_screen_width()   -- Returns 800
local height = game.get_screen_height() -- Returns 600

-- Get number of active players
local count = game.get_player_count()
```

---

## Examples

### Wave Movement Pattern

```lua
local SPEED = 80
local AMPLITUDE = 60
local FREQUENCY = 3

local startY = 0
local startTime = 0

function on_init(entity)
    local x, y = entity.get_position(entity)
    startY = y
    startTime = utils.get_time()
end

function on_update(entity, dt)
    local elapsed = utils.get_time() - startTime
    local waveOffset = math.sin(elapsed * FREQUENCY * 2 * math.pi) * AMPLITUDE
    
    local x, y = entity.get_position(entity)
    local targetY = startY + waveOffset
    local vy = (targetY - y) * 5
    
    entity.set_velocity(entity, -SPEED, vy)
end
```

### Shooting at Players

```lua
local SHOOT_COOLDOWN = 2.0
local BULLET_SPEED = 200
local shootTimer = 0

function on_update(entity, dt)
    shootTimer = shootTimer - dt
    
    if shootTimer <= 0 then
        local target = query.find_nearest_player(entity, 400)
        
        if target then
            shootTimer = SHOOT_COOLDOWN
            
            local x, y = entity.get_position(entity)
            local tx, ty = entity.get_position(target)
            
            local dx, dy = tx - x, ty - y
            local dist = math.sqrt(dx * dx + dy * dy)
            
            local vx = (dx / dist) * BULLET_SPEED
            local vy = (dy / dist) * BULLET_SPEED
            
            spawn.projectile(x, y, vx, vy, 1, 10)
        end
    end
end
```

### Chase Behavior

```lua
local CHASE_SPEED = 120
local DETECTION_RANGE = 300

function on_update(entity, dt)
    local target = query.find_nearest_player(entity, DETECTION_RANGE)
    
    if target then
        local x, y = entity.get_position(entity)
        local tx, ty = entity.get_position(target)
        
        local dx, dy = tx - x, ty - y
        local dist = math.sqrt(dx * dx + dy * dy)
        
        if dist > 30 then
            local vx = (dx / dist) * CHASE_SPEED
            local vy = (dy / dist) * CHASE_SPEED
            entity.set_velocity(entity, vx, vy)
        end
    else
        entity.set_velocity(entity, -60, 0)  -- Patrol
    end
end
```

---

## Best Practices

### 1. Use Local Variables

```lua
-- Good: Local variables are faster
local SPEED = 100
local timer = 0

-- Avoid: Global variables are slower and can cause conflicts
SPEED = 100  -- Don't do this!
```

### 2. Cache Expensive Calculations

```lua
-- Good: Calculate once, reuse
function on_update(entity, dt)
    local x, y = entity.get_position(entity)  -- Cache position
    
    -- Use x, y multiple times...
end
```

### 3. Avoid Expensive Operations in on_update

```lua
-- Avoid: Searching all entities every frame
function on_update(entity, dt)
    local enemies = query.get_entities_in_range(x, y, 1000, 1)  -- Expensive!
end

-- Better: Search periodically
local searchTimer = 0
local cachedTarget = nil

function on_update(entity, dt)
    searchTimer = searchTimer + dt
    
    if searchTimer > 0.5 then  -- Search every 0.5 seconds
        searchTimer = 0
        cachedTarget = query.find_nearest_player(entity, 300)
    end
    
    if cachedTarget then
        -- Use cached target
    end
end
```

### 4. Use the Patterns Library

```lua
local patterns = require("lib.patterns")

function on_update(entity, dt)
    patterns.wave(entity, 60, 3, -80)  -- Reusable wave pattern
end
```

---

## Troubleshooting

### Script Not Running

1. Check the script path is correct (relative to `scripts/` folder)
2. Ensure `ScriptComponent` is attached to the entity
3. Check console for Lua error messages

### Function Not Called

- Ensure function name is spelled correctly (`on_update`, not `onUpdate`)
- Check that the function is at global scope (not inside another function)

### Entity Operations Fail

- Verify the entity ID is valid using `entity.is_valid(id)`
- Check that the entity has the required components

### Performance Issues

- Reduce frequency of expensive operations
- Use local variables instead of globals
- Cache results that don't change often

---

## File Structure

```
scripts/
├── lib/
│   └── patterns.lua      # Reusable movement/shooting patterns
├── enemies/
│   ├── bydos_basic.lua   # Simple left-moving enemy
│   ├── bydos_wave.lua    # Sinusoidal movement
│   ├── bydos_shooter.lua # Enemy that shoots at players
│   ├── bydos_chaser.lua  # Aggressive chasing enemy
│   └── turret.lua        # Stationary shooting enemy
└── bosses/
    └── (future boss scripts)
```

---

## Building with Scripting Support

Scripting is enabled by default. To disable:

```bash
cmake -DENABLE_SCRIPTING=OFF ..
```

The scripting system requires Lua 5.3 or 5.4. If not found on the system, it will be downloaded automatically via CMake FetchContent.
