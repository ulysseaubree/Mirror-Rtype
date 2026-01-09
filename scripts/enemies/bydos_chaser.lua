--[[
    Chaser Bydos Enemy
    
    An aggressive enemy that chases the nearest player.
    
    Behavior:
    - Detects nearby players
    - Actively moves towards the player
    - If no player nearby, patrols left slowly
    - Faster and more dangerous than basic enemies
]]

-- Configuration
local PATROL_SPEED = 60        -- Speed when no target
local CHASE_SPEED = 120        -- Speed when chasing
local DETECTION_RANGE = 300    -- Range to detect players
local ATTACK_RANGE = 30        -- Range considered "close enough"

-- State
local state = "patrol"  -- "patrol" or "chase"
local targetEntity = nil

-- Called once when the entity is created
function on_init(entity)
    utils.log("Chaser Bydos spawned: " .. entity)
    state = "patrol"
end

-- Called every frame
function on_update(entity, dt)
    -- Look for a target
    local nearestPlayer = query.find_nearest_player(entity, DETECTION_RANGE)
    
    if nearestPlayer then
        -- Switch to chase mode
        state = "chase"
        targetEntity = nearestPlayer
        
        -- Get positions
        local x, y = entity.get_position(entity)
        local tx, ty = entity.get_position(targetEntity)
        
        -- Calculate direction to player
        local dx = tx - x
        local dy = ty - y
        local dist = math.sqrt(dx * dx + dy * dy)
        
        if dist > ATTACK_RANGE then
            -- Move towards player
            local vx = (dx / dist) * CHASE_SPEED
            local vy = (dy / dist) * CHASE_SPEED
            entity.set_velocity(entity, vx, vy)
        else
            -- Close enough - slow down (collision will handle damage)
            entity.set_velocity(entity, 0, 0)
        end
    else
        -- No target found, patrol
        state = "patrol"
        targetEntity = nil
        
        -- Simple left movement with slight wave
        local time = utils.get_time()
        local vy = math.sin(time * 2) * 30
        entity.set_velocity(entity, -PATROL_SPEED, vy)
    end
end

-- Called when this entity collides with another
function on_collision(entity, other)
    local otherTeam = entity.get_team(other)
    
    if otherTeam == 0 then
        utils.log("Chaser Bydos collided with player!")
    end
end

-- Called when this entity takes damage
function on_damage(entity, amount)
    utils.log("Chaser Bydos took " .. amount .. " damage! Getting angry...")
    -- Could increase aggression here
end

-- Called when this entity is about to be destroyed
function on_death(entity)
    utils.log("Chaser Bydos destroyed!")
end