--[[
    Wave Bydos Enemy
    
    An enemy that moves in a sinusoidal wave pattern.
    Classic R-Type behavior for swarm enemies.
    
    Behavior:
    - Moves left while oscillating up and down
    - Wave amplitude and frequency can be customized
    - No shooting
]]

-- Configuration
local SPEED = 80           -- Horizontal speed
local AMPLITUDE = 60       -- Wave height (pixels)
local FREQUENCY = 3        -- Wave frequency (oscillations per second)

-- State
local startY = 0
local startTime = 0

-- Called once when the entity is created
function on_init(entity)
    utils.log("Wave Bydos spawned: " .. entity)
    
    -- Store initial Y position for wave calculation
    local x, y = entity.get_position(entity)
    startY = y
    startTime = utils.get_time()
end

-- Called every frame
function on_update(entity, dt)
    local x, y = entity.get_position(entity)
    local elapsed = utils.get_time() - startTime
    
    -- Calculate wave offset
    local waveOffset = math.sin(elapsed * FREQUENCY * 2 * math.pi) * AMPLITUDE
    
    -- Set velocity: constant left movement + vertical oscillation
    local targetY = startY + waveOffset
    local vy = (targetY - y) * 5  -- Smooth interpolation
    
    entity.set_velocity(entity, -SPEED, vy)
end

-- Called when this entity collides with another
function on_collision(entity, other)
    local otherTeam = entity.get_team(other)
    
    if otherTeam == 0 then
        utils.log("Wave Bydos hit!")
    end
end

-- Called when this entity is about to be destroyed
function on_death(entity)
    utils.log("Wave Bydos destroyed!")
end