require 'core/class'

FPSCamera = class(FPSCamera)

function FPSCamera:init(world, camera_unit, fly_mode)
    self.world = world
    self.unit = camera_unit
    self.camera = Camera.GetCamera(world, camera_unit)
    self.transform = Transform.GetTransform(world, camera_unit)

    self.fly_mode = fly_mode or false
end

function FPSCamera:setFlyMode(enable)
    self.fly_mode = enable
end

function FPSCamera:update(dt, dx, dy, updown, leftright)
    local pos = Transform.GetPosition(self.world, self.transform)
    local rot = Transform.GetRotation(self.world, self.transform)

    local m_world = Transform.GetWorldMatrix(self.world, self.transform)
    local z_dir = Mat4f.Z(m_world)
    local x_dir = Mat4f.X(m_world)

    -- Rotation
    local rotation_around_world_up = Quatf.FromAxisAngle(Vec3f.UnitY, dx * 1.0)
    local rotation_around_camera_right = Quatf.FromAxisAngle(x_dir, dy * -1.0)
    local rotation = rotation_around_camera_right * rotation_around_world_up
    Transform.SetRotation(self.world, self.transform, rot * rotation)

    -- Position
    if not self.fly_mode then
        z_dir.Y = 0.0
    end

    pos = pos + z_dir * updown
    pos = pos + x_dir * leftright

    Transform.SetPosition(self.world, self.transform, pos)
end