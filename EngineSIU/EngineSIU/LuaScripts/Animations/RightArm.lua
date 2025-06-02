local EngineTypes = EngineTypes
local CustomEnums = CustomEnums
setmetatable(_ENV, {
    __index = function(t, k)
        return rawget(EngineTypes, k) or rawget(CustomEnums, k)
    end
})


AnimFSM = {
    current = "Idle",
    
    Update = function(self, dt)
        -- self.current = "Contents/Fbx/Capoeira.fbx"

        -- self.current = "Contents/Human/FastRun"
        -- self.current = "Contents/Human/NarutoRun"
        
        print(self.Owner)
        print(self.Owner.State)
        if(self.Owner.State == PlayerState.Shooting) then
            self.current = "Contents/Armature|GunPlay"
        end

        return {
            anim = self.current,
            blend = 5.0
        }
    end,

    ChangeMode = function(self)
        self.current = "Contents/Armature|GunPlay"
    end,
}

return AnimFSM
