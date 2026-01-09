--[[
    Basic Bydos Enemy
    
    A simple enemy that moves in a straight line from right to left.
    This is the most basic enemy type in R-Type.
    
    Behavior:
    - Spawns on the right side of the screen
    - Moves left at constant speed
    - No shooting
    - Dies in one hit
]]

-- Called once when the entity is created
function on_init(entity)
    utils.log("Basic Bydos spawned: " .. entity)
    
    -- Set initial velocity (moving left)
    entity.set_velocity(entity, -100, 0)
end

-- Called every frame
function on_update(entity, dt)
    -- Basic enemy just moves left, no special behavior
    -- Velocity was set in on_init
end

-- Called when this entity collides with another
function on_collision(entity, other)
    local otherTeam = entity.get_team(other)
    
    -- If we hit a player or player projectile (team 0)
    if otherTeam == 0 then
        utils.log("Basic Bydos hit by player!")
        -- Damage is handled by the collision system
    end
end

-- Called when this entity takes damage
function on_damage(entity, amount)
    utils.log("Basic Bydos took " .. amount .. " damage!")
end

-- Called when this entity is about to be destroyed
function on_death(entity)
    utils.log("Basic Bydos destroyed!")
    -- Could spawn explosion particles or power-ups here
end