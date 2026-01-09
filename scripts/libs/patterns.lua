--[[
    R-Type Scripting Library
    
    This file contains reusable movement patterns and utility functions
    for enemy behaviors. Include it in your scripts with:
    
        local patterns = require("lib.patterns")
]]

local patterns = {}

-- ============================================================================
-- Movement Patterns
-- ============================================================================

--- Creates a sinusoidal wave movement pattern
-- @param entity The entity ID
-- @param amplitude The wave amplitude (pixels)
-- @param frequency How fast the wave oscillates
-- @param baseSpeed Horizontal speed (negative = left)
function patterns.wave(entity, amplitude, frequency, baseSpeed)
    local x, y = entity.get_position(entity)
    local time = utils.get_time()
    
    local vy = math.sin(time * frequency) * amplitude
    entity.set_velocity(entity, baseSpeed, vy)
end

--- Creates a zigzag movement pattern
-- @param entity The entity ID
-- @param speed Movement speed
-- @param switchTime Time between direction changes
-- @param state Table to store state (must persist between calls)
function patterns.zigzag(entity, speed, switchTime, state)
    state.timer = (state.timer or 0) + 0.016  -- Approximate dt
    state.direction = state.direction or 1
    
    if state.timer >= switchTime then
        state.direction = -state.direction
        state.timer = 0
    end
    
    entity.set_velocity(entity, -speed, state.direction * speed * 0.5)
end

--- Creates a circular movement pattern
-- @param entity The entity ID
-- @param centerX Center X of the circle
-- @param centerY Center Y of the circle
-- @param radius Circle radius
-- @param speed Angular speed (radians per second)
function patterns.circle(entity, centerX, centerY, radius, speed)
    local time = utils.get_time()
    local angle = time * speed
    
    local x = centerX + math.cos(angle) * radius
    local y = centerY + math.sin(angle) * radius
    
    entity.set_position(entity, x, y)
end

--- Creates a figure-8 movement pattern
-- @param entity The entity ID
-- @param centerX Center X
-- @param centerY Center Y
-- @param width Width of the figure-8
-- @param height Height of the figure-8
-- @param speed Angular speed
function patterns.figure8(entity, centerX, centerY, width, height, speed)
    local time = utils.get_time()
    local angle = time * speed
    
    local x = centerX + math.sin(angle) * width
    local y = centerY + math.sin(angle * 2) * height * 0.5
    
    entity.set_position(entity, x, y)
end

--- Move towards a target position smoothly
-- @param entity The entity ID
-- @param targetX Target X position
-- @param targetY Target Y position
-- @param speed Movement speed
function patterns.move_towards(entity, targetX, targetY, speed)
    local x, y = entity.get_position(entity)
    
    local dx = targetX - x
    local dy = targetY - y
    local dist = math.sqrt(dx * dx + dy * dy)
    
    if dist > 1 then
        local vx = (dx / dist) * speed
        local vy = (dy / dist) * speed
        entity.set_velocity(entity, vx, vy)
    else
        entity.set_velocity(entity, 0, 0)
    end
end

--- Move away from a target (flee behavior)
-- @param entity The entity ID
-- @param targetX Target X position to flee from
-- @param targetY Target Y position to flee from
-- @param speed Movement speed
function patterns.flee_from(entity, targetX, targetY, speed)
    local x, y = entity.get_position(entity)
    
    local dx = x - targetX
    local dy = y - targetY
    local dist = math.sqrt(dx * dx + dy * dy)
    
    if dist > 0.1 then
        local vx = (dx / dist) * speed
        local vy = (dy / dist) * speed
        entity.set_velocity(entity, vx, vy)
    end
end

-- ============================================================================
-- Shooting Patterns
-- ============================================================================

--- Fire a single projectile towards a target
-- @param entity The entity ID (shooter)
-- @param targetEntity The target entity ID
-- @param speed Projectile speed
-- @param damage Projectile damage
function patterns.shoot_at_target(entity, targetEntity, speed, damage)
    if not targetEntity then return end
    
    local x, y = entity.get_position(entity)
    local tx, ty = entity.get_position(targetEntity)
    
    local dx = tx - x
    local dy = ty - y
    local dist = math.sqrt(dx * dx + dy * dy)
    
    if dist > 0.1 then
        local vx = (dx / dist) * speed
        local vy = (dy / dist) * speed
        spawn.projectile(x, y, vx, vy, 1, damage or 10)
    end
end

--- Fire a spread of projectiles
-- @param entity The entity ID (shooter)
-- @param numBullets Number of bullets in the spread
-- @param spreadAngle Total angle of the spread (degrees)
-- @param speed Projectile speed
-- @param damage Projectile damage
function patterns.shoot_spread(entity, numBullets, spreadAngle, speed, damage)
    local x, y = entity.get_position(entity)
    local baseAngle = 180  -- Fire to the left
    local angleStep = spreadAngle / (numBullets - 1)
    local startAngle = baseAngle - spreadAngle / 2
    
    for i = 0, numBullets - 1 do
        local angle = math.rad(startAngle + angleStep * i)
        local vx = math.cos(angle) * speed
        local vy = math.sin(angle) * speed
        spawn.projectile(x, y, vx, vy, 1, damage or 10)
    end
end

--- Fire projectiles in a circular pattern
-- @param entity The entity ID (shooter)
-- @param numBullets Number of bullets
-- @param speed Projectile speed
-- @param damage Projectile damage
function patterns.shoot_circle(entity, numBullets, speed, damage)
    local x, y = entity.get_position(entity)
    local angleStep = (2 * math.pi) / numBullets
    
    for i = 0, numBullets - 1 do
        local angle = angleStep * i
        local vx = math.cos(angle) * speed
        local vy = math.sin(angle) * speed
        spawn.projectile(x, y, vx, vy, 1, damage or 10)
    end
end

-- ============================================================================
-- Utility Functions
-- ============================================================================

--- Check if entity is off-screen
-- @param entity The entity ID
-- @param margin Extra margin beyond screen edges
function patterns.is_offscreen(entity, margin)
    margin = margin or 50
    local x, y = entity.get_position(entity)
    local w = game.get_screen_width()
    local h = game.get_screen_height()
    
    return x < -margin or x > w + margin or y < -margin or y > h + margin
end

--- Clamp entity position to screen bounds
-- @param entity The entity ID
-- @param margin Margin from edges
function patterns.clamp_to_screen(entity, margin)
    margin = margin or 0
    local x, y = entity.get_position(entity)
    local w = game.get_screen_width()
    local h = game.get_screen_height()
    
    x = math.max(margin, math.min(x, w - margin))
    y = math.max(margin, math.min(y, h - margin))
    
    entity.set_position(entity, x, y)
end

return patterns
