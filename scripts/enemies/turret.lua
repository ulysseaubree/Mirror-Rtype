--[[
    Turret Enemy
    
    A stationary enemy that doesn't move but fires at players.
    Typically placed at strategic positions on the level.
    
    Behavior:
    - Stays in place (no movement)
    - Rotates to face the nearest player
    - Fires periodically
    - Has more health than regular enemies
]]

-- Configuration
local SHOOT_COOLDOWN = 1.5     -- Seconds between shots
local BULLET_SPEED = 250       -- Projectile speed
local BULLET_DAMAGE = 15       -- Damage per hit
local DETECTION_RANGE = 500    -- Range to detect players
local ROTATION_SPEED = 3       -- How fast the turret rotates

-- State
local shootTimer = 0
local currentAngle = 180       -- Start facing left (degrees)

-- Called once when the entity is created
function on_init(entity)
    utils.log("Turret spawned: " .. entity)
    
    -- Turrets don't move
    entity.set_velocity(entity, 0, 0)
    
    -- Random initial delay
    shootTimer = utils.random(0, SHOOT_COOLDOWN)
end

-- Called every frame
function on_update(entity, dt)
    local x, y = entity.get_position(entity)
    
    -- Find nearest player
    local target = query.find_nearest_player(entity, DETECTION_RANGE)
    
    if target then
        -- Get target position
        local tx, ty = entity.get_position(target)
        
        -- Calculate angle to target
        local dx = tx - x
        local dy = ty - y
        local targetAngle = math.deg(math.atan2(dy, dx))
        
        -- Smoothly rotate towards target
        local angleDiff = targetAngle - currentAngle
        
        -- Normalize angle difference to [-180, 180]
        while angleDiff > 180 do angleDiff = angleDiff - 360 end
        while angleDiff < -180 do angleDiff = angleDiff + 360 end
        
        -- Apply rotation
        if math.abs(angleDiff) > 1 then
            if angleDiff > 0 then
                currentAngle = currentAngle + ROTATION_SPEED
            else
                currentAngle = currentAngle - ROTATION_SPEED
            end
        end
        
        -- Update entity rotation (for visual representation)
        entity.set_rotation(entity, currentAngle)
        
        -- Update shoot timer
        shootTimer = shootTimer - dt
        
        -- Only shoot if roughly facing the target
        if shootTimer <= 0 and math.abs(angleDiff) < 30 then
            shootTimer = SHOOT_COOLDOWN
            
            -- Fire in the direction we're facing
            local rad = math.rad(currentAngle)
            local vx = math.cos(rad) * BULLET_SPEED
            local vy = math.sin(rad) * BULLET_SPEED
            
            spawn.projectile(x, y, vx, vy, 1, BULLET_DAMAGE)
            utils.log("Turret fired!")
        end
    else
        -- No target - slowly scan back and forth
        local time = utils.get_time()
        currentAngle = 180 + math.sin(time * 0.5) * 45  -- Scan 45 degrees each way
        entity.set_rotation(entity, currentAngle)
    end
end

-- Called when this entity collides with another
function on_collision(entity, other)
    local otherTeam = entity.get_team(other)
    
    if otherTeam == 0 then
        utils.log("Turret hit!")
    end
end

-- Called when this entity is about to be destroyed
function on_death(entity)
    utils.log("Turret destroyed!")
    -- Spawn explosion effect
end